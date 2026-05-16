#include "student.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

cJSON *student_to_json(const Student *s) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "id", s->id);
    cJSON_AddStringToObject(json, "name", s->name);
    cJSON_AddStringToObject(json, "id_doc", s->id_doc);
    cJSON_AddStringToObject(json, "email", s->email);
    return json;
}

static int validate_name(const char *name, char *err, size_t err_size) {
    size_t len = strlen(name);
    if (len < 1 || len > 100) {
        snprintf(err, err_size, "name length must be 1-100");
        return 0;
    }
    return 1;
}

static int validate_id_doc(const char *id_doc, char *err, size_t err_size) {
    size_t len = strlen(id_doc);
    if (len < 7 || len > 10) {
        snprintf(err, err_size, "id_doc length must be 7-10");
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)id_doc[i])) {
            snprintf(err, err_size, "id_doc must contain only digits");
            return 0;
        }
    }
    return 1;
}

static int validate_email(const char *email, char *err, size_t err_size) {
    size_t len = strlen(email);
    if (len < 3 || len > 120) {
        snprintf(err, err_size, "email length must be 3-120");
        return 0;
    }
    const char *at = strchr(email, '@');
    if (!at || at == email) {
        snprintf(err, err_size, "email must contain @");
        return 0;
    }
    const char *dot = strchr(at, '.');
    if (!dot || dot == at + 1 || *(dot + 1) == '\0') {
        snprintf(err, err_size, "email must contain . after @");
        return 0;
    }
    return 1;
}

int student_from_json(const cJSON *json, Student *out, char *err, size_t err_size) {
    if (!cJSON_IsObject(json)) {
        snprintf(err, err_size, "body must be a JSON object");
        return 0;
    }

    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    cJSON *id_doc = cJSON_GetObjectItemCaseSensitive(json, "id_doc");
    cJSON *email = cJSON_GetObjectItemCaseSensitive(json, "email");

    if (!cJSON_IsString(name) || !name->valuestring) {
        snprintf(err, err_size, "name is required");
        return 0;
    }
    if (!cJSON_IsString(id_doc) || !id_doc->valuestring) {
        snprintf(err, err_size, "id_doc is required");
        return 0;
    }
    if (!cJSON_IsString(email) || !email->valuestring) {
        snprintf(err, err_size, "email is required");
        return 0;
    }

    if (strlen(name->valuestring) >= sizeof(out->name) ||
        strlen(id_doc->valuestring) >= sizeof(out->id_doc) ||
        strlen(email->valuestring) >= sizeof(out->email)) {
        snprintf(err, err_size, "field too long");
        return 0;
    }

    strncpy(out->name, name->valuestring, sizeof(out->name) - 1);
    out->name[sizeof(out->name) - 1] = '\0';
    strncpy(out->id_doc, id_doc->valuestring, sizeof(out->id_doc) - 1);
    out->id_doc[sizeof(out->id_doc) - 1] = '\0';
    strncpy(out->email, email->valuestring, sizeof(out->email) - 1);
    out->email[sizeof(out->email) - 1] = '\0';

    if (!validate_name(out->name, err, err_size)) return 0;
    if (!validate_id_doc(out->id_doc, err, err_size)) return 0;
    if (!validate_email(out->email, err, err_size)) return 0;

    return 1;
}
