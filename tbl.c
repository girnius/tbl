/* tbl: Fast and simple hash table
 * Copyright (c) 2024 Vakaris Girnius <vakaris@girnius.dev>
 *
 * This source code is licensed under the zlib license (found in the
 * LICENSE file in this repository)
*/

#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "tbl.h"
#include "tbl_list.h"

#define XXH_INLINE_ALL 1
#define XXH_NO_STREAM 1
#include "xxhash.h"

#define _IS_LIST(x) ((x) == &(x))
#define _SET_IS_LIST(x) ((x) = &(x))

struct tbl_bkt{
	union{
		void *e1;
		struct tbl *is_list;
	};
	union{
		void *e2;
		struct tbl_list *list;
	};
};

struct tbl{
	struct tbl_bkt *a;
	uint64_t seed;
	uint64_t n_entries;
	uint32_t max;
	uint32_t max_lg2;
	const char *(*getkey)(void *value);
};

static uint32_t _hash_to_pos(int size_lg2, uint64_t hash)
{
	return (uint32_t)hash & ~(UINT32_MAX << size_lg2);
}

static struct tbl* _create_with_sizelog2(uint32_t max_lg2,
					 const char *(*getkey)(void *value))
{
	struct tbl *t = malloc(sizeof(struct tbl));
	if (!t)
		return NULL;
	t->max_lg2 = max_lg2;
	t->max = 1 << t->max_lg2;
	t->a = calloc(0, sizeof(struct tbl_bkt) << t->max_lg2);
	if (!t->a){
		free(t);
		return NULL;
	}
	t->seed = (uint64_t)t->a;
	t->n_entries = 0;
	t->getkey = getkey;
	return t;
}

static const char *_getkey_default(void *value)
{
	return (const char*)value;
}

struct tbl* tbl_create(const char *(*getkey)(void *value))
{
	if (getkey)
		return _create_with_sizelog2(
				ceil(log2(HT8_DEFAULT_BUCKETS_N)), getkey);
	else
		return _create_with_sizelog2(
				ceil(log2(HT8_DEFAULT_BUCKETS_N)),
						_getkey_default);
}


static int _put_with_hash(struct tbl *t, uint64_t hash, void *value)
{
	assert(t && value);
	uint32_t pos = _hash_to_pos(t->max_lg2, hash);
	if (!t->a[pos].e1){
		t->a[pos].e1 = value;
	}else if (!t->a[pos].e2){
		t->a[pos].e2 = value;
	}else{
		if (_IS_LIST(t->a[pos].is_list)){
			if (_list_add(t->a[pos].list, value))
				return -1;
		}else{
			struct tbl_list *l = _list_create();
			if (!l)
				return -1;
			_list_add(l, t->a[pos].e1);
			_list_add(l, t->a[pos].e2);
			_list_add(l, value);
			_SET_IS_LIST(t->a[pos].is_list);
			t->a[pos].list = l;
		}
	}
	t->n_entries++;
	return 0;
}

int tbl_put(struct tbl *t, void *value)
{
	assert(t && key && value);
	const char *key = t->getkey(value);
	uint64_t hash = XXH64(key, strlen(key), t->seed);
	if (t->n_entries >
			(t->max -(t->max / HT8_MIN_FREE_BUCKETS_RATIO))){
		tbl_grow(t);
	}
	if (!_put_with_hash(t, hash, value))
		return 0;
	else
		return -1;
}

void *tbl_get(struct tbl *t, const char *key)
{
	assert(t && key);
	uint64_t hash = XXH64(key, strlen(key), t->seed);
	uint32_t pos = _hash_to_pos(t->max_lg2, hash);
	void *found;
	if (!t->a[pos].e2){
		found = t->a[pos].e1;
	}else{
		if (!t->a[pos].e1){
			found = t->a[pos].e2;
		}else{
			if (_IS_LIST(t->a[pos].is_list))
				return _list_get(t->a[pos].list, t->getkey,
								key);
		}
	}
	if (!strcmp(t->getkey(found), key))
		return found;
	else
		return NULL;
}

