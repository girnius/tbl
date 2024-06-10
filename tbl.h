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

#ifndef TBL_H
#define TBL_H

#include <limits.h>

#define TBL_VERSION_STR "0.2"

#ifndef TBL_DEFAULT_SIZE
#define TBL_DEFAULT_SIZE 8
#define TBL_DEFAULT_SIZE_LG2 3
#endif

#ifndef TBL_FREE_BUCKET_RATIO
#define TBL_FREE_BUCKET_RATIO 4
#define TBL_FREE_BUCKET_RATIO_LG2 2
#endif

#define TBL_MAX ULONG_MAX

struct tbl_bkt{
	void *value;
	unsigned int hash;
	unsigned int maxoff;
};

struct tbl{
        struct tbl_bkt *a;
        unsigned long seed;
        unsigned int n;
        unsigned int max;
        unsigned short max_lg2;
        unsigned short keylen;
        unsigned int hashmask;
};

struct tbl *tbl_create(unsigned short keylen);

int tbl_put(struct tbl *t, void *value);
void *tbl_get(struct tbl *t, const char *key);
void *tbl_remove(struct tbl *t, const char *key);

int tbl_grow(struct tbl *t);
int tbl_copy(struct tbl *dest, struct tbl *src);

void tbl_free(struct tbl *t);

#endif /* tbl.h */
