#include "db.h"
#include "http_util.h"
#include "router.h"
#include <microhttpd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_PORT 8080

static volatile sig_atomic_t g_stop = 0;

static void on_signal(int sig) {
    (void)sig;
    g_stop = 1;
}

static void log_request(const char *method, const char *url, unsigned int status) {
    time_t now = time(NULL);
    struct tm tm;
    gmtime_r(&now, &tm);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tm);
    printf("%s %s %s %u\n", ts, method, url, status);
    fflush(stdout);
}

static MhdResult access_handler(void *cls,
                                struct MHD_Connection *connection,
                                const char *url,
                                const char *method,
                                const char *version,
                                const char *upload_data,
                                size_t *upload_data_size,
                                void **req_cls) {
    (void)cls;
    (void)version;

    if (*req_cls == NULL) {
        BodyBuf *buf = malloc(sizeof(BodyBuf));
        if (!buf) return MHD_NO;
        body_init(buf);
        *req_cls = buf;
        return MHD_YES;
    }

    BodyBuf *buf = *req_cls;

    if (*upload_data_size > 0) {
        if (body_append(buf, upload_data, *upload_data_size) != 0) {
            return MHD_NO;
        }
        *upload_data_size = 0;
        return MHD_YES;
    }

    MhdResult r = route(connection, method, url, buf->data ? buf->data : "");

    const union MHD_ConnectionInfo *info = MHD_get_connection_info(
        connection, MHD_CONNECTION_INFO_HTTP_STATUS);
    unsigned int status = info ? (unsigned int)info->http_status : 0;
    log_request(method, url, status);

    return r;
}

static void request_completed(void *cls,
                              struct MHD_Connection *conn,
                              void **req_cls,
                              enum MHD_RequestTerminationCode toe) {
    (void)cls;
    (void)conn;
    (void)toe;
    if (*req_cls) {
        BodyBuf *buf = *req_cls;
        body_free(buf);
        free(buf);
        *req_cls = NULL;
    }
}

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "invalid port: %s\n", argv[1]);
            return 1;
        }
    }

    const char *db_path = getenv("STUDENTS_DB");
    if (!db_path || !*db_path) db_path = "students.db";

    if (db_init(db_path) != DB_OK) {
        fprintf(stderr, "db_init failed\n");
        return 1;
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        port,
        NULL, NULL,
        &access_handler, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
        MHD_OPTION_END);
    if (!daemon) {
        fprintf(stderr, "MHD_start_daemon failed\n");
        db_close();
        return 1;
    }

    printf("listening on :%d (db=%s)\n", port, db_path);
    fflush(stdout);

    while (!g_stop) {
        struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
        nanosleep(&ts, NULL);
    }

    printf("shutting down\n");
    MHD_stop_daemon(daemon);
    db_close();
    return 0;
}
