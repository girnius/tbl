/* tbl: Fast and simple hash table
 * Copyright (c) 2024 Vakaris Girnius <vakaris@girnius.dev>
 *
 * This source code is licensed under the zlib license (found in the
 * LICENSE file in this repository)
*/

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "tbl.h"

#define XXH_INLINE_ALL 1
#define XXH_NO_STREAM 1
#include "xxhash.h"

#define _LIST_ENTRIES_N 4

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


struct tbl_list{
	void *entries[_LIST_ENTRIES_N];
	struct tbl_list *next;
};

static struct tbl_list *_list_create(void);
static int _list_add(struct tbl_list *l, void *value);
static void *_list_get(struct tbl_list *l,
		       const char *(*getkey)(void *value),
		       const char *key);
static void *_list_remove(struct tbl_list *l,
			  const char *(*getkey)(void *value),
			  const char *key);
static void _list_free(struct tbl_list *l);

static int _is_list(struct tbl *t, struct tbl_bkt *b);
static void _set_is_list(struct tbl *t, struct tbl_bkt *b);


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
		return _create_with_sizelog2(TBL_DEFAULT_BUCKETS_N, getkey);
	else
		return _create_with_sizelog2(TBL_DEFAULT_BUCKETS_N,
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
		if (_is_list(t, t->a + pos)){
			if (_list_add(t->a[pos].list, value))
				return -1;
		}else{
			struct tbl_list *l = _list_create();
			if (!l)
				return -1;
			_list_add(l, t->a[pos].e1);
			_list_add(l, t->a[pos].e2);
			_list_add(l, value);
			_set_is_list(t, t->a + pos);
			t->a[pos].list = l;
		}
	}
	t->n_entries++;
	return 0;
}

int tbl_put(struct tbl *t, void *value)
{
	assert(t && value);
	const char *key = t->getkey(value);
	uint64_t hash = XXH64(key, strlen(key), t->seed);
	if (t->n_entries >
			(t->max -(t->max / TBL_MIN_FREE_BUCKETS_RATIO))){
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
	if (_is_list(t, t->a + pos))
		return _list_get(t->a[pos].list, t->getkey, key);
	if (t->a[pos].e1)
		found = t->a[pos].e1;
	else if (t->a[pos].e2)
		found = t->a[pos].e2;
	else
		return NULL;
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
	if (_is_list(t, t->a + pos))
		return _list_remove(t->a[pos].list, t->getkey, key);
	if (t->a[pos].e1)
		found = &t->a[pos].e1;
	else if (t->a[pos].e2)
		found = &t->a[pos].e2;
	else
		return NULL;
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
		if (!_is_list(t, t->a + i)){
			if (t->a[i].e1){
				if ((ret = iter(t->a[i].e1, ctx)))
					return ret;
			}
			if (t->a[i].e2){
				if ((ret = iter(t->a[i].e2, ctx)))
					return ret;
			}
		}else{
			struct tbl_list *curr = t->a[i].list;
			do{
				for (int i=0; i < _LIST_ENTRIES_N; i++){
					if (curr->entries[i]){
						if ((ret = iter(t->a[i].e1,
									ctx)))
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
	if(tbl_put(t, value))
		return -1;
	return 0;
}
int tbl_copy(struct tbl *dest, struct tbl *src)
{
	assert(src && dest);
	return tbl_iterate(src, _copy_iter, dest);
}

int tbl_recreate(struct tbl *t)
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
		if (_is_list(t, t->a + i))
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
		if (_is_list(t, t->a + i))
			_list_free(t->a[i].list);
	}
	free(t->a);
	free(t);
	return;
}

static int _is_list(struct tbl *t, struct tbl_bkt *b)
{
	assert(t && b);
	if (b->is_list == t)
		return 1;
	else
		return 0;
}

static void _set_is_list(struct tbl *t, struct tbl_bkt *b)
{
	assert(t && b);
	b->is_list = t;
	return;
}

static struct tbl_list *_list_create(void)
{
	return calloc(0, sizeof(struct tbl_list));
}

static int _list_add(struct tbl_list *l, void *value)
{
	struct tbl_list *curr = l;
	do{
		for (int i=0; i < _LIST_ENTRIES_N; i++){
			if (!curr->entries[i]){
				curr->entries[i] = value;
				return 0;
			}
		}
	}while (curr->next);
	struct tbl_list *newlist = _list_create();
	if (!newlist)
		return -1;
	curr->next = newlist;
	newlist->entries[0] = value;
	return 0;
}

static void *_list_get(struct tbl_list *l,
		       const char *(*getkey)(void *value),
		       const char *key)
{
	struct tbl_list *curr = l;
	do{
		for (int i =0; i < _LIST_ENTRIES_N; i++){
			if (curr->entries[i]){
				const char *entrykey =
						getkey(curr->entries[i]);
				if (!strcmp(key, entrykey))
					return curr->entries[i];
			}
		}
		curr = curr->next;
	}while (curr);
	return NULL;
}

static void *_list_remove(struct tbl_list *l,
			  const char *(*getkey)(void *value),
			  const char *key)
{
	struct tbl_list *curr = l;
	do{
		for (int i =0; i < _LIST_ENTRIES_N; i++){
			if (curr->entries[i]){
				const char *entrykey =
						getkey(curr->entries[i]);
				if (!strcmp(key, entrykey)){
					void *ret = curr->entries[i];
					curr->entries[i] = NULL;
					return ret;
				}
			}
		}
		curr = curr->next;
	}while (curr);
	return NULL;
}

static void _list_free(struct tbl_list *l)
{
	struct tbl_list *prev;
	struct tbl_list *curr = l;
	do{
		prev = curr;
		curr = curr->next;
		free(prev);
	}while (curr);
	return;
}
