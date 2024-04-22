/* ht8: Fast and simple hash table
 * Copyright (c) 2024 Vakaris Girnius <dev@girnius.me>
 *
 * This source code is licensed under the zlib license (found in the
 * LICENSE file in this repository)
*/

#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "ht8.h"

#define XXH_INLINE_ALL 1
#define XXH_NO_STREAM 1
#include "xxhash.h"

#define _LIST_ENTRIES_N 4

struct ht8_list{
	void *entries[_LIST_ENTRIES_N];
	struct ht8_list *next;
};

struct ht8_bkt{
	union{
		void *e1;
		struct ht8 *is_list;
	};
	union{
		void *e2;
		struct ht8_list *list;
	};
};

struct ht8{
	struct ht8_bkt *t;
	uint64_t seed;
	uint64_t n_entries;
	uint32_t max;
	uint32_t max_lg2;
	const char *(*getkey)(void *value);
};

static struct ht8_list *_list_create(void)
{
	return calloc(0, sizeof(struct ht8_list));
}

static int _list_add(struct ht8_list *l, void *value)
{
	for (int i=0; i < _LIST_ENTRIES_N; i++){
		if (!l->entries[i]){
			l->entries[i] = value;
			return 0;
		}
	}
	struct ht8_list *newlist = _list_create();
	if (!newlist)
		return -1;
	l->next = newlist;
	newlist->entries[0] = value;
	return 0;
}

static void *_list_get(struct ht8_list *l, struct ht8 *ht, const char *key)
{
	struct ht8_list *curr = l;
	do{
		for (int i =0; i < _LIST_ENTRIES_N; i++){
			if (curr->entries[i]){
				const char *entrykey =
						ht->getkey(curr->entries[i]);
				if (!strcmp(key, entrykey))
					return curr->entries[i];
			}
		}
		curr = curr->next;
	}while (curr);
	return NULL;
}

