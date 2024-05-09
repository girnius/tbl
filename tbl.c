/* tbl: Fast and simple hash table
 * -------------------------------
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

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "tbl.h"

#define XXH_INLINE_ALL 1
#define XXH_NO_STREAM 1
#include "xxhash.h"

void tbl_init(struct tbl *t, uint16_t n_lg2, uint16_t keylen)
{
	assert(t && n_lg2 && keylen);
	if (t->n)
		memset(t->a, 0, sizeof(struct tbl_bkt) << n_lg2);
	if (!t->seed)
		t->seed = (uint64_t)t ^ keylen;
	else
		t->seed++;
	t->n = 0;
	t->max = 1 << n_lg2;
	t->max_lg2 = n_lg2;
	t->keylen = keylen;
	t->hashmask = ~(UINT32_MAX << n_lg2);
	return;
}

int tbl_put(struct tbl *t, void *value)
{
	assert(t && value);
	uint32_t hash = (uint32_t)XXH64((char*)value, t->keylen, t->seed);
	uint32_t pos = hash & t->hashmask;
	uint32_t off = 0;
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

void *tbl_get(struct tbl *t, const char *key)
{
	assert(t && key);
	uint32_t hash = (uint32_t)XXH64(key, t->keylen, t->seed);
	uint32_t pos = hash & t->hashmask;

	for (uint32_t off=t->a[pos].maxoff; off >= 0; off--){
		if (t->a[pos].value && hash == t->a[pos].hash){
			if (!(memcmp(key, t->a[pos].value, t->keylen)))
				return t->a[pos].value;
		}
		pos = (pos+1) & t->hashmask;
	}
	return NULL;
}

void *tbl_remove(struct tbl *t, const char *key)
{
	assert(t && key);
	uint32_t hash = (uint32_t)XXH64(key, t->keylen, t->seed);
	uint32_t pos = hash & t->hashmask;
	void *found;

	for (uint32_t off=t->a[pos].maxoff; off >= 0; off--){
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

void tbl_copy(struct tbl *dest, struct tbl *src)
{
	assert(src && dest);
	assert(dest->max >= src->max);
	for (uint32_t i=0; i != src->max; i++){
		if (src->a[i].value)
			assert(tbl_put(dest, src->a[i].value) == 0);
	}
	return;
}
