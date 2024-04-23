HT8
===

Fast and simple hash table. It grows automaticly as the number of entries
increases and works with basicly every type of data.

The hash table stores simple `void*` values. To add or access a value, a key is
required, a NULL-terminated string. The keys themself are not stored in the
table, but are accessed via a user-defined function.

For simple use cases where the value is a pointer to a NULL-terminated string
or a structure with a NULL-terminated string as its first member, the
user-defined function can be omitted by passing *NULL* to `ht8_create`.


Installation
------------

* Use git submodule or subtree to add the library to your project
or
* Simply copy all files from `src/` into your project.


API
---

```c
struct ht8 *ht8_create(const char *(*getkey)(void *value));
```

Allocates memory for the hash table on the heap and initializes it.

`getkey` should accept a value from the table and return the corresponding key.
If `getkey` is *NULL*, a default function is used that assumes that the entries
in the table are pointers to NULL-terminated strings.

On success it returns a pointer to the hash table, otherwise it returns *NULL*.


```c
int ht8_put(struct ht8 *ht. void *value);
```

Inserts `value` into the hash table `ht`.

Returns 0 on success.


```c
void *ht8_get(struct ht8 *ht. const char *key);
```

If the corresponding value to the `key` exists, it returns the value, otherwise
it returns *NULL*.


```c
void *ht8_remove(struct ht8 *ht. const char *key);
```

If the corresponding value to the `key` exists, it returns the value and
removes it from the table, otherwise it returns *NULL*.


```c
int ht8_iterate(struct ht8 *ht, int (*iter)(void *value, void *ctx),
                void *ctx);
```

Calls `iter` for every value in the table and passes the additional argument
`ctx` for user-defined porposes.


```c
int ht8_copy(struct ht8 *dest, struct ht8 *src);
```

Copies all values from `src` to `dest`.

Returns 0 on success.


```c
int ht8_renew(struct ht8 *ht);
```

Recreates the table `ht`. This can be used to shrink the table after the number
of values has reduced.

On success it returns 0.


```c
void ht8_clean(struct ht8 *ht);
```

Removes all values from the table


```c
int ht8_grow(struct ht8 *ht);
```

Doubles the size of the table.

Returns 0 on success.


```c
void ht8_setfunc(struct ht8 *ht, const char *(*getkey)(void *value));
```

Sets `getkey` as the callback-function for the table `ht`.


```c
uint32_t ht8_getnum(struct ht8 *ht);
```

Returns the number of values in the table `ht`.


```c
void ht8_free(struct ht8 *ht);
```

Frees the allocated memory of the table `ht`.

Example
-------

```c
//TODO


```
