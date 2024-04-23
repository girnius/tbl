/* ht8: Fast and simple hash table
 * Copyright (c) 2024 Vakaris Girnius <vakaris@girnius.dev>
 *
 * This source code is licensed under the zlib license (found in the
 * LICENSE file in this repository)
*/

#ifndef HT8_H
#define HT8_H

#include <stdint.h>

#ifndef HT8_DEFAULT_BUCKETS_N
 #define HT8_DEFAULT_BUCKETS_N 8
#endif

#define HT8_ENTRIES_PER_BUCKET 2

#ifndef HT8_MIN_FREE_BUCKETS_RATIO 
 #define HT8_MIN_FREE_BUCKETS_RATIO 4
#endif

struct ht8;

struct ht8 *ht8_create(const char *(*getkey)(void *value));

int ht8_put(struct ht8 *ht, void *value);
void *ht8_get(struct ht8 *ht, const char *key);
void *ht8_remove(struct ht8 *ht, const char *key);

int ht8_iterate(struct ht8 *ht, int (*iter)(void *value, void *ctx), void *ctx);
int ht8_copy(struct ht8 *dest, struct ht8 *src);
int ht8_renew(struct ht8 *ht);
void ht8_clean(struct ht8 *ht);
int ht8_grow(struct ht8 *ht);

void ht8_setfunc(struct ht8 *ht, const char *(*getkey)(void *value));
uint32_t ht8_getnum(struct ht8 *ht);

void ht8_free(struct ht8 *ht);

#endif /* ht8.h */
