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

#ifndef TBL_H
#define TBL_H

#include <stdint.h>

struct tbl_bkt{
	void *value;
	uint32_t hash;
	uint32_t maxoff;
};

struct tbl{
        uint64_t seed;
        uint32_t n;
        uint32_t max;
        uint16_t max_lg2;
        uint16_t keylen;
        uint32_t hashmask;
        struct tbl_bkt a[];
};

#define TBL_MAX UINT32_MAX
#define TBL_MAX_LOG2 32

void tbl_init(struct tbl *t, uint16_t n_lg2, uint16_t keylen);

int tbl_put(struct tbl *t, void *value);
void *tbl_get(struct tbl *t, const char *key);
void *tbl_remove(struct tbl *t, const char *key);

void tbl_copy(struct tbl *dest, struct tbl *src);

#endif /* tbl.h */
