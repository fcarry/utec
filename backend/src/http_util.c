#include "http_util.h"
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>

void body_init(BodyBuf *b) {
    b->data = NULL;
    b->size = 0;
    b->cap = 0;
}

int body_append(BodyBuf *b, const char *data, size_t size) {
    if (b->size + size + 1 > b->cap) {
        size_t newcap = b->cap == 0 ? 256 : b->cap * 2;
        while (newcap < b->size + size + 1) newcap *= 2;
        char *bigger = realloc(b->data, newcap);
        if (!bigger) return -1;
        b->data = bigger;
        b->cap = newcap;
    }
    memcpy(b->data + b->size, data, size);
    b->size += size;
    b->data[b->size] = '\0';
    return 0;
}

void body_free(BodyBuf *b) {
    free(b->data);
    b->data = NULL;
    b->size = 0;
    b->cap = 0;
}

static void add_cors_headers(struct MHD_Response *resp) {
    MHD_add_response_header(resp, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(resp, "Access-Control-Allow-Methods",
                            "GET, POST, PUT, DELETE, OPTIONS");
    MHD_add_response_header(resp, "Access-Control-Allow-Headers", "Content-Type");
}

MhdResult send_json(struct MHD_Connection *conn, unsigned int status, const char *json) {
    size_t len = strlen(json);
    char *copy = malloc(len);
    if (!copy) return MHD_NO;
    memcpy(copy, json, len);
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        len, copy, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(resp, "Content-Type", "application/json");
    add_cors_headers(resp);
    MhdResult r = MHD_queue_response(conn, status, resp);
    MHD_destroy_response(resp);
    return r;
}

MhdResult send_error(struct MHD_Connection *conn, unsigned int status, const char *message) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "error", message);
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    MhdResult r = send_json(conn, status, json);
    free(json);
    return r;
}

MhdResult send_no_content(struct MHD_Connection *conn) {
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        0, NULL, MHD_RESPMEM_PERSISTENT);
    add_cors_headers(resp);
    MhdResult r = MHD_queue_response(conn, 204, resp);
    MHD_destroy_response(resp);
    return r;
}

MhdResult send_cors_preflight(struct MHD_Connection *conn) {
    return send_no_content(conn);
}
