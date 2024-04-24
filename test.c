#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "tbl.h"

#define BUFSIZE 8

struct person{
	char name[BUFSIZE];
	char age[BUFSIZE];
};

int _should_stop(void)
{
	int option = 'y';
	if (fputs("Stop? (y/n): ", stdout) < 0)
		return 1;
	if ((option = getchar()) == EOF)
		return 1;
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	if (option == 'y')
		return 1;
	else
		return 0;
}

void test_put(struct tbl *t)
{
	struct person *p;
	while (1){
		p = malloc(sizeof(struct person));
		if (!p)
			break;
		if (fputs("Person created\n", stdout) < 0)
			break;
		if (fputs("name: ", stdout) < 0)
			break;
		if (fgets(p->name, BUFSIZE, stdin) < 0)
			break;
		if (fputs("age: ", stdout) < 0)
			break;
		if (fgets(p->age, BUFSIZE, stdin) < 0)
			break;
		if (tbl_put(t, p))
			break;
		if (fputs("Inserted person in table\n", stdout) < 0)
			break;
		if (_should_stop())
			return;
	}
	free(p);
}

void test_get(struct tbl *t)
{
	char buf[BUFSIZE];
	struct person *p;
	while (1){
		if (fputs("name: ", stdout) < 0)
			break;
		if (fgets(buf, BUFSIZE, stdin) < 0)
			break;
		if (!(p = tbl_get(t, buf)))
			break;
		if (fputs("age is ", stdout) < 0)
			break;
		if (fputs(p->age, stdout) < 0)
			break;
		if (_should_stop())
			return;
	}
}

void test_remove(struct tbl *t)
{
	char buf[BUFSIZE];
	while (1){
		struct person *p;
		if (fputs("name: ", stdout) < 0)
			break;
		if (fgets(buf, BUFSIZE, stdin) < 0)
			break;
		if (!(p = tbl_remove(t, buf)))
			break;
		if (fputs("Removed person from table\n", stdout) < 0)
			break;
		if (fputs("age is ", stdout) < 0)
			break;
		if (fputs(p->age, stdout) < 0)
			break;
		free(p);
		if (fputs("Person freed", stdout) < 0)
			break;
		if (_should_stop())
			return;
	}
}

void test_grow(struct tbl *t)
{
	return;
}

void test_getnum(struct tbl *t)
{
	printf("Number of entries: %i", (int)tbl_getnum(t));
	return;
}

void test_recreate(struct tbl *t)
{
	return;
}

void test_copy(struct tbl *t)
{
	return;
}

void test_iterate(struct tbl *t)
{
	return;
}

void test_clean(struct tbl *t)
{
	return;
}

void test_setfunc(struct tbl *t)
{
	return;
}

int main()
{
	struct tbl *t = tbl_create(NULL);
	char buf[BUFSIZE];
	if (!t)
		return -1;
	if (fputs("Table created\n", stdout) < 0)
		return -1;
	while (1) {
		if (fputs("> ", stdout) < 0)
			return -1;
		if (!fgets(buf, BUFSIZE, stdin))
			return -1;
		switch (buf[0]){
			case 'f':
				tbl_free(t);
				return 0;
			case 'p':
				test_put(t);
				break;
			case 'g':
				if (buf[1] == 'e'){
					if (buf[3] == 'n')
						test_getnum(t);
					else
						test_get(t);
				}else {
					test_grow(t);
				}
				break;
			case 'r':
				if (buf[2] == 'm')
					test_remove(t);
				else
					test_recreate(t);
				break;
			case 'i':
				test_iterate(t);
				break;
			case 'c':
				if (buf[1] == 'o')
					test_copy(t);
				else
					test_clean(t);
				break;
			case 's':
				test_setfunc(t);
				break;
		}
	}
}
