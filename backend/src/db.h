#ifndef DB_H
#define DB_H

#include <stddef.h>
#include "student.h"

#define DB_OK         0
#define DB_ERR       -1
#define DB_NOT_FOUND -2
#define DB_CONFLICT  -3

int  db_init(const char *path);
int  db_list(Student **out, size_t *count);
int  db_get(int id, Student *out);
int  db_insert(const Student *in, int *new_id);
int  db_update(int id, const Student *in);
int  db_delete(int id);
void db_close(void);

#endif
