/* tbl: Fast and simple hash table
 * Copyright (c) 2024 Vakaris Girnius <vakaris@girnius.dev>
 *
 * This source code is licensed under the zlib license (found in the
 * LICENSE file in this repository)
*/

#define _LIST_ENTRIES_N 4

struct tbl_list{
	void *entries[_LIST_ENTRIES_N];
	struct tbl_list *next;
};

static struct tbl_list *_list_create(void)
{
	return calloc(0, sizeof(struct tbl_list));
}

static int _list_add(struct tbl_list *l, void *value)
{
	for (int i=0; i < _LIST_ENTRIES_N; i++){
		if (!l->entries[i]){
			l->entries[i] = value;
			return 0;
		}
	}
	struct tbl_list *newlist = _list_create();
	if (!newlist)
		return -1;
	l->next = newlist;
	newlist->entries[0] = value;
	return 0;
}

static void *_list_get(struct tbl_list *l, const char *(*getkey)(void *value),
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

static void *_list_remove(struct tbl_list *l,const char *(*getkey)(void *value),
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