void *tbl_remove(struct tbl *t, const char *key)
{
	assert(t && key);
	uint64_t hash = XXH64(key, strlen(key), t->seed);
	uint32_t pos = _hash_to_pos(t->max_lg2, hash);
	void *ret;
	void **found;
	if (!t->a[pos].e2){
		found = &t->a[pos].e1;
	}else{
		if (!t->a[pos].e1){
			found = &t->a[pos].e2;
		}else{
			if (_IS_LIST(t->a[pos].is_list)){
				return _list_remove(t->a[pos].list,
						    t->getkey, key);
			}else{
				return NULL;
			}
		}
	}
	if (!strcmp(t->getkey(*found), key)){
		ret = *found;
		*found = NULL;
		return ret;
	}else{
		return NULL;
	}
}

int tbl_iterate(struct tbl *t, int (*iter)(void *value, void *ctx), void *ctx)
{
	int ret = 0;
	assert(t && iter);
	for (uint32_t i=0; i < t->max; i++){
		if (!_IS_LIST(t->a[i].is_list)){
			if (t->a[i].e1){
				if (ret = iter(t->a[i].e1, ctx))
					return ret;
			}
			if (t->a[i].e2){
				if (ret = iter(t->a[i].e2, ctx))
					return ret;
			}
		}else{
			struct tbl_list *curr = t->a[i].list;
			do{
				for (int i=0; i < _LIST_ENTRIES_N; i++){
					if (curr->entries[i]){
						if (ret = iter(t->a[i].e1,
									ctx))
							return ret;
					}
				}
				curr = curr->next;
			}while (curr);
		}
	}
	return ret;
}

static int _copy_iter(void *value, void *ctx)
{
	assert(value && ctx);
	struct tbl *t = (struct tbl*)ctx;
	if(tbl_put(t, t->getkey(value), value))
		return -1;
	return 0;
}
int tbl_copy(struct tbl *dest, struct tbl *src)
{
	assert(src && dest);
	return tbl_iterate(src, _copy_iter, dest);
}

int tbl_renew(struct tbl *t)
{
	assert(t);
	struct tbl *new_t = tbl_create(t->getkey);
	if (!new_t)
		return -1;
	if (tbl_copy(new_t, t)){
		tbl_free(new_t);
		return -1;
	}
	void *old_a = t->a;
	*t = *new_t;
	free(new_t);
	free(old_a);
	return 0;
}

void tbl_clean(struct tbl *t)
{
	assert(t);
	for (uint32_t i=0; i < t->max; i++){
		if (_IS_LIST(t->a[i].is_list))
			_list_free(t->a[i].list);
	}
	memset(t->a, 0, ((sizeof(struct tbl_bkt) << t->max_lg2) /
				sizeof(unsigned char)));
	return;
}

int tbl_grow(struct tbl *t)
{
	assert(t);
	struct tbl *newht = _create_with_sizelog2(t->max_lg2 + 1, t->getkey);
	if (!newht)
		return -1;
	if (tbl_copy(newht, t)){
		tbl_free(newht);
		return -1;
	}
	void *oldtable = t->a;
	*t = *newht;
	free(newht);
	free(oldtable);
	return 0;
}

void tbl_setfunc(struct tbl *t, const char *(*getkey)(void *value))
{
	assert(t && getkey);
	t->getkey = getkey;
	return;
}

uint32_t tbl_getnum(struct tbl *t)
{
	assert(t);
	return t->n_entries;
}

void tbl_free(struct tbl *t)
{
	assert(t);
	for (uint32_t i=0; i < t->max; i++){
		if (_IS_LIST(t->a[i].is_list))
			_list_free(t->a[i].list);
	}
	free(t->a);
	free(t);
	return;
}

