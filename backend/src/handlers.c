#include "handlers.h"
#include "db.h"
#include "student.h"
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>

MhdResult handle_list_students(struct MHD_Connection *conn) {
    Student *arr = NULL;
    size_t n = 0;
    if (db_list(&arr, &n) != DB_OK) {
        return send_error(conn, 500, "database error");
    }
    cJSON *root = cJSON_CreateArray();
    for (size_t i = 0; i < n; i++) {
        cJSON_AddItemToArray(root, student_to_json(&arr[i]));
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    free(arr);
    MhdResult r = send_json(conn, 200, json);
    free(json);
    return r;
}

MhdResult handle_get_student(struct MHD_Connection *conn, int id) {
    Student s;
    int rc = db_get(id, &s);
    if (rc == DB_NOT_FOUND) {
        return send_error(conn, 404, "student not found");
    }
    if (rc != DB_OK) {
        return send_error(conn, 500, "database error");
    }
    cJSON *json = student_to_json(&s);
    char *out = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    MhdResult r = send_json(conn, 200, out);
    free(out);
    return r;
}

static int parse_body(struct MHD_Connection *conn, const char *body, Student *out, MhdResult *err_resp) {
    if (!body || !*body) {
        *err_resp = send_error(conn, 400, "empty body");
        return 0;
    }
    cJSON *json = cJSON_Parse(body);
    if (!json) {
        *err_resp = send_error(conn, 400, "invalid JSON");
        return 0;
    }
    char err[128];
    if (!student_from_json(json, out, err, sizeof(err))) {
        cJSON_Delete(json);
        *err_resp = send_error(conn, 400, err);
        return 0;
    }
    cJSON_Delete(json);
    return 1;
}

MhdResult handle_create_student(struct MHD_Connection *conn, const char *body) {
    Student s;
    MhdResult err_resp;
    if (!parse_body(conn, body, &s, &err_resp)) return err_resp;

    int new_id = 0;
    int rc = db_insert(&s, &new_id);
    if (rc == DB_CONFLICT) {
        return send_error(conn, 409, "id_doc already exists");
    }
    if (rc != DB_OK) {
        return send_error(conn, 500, "database error");
    }
    s.id = new_id;
    cJSON *json = student_to_json(&s);
    char *out = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    MhdResult r = send_json(conn, 201, out);
    free(out);
    return r;
}

MhdResult handle_update_student(struct MHD_Connection *conn, int id, const char *body) {
    Student s;
    MhdResult err_resp;
    if (!parse_body(conn, body, &s, &err_resp)) return err_resp;

    int rc = db_update(id, &s);
    if (rc == DB_NOT_FOUND) {
        return send_error(conn, 404, "student not found");
    }
    if (rc == DB_CONFLICT) {
        return send_error(conn, 409, "id_doc already exists");
    }
    if (rc != DB_OK) {
        return send_error(conn, 500, "database error");
    }
    s.id = id;
    cJSON *json = student_to_json(&s);
    char *out = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    MhdResult r = send_json(conn, 200, out);
    free(out);
    return r;
}

MhdResult handle_delete_student(struct MHD_Connection *conn, int id) {
    int rc = db_delete(id);
    if (rc == DB_NOT_FOUND) {
        return send_error(conn, 404, "student not found");
    }
    if (rc != DB_OK) {
        return send_error(conn, 500, "database error");
    }
    return send_no_content(conn);
}
