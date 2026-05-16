#include "router.h"
#include "handlers.h"
#include <stdlib.h>
#include <string.h>

static int parse_id(const char *s, int *out) {
    if (!s || !*s) return 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (*end != '\0') return 0;
    if (v <= 0 || v > 2147483647L) return 0;
    *out = (int)v;
    return 1;
}

MhdResult route(struct MHD_Connection *conn, const char *method,
                const char *url, const char *body) {
    if (strcmp(method, "OPTIONS") == 0) {
        return send_cors_preflight(conn);
    }

    if (strcmp(url, "/students") == 0) {
        if (strcmp(method, "GET") == 0)  return handle_list_students(conn);
        if (strcmp(method, "POST") == 0) return handle_create_student(conn, body);
        return send_error(conn, 405, "method not allowed");
    }

    if (strncmp(url, "/students/", 10) == 0) {
        int id = 0;
        if (!parse_id(url + 10, &id)) {
            return send_error(conn, 400, "invalid id");
        }
        if (strcmp(method, "GET") == 0)    return handle_get_student(conn, id);
        if (strcmp(method, "PUT") == 0)    return handle_update_student(conn, id, body);
        if (strcmp(method, "DELETE") == 0) return handle_delete_student(conn, id);
        return send_error(conn, 405, "method not allowed");
    }

    return send_error(conn, 404, "not found");
}
