#ifndef ROUTER_H
#define ROUTER_H

#include "http_util.h"

MhdResult route(struct MHD_Connection *conn, const char *method,
                const char *url, const char *body);

#endif
