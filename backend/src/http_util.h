#ifndef HTTP_UTIL_H
#define HTTP_UTIL_H

#include <microhttpd.h>
#include <stddef.h>

#if MHD_VERSION >= 0x00097002
typedef enum MHD_Result MhdResult;
#else
typedef int MhdResult;
#endif

typedef struct {
    char  *data;
    size_t size;
    size_t cap;
} BodyBuf;

void body_init(BodyBuf *b);
int  body_append(BodyBuf *b, const char *data, size_t size);
void body_free(BodyBuf *b);

MhdResult send_json(struct MHD_Connection *conn, unsigned int status, const char *json);
MhdResult send_error(struct MHD_Connection *conn, unsigned int status, const char *message);
MhdResult send_no_content(struct MHD_Connection *conn);
MhdResult send_cors_preflight(struct MHD_Connection *conn);

#endif
