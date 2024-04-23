/* tbl: Fast and simple hash table
 * Copyright (c) 2024 Vakaris Girnius <vakaris@girnius.dev>
 *
 * This source code is licensed under the zlib license (found in the
 * LICENSE file in this repository)
*/

#ifndef TBL_H
#define TBL_H

#include <stdint.h>

#ifndef TBL_DEFAULT_BUCKETS_N
 #define TBL_DEFAULT_BUCKETS_N 8
#endif

#define TBL_ENTRIES_PER_BUCKET 2

#ifndef TBL_MIN_FREE_BUCKETS_RATIO 
 #define TBL_MIN_FREE_BUCKETS_RATIO 4
#endif

struct tbl;

struct tbl *tbl_create(const char *(*getkey)(void *value));

int tbl_put(struct tbl *t, void *value);
void *tbl_get(struct tbl *t, const char *key);
void *tbl_remove(struct tbl *t, const char *key);

int tbl_iterate(struct tbl *t, int (*iter)(void *value, void *ctx),
		void *ctx);
int tbl_copy(struct tbl *dest, struct tbl *src);
int tbl_renew(struct tbl *t);
void tbl_clean(struct tbl *t);
int tbl_grow(struct tbl *t);

void tbl_setfunc(struct tbl *t, const char *(*getkey)(void *value));
uint32_t tbl_getnum(struct tbl *t);

void tbl_free(struct tbl *t);

#endif /* tbl.h */
