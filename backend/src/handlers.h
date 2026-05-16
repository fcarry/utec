#ifndef HANDLERS_H
#define HANDLERS_H

#include "http_util.h"

MhdResult handle_list_students(struct MHD_Connection *conn);
MhdResult handle_get_student(struct MHD_Connection *conn, int id);
MhdResult handle_create_student(struct MHD_Connection *conn, const char *body);
MhdResult handle_update_student(struct MHD_Connection *conn, int id, const char *body);
MhdResult handle_delete_student(struct MHD_Connection *conn, int id);

#endif
