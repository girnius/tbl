/* tbl: Dynamic hash table
 * -----------------------
 * Copyright (c) 2024 Vakaris Girnius <vakaris@girnius.dev>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tbl.h"

#define XXH_INLINE_ALL 1
#define XXH_NO_STREAM 1
#include "xxhash.h"

static inline void _init(struct tbl *t, struct tbl_bkt *array, unsigned short n_lg2, unsigned short keylen)
{
	assert(t && array && n_lg2 && keylen);
	if (t->n)
		memset(array, 0, sizeof(struct tbl_bkt) << n_lg2);
	if (!t->seed)
		t->seed = (unsigned long)t ^ keylen;
	else
		t->seed++;
	t->a = array;
	t->n = 0;
	t->max = 1 << n_lg2;
	t->max_lg2 = n_lg2;
	t->keylen = keylen;
	t->hashmask = ~(ULONG_MAX << n_lg2);
	return;
}

static inline int _put(struct tbl *t, void *value)
{
	assert(t && value);
	unsigned int hash = (unsigned int)XXH3_64bits_withSeed((char*)value, t->keylen, t->seed);
	unsigned int pos = hash & t->hashmask;
	unsigned int off = 0;
	if (t->n == t->max)
		return -1;
	while (1){
		if (!t->a[pos].value)
			break;
		pos = (pos+1) & t->hashmask;
		++off;
	}
	t->a[pos].value = value;
	t->a[pos].hash = hash;
	if (off > t->a[pos].maxoff)
		t->a[pos].maxoff = off;
	t->n++;
	return 0;
}

static inline void *_get(struct tbl *t, const char *key)
{
	assert(t && key);
	unsigned int hash = (unsigned int)XXH3_64bits_withSeed(key, t->keylen, t->seed);
	unsigned int pos = hash & t->hashmask;

	for (unsigned int off=t->a[pos].maxoff; off >= 0; off--){
		if (t->a[pos].value && hash == t->a[pos].hash){
			if (!(memcmp(key, t->a[pos].value, t->keylen)))
				return t->a[pos].value;
		}
		pos = (pos+1) & t->hashmask;
	}
	return NULL;
}

static inline void *_remove(struct tbl *t, const char *key)
{
	assert(t && key);
	unsigned int hash = (unsigned int)XXH3_64bits_withSeed(key, t->keylen, t->seed);
	unsigned int pos = hash & t->hashmask;
	void *found;

	for (unsigned int off=t->a[pos].maxoff; off >= 0; off--){
		if (t->a[pos].value && hash == t->a[pos].hash){
			if (!(memcmp(key, t->a[pos].value, t->keylen))){
				found = t->a[pos].value;
				t->a[pos].value = NULL;
				t->n--;
				return found;
			}
		}
		pos = (pos+1) & t->hashmask;
	}
	return NULL;
}

static inline void _copy(struct tbl *dest, struct tbl *src)
{
	assert(src && dest);
	assert(dest->max >= src->max);
	for (unsigned int i=0; i != src->max; i++){
		if (src->a[i].value)
			_put(dest, src->a[i].value);
	}
	return;
}

struct tbl *tbl_create(unsigned short keylen)
{
	assert(keylen);
	struct tbl *t = malloc(sizeof(struct tbl));
	if (!t)
		return NULL;
	struct tbl_bkt *array = malloc(sizeof(struct tbl_bkt) * TBL_DEFAULT_SIZE);
	if (!array){
		free(t);
		return NULL;
	}
	_init(t, array, TBL_DEFAULT_SIZE_LG2, keylen);
	return t;
}

int tbl_put(struct tbl *t, void *value)
{
	assert(t && value);
	if (t->max >> TBL_FREE_BUCKET_RATIO_LG2 != t->max - t->n){
		return _put(t, value);
	}else{
		if (tbl_grow(t) && _put(t, value))
			return -1;
	}
}

void *tbl_get(struct tbl *t, const char *key)
{
	return _get(t, key);
}

void *tbl_remove(struct tbl *t, const char *key)
{
	return _remove(t, key);
}

int tbl_grow(struct tbl *t)
{
	assert(t);
	struct tbl old_t;
	struct tbl_bkt *array = malloc(sizeof(struct tbl_bkt) << (t->max_lg2 + 1));
	if (!array){
		return -1;
	}
	memcpy(&old_t, t, sizeof(struct tbl));
	memset(t, 0, sizeof(struct tbl));
	_init(t, array, (t->max_lg2 + 1), t->keylen);
	_copy(t, &old_t);
	free(old_t.a);
	return 0;
}

int tbl_copy(struct tbl *dest, struct tbl *src)
{
	assert(dest && src);
	while (dest->max < src->max){
		if (tbl_grow(dest))
			return -1;
	}
	_copy(dest, src);
	return 0;
}

void tbl_free(struct tbl *t)
{
	free(t->a);
	free(t);
	return;
}