static void *_list_remove(struct ht8_list *l, struct ht8 *ht, const char *key)
{
	struct ht8_list *curr = l;
	do{
		for (int i =0; i < _LIST_ENTRIES_N; i++){
			if (curr->entries[i]){
				const char *entrykey =
						ht->getkey(curr->entries[i]);
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

static void _list_free(struct ht8_list *l)
{
	struct ht8_list *prev;
	struct ht8_list *curr = l;
	do{
		prev = curr;
		curr = curr->next;
		free(prev);
	}while (curr);
	return;
}

static struct ht8* _create_with_sizelog2(uint32_t max_lg2, const char *(*getkey)(void *value))
{
	struct ht8 *ht = malloc(sizeof(struct ht8));
	if (!ht)
		return NULL;
	ht->max_lg2 = max_lg2;
	ht->max = 1 << ht->max_lg2;
	ht->t = calloc(0, sizeof(struct ht8_bkt) << ht->max_lg2);
	if (!ht->t){
		free(ht);
		return NULL;
	}
	ht->seed = (uint64_t)ht->t;
	ht->n_entries = 0;
	ht->getkey = getkey;
	return ht;
}

static const char *_getkey_default(void *value)
{
	return (const char*)value;
}

struct ht8* ht8_create(const char *(*getkey)(void *value))
{
	if (getkey)
		return _create_with_sizelog2(
				ceil(log2(HT8_DEFAULT_BUCKETS_N)), getkey);
	else
		return _create_with_sizelog2(
				ceil(log2(HT8_DEFAULT_BUCKETS_N)),
						_getkey_default);
}


static int _put_with_hash(struct ht8 *ht, uint32_t hash, void *value)
{
	assert(ht && value);
	uint32_t pos = (uint32_t)hash & ~(UINT32_MAX << ht->max_lg2);
	if (!ht->t[pos].e1){
		ht->t[pos].e1 = value;
	}else if (!ht->t[pos].e2){
		ht->t[pos].e2 = value;
	}else{
		if (ht->t[pos].is_list == ht){
			if (_list_add(ht->t[pos].list, value))
				return -1;
		}else{
			struct ht8_list *l = _list_create();
			if (!l)
				return -1;
			_list_add(l, ht->t[pos].e1);
			_list_add(l, ht->t[pos].e2);
			_list_add(l, value);
			ht->t[pos].is_list = ht;
			ht->t[pos].list = l;
		}
	}
	ht->n_entries++;
	return 0;
}

int ht8_put(struct ht8 *ht, const char *key, void *value)
{
	assert(ht && key && value);
	uint64_t hash = XXH64(key, strlen(key), ht->seed);
	if (ht->n_entries > ht->max - (ht->max / HT8_MIN_FREE_BUCKETS_RATIO)){
		ht8_grow(ht);
	}
	if (!_put_with_hash(ht, (uint32_t)hash, value))
		return 0;
	else
		return -1;
}

void *ht8_get(struct ht8 *ht, const char *key)
{
	assert(ht && key);
	uint64_t hash = XXH64(key, strlen(key), ht->seed);
	uint32_t pos = (uint32_t)hash & ~(UINT32_MAX << ht->max_lg2);
	void *found;
	if (!ht->t[pos].e2){
		found = ht->t[pos].e1;
	}else{
		if (!ht->t[pos].e1){
			found = ht->t[pos].e2;
		}else{
			if (ht->t[pos].is_list == ht)
				return _list_get(ht->t[pos].list, ht, key);
		}
	}
	if (!strcmp(ht->getkey(found), key))
		return found;
	else
		return NULL;
}

void *ht8_remove(struct ht8 *ht, const char *key)
{
	assert(ht && key);
	uint64_t hash = XXH64(key, strlen(key), ht->seed);
	uint32_t pos = (uint32_t)hash & ~(UINT32_MAX << ht->max_lg2);
	void *ret;
	void **found;
	if (!ht->t[pos].e2){
		found = &ht->t[pos].e1;
	}else{
		if (!ht->t[pos].e1){
			found = &ht->t[pos].e2;
		}else{
			if (ht->t[pos].is_list == ht){
				return _list_remove(ht->t[pos].list, ht, key);
			}else{
				return NULL;
			}
		}
	}
	if (!strcmp(ht->getkey(*found), key)){
		ret = *found;
		*found = NULL;
		return ret;
	}else{
		return NULL;
	}
}

int ht8_iterate(struct ht8 *ht, int (*iter)(void *value, void *ctx), void *ctx)
{
	int ret = 0;
	assert(ht && iter);
	for (uint32_t i=0; i < ht->max; i++){
		if (ht->t[i].is_list != ht){
			if (ht->t[i].e1){
				if (ret = iter(ht->t[i].e1, ctx))
					return ret;
			}
			if (ht->t[i].e2){
				if (ret = iter(ht->t[i].e2, ctx))
					return ret;
			}
		}else{
			struct ht8_list *curr = ht->t[i].list;
			do{
				for (int i=0; i < _LIST_ENTRIES_N; i++){
					if (curr->entries[i]){
						if (ret = iter(ht->t[i].e1, ctx))
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
	struct ht8 *ht = (struct ht8*)ctx;
	if(ht8_put(ht, ht->getkey(value), value))
		return -1;
	return 0;
}
int ht8_copy(struct ht8 *dest, struct ht8 *src)
{
	assert(src && dest);
	return ht8_iterate(src, _copy_iter, dest);
}

int ht8_renew(struct ht8 *ht)
{
	assert(ht);
	struct ht8 *newht = ht8_create(ht->getkey);
	if (!newht)
		return -1;
	if (ht8_copy(newht, ht)){
		ht8_free(newht);
		return -1;
	}
	void *oldtable = ht->t;
	*ht = *newht;
	free(newht);
	free(oldtable);
	return 0;
}

void ht8_clean(struct ht8 *ht)
{
	assert(ht);
	for (uint32_t i=0; i < ht->max; i++){
		if (ht->t[i].is_list == ht)
			_list_free(ht->t[i].list);
	}
	memset(ht->t, 0, ((sizeof(struct ht8_bkt) << ht->max_lg2) / sizeof(unsigned char)));
	return;
}

int ht8_grow(struct ht8 *ht)
{
	assert(ht);
	struct ht8 *newht = _create_with_sizelog2(ht->max_lg2 + 1, ht->getkey);
	if (!newht)
		return -1;
	if (ht8_copy(newht, ht)){
		ht8_free(newht);
		return -1;
	}
	void *oldtable = ht->t;
	*ht = *newht;
	free(newht);
	free(oldtable);
	return 0;
}

void ht8_setfunc(struct ht8 *ht, const char *(*getkey)(void *value))
{
	assert(ht && getkey);
	ht->getkey = getkey;
	return;
}

uint32_t ht8_getnum(struct ht8 *ht)
{
	assert(ht);
	return ht->n_entries;
}

void ht8_free(struct ht8 *ht)
{
	assert(ht);
	for (uint32_t i=0; i < ht->max; i++){
		if (ht->t[i].is_list == ht)
			_list_free(ht->t[i].list);
	}
	free(ht->t);
	free(ht);
	return;
}
