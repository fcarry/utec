#ifndef STUDENT_H
#define STUDENT_H

#include <cjson/cJSON.h>

typedef struct {
    int  id;
    char name[101];
    char id_doc[16];
    char email[121];
} Student;

cJSON *student_to_json(const Student *s);
int    student_from_json(const cJSON *json, Student *out, char *err, size_t err_size);

#endif
