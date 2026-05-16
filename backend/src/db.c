#include "db.h"
#include <pthread.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3 *g_db = NULL;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static const char *DDL =
    "CREATE TABLE IF NOT EXISTS students ("
    "    id      INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    name    TEXT NOT NULL,"
    "    id_doc  TEXT NOT NULL UNIQUE,"
    "    email   TEXT NOT NULL"
    ");";

int db_init(const char *path) {
    if (sqlite3_open(path, &g_db) != SQLITE_OK) {
        fprintf(stderr, "sqlite3_open failed: %s\n", sqlite3_errmsg(g_db));
        return DB_ERR;
    }
    char *err = NULL;
    if (sqlite3_exec(g_db, DDL, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "DDL failed: %s\n", err);
        sqlite3_free(err);
        return DB_ERR;
    }
    return DB_OK;
}

void db_close(void) {
    pthread_mutex_lock(&g_lock);
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    pthread_mutex_unlock(&g_lock);
}

static void row_to_student(sqlite3_stmt *stmt, Student *s) {
    s->id = sqlite3_column_int(stmt, 0);
    const unsigned char *name = sqlite3_column_text(stmt, 1);
    const unsigned char *id_doc = sqlite3_column_text(stmt, 2);
    const unsigned char *email = sqlite3_column_text(stmt, 3);
    snprintf(s->name, sizeof(s->name), "%s", name ? (const char *)name : "");
    snprintf(s->id_doc, sizeof(s->id_doc), "%s", id_doc ? (const char *)id_doc : "");
    snprintf(s->email, sizeof(s->email), "%s", email ? (const char *)email : "");
}

int db_list(Student **out, size_t *count) {
    *out = NULL;
    *count = 0;

    pthread_mutex_lock(&g_lock);
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db,
        "SELECT id, name, id_doc, email FROM students ORDER BY id",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_lock);
        return DB_ERR;
    }

    size_t cap = 16;
    Student *arr = malloc(cap * sizeof(Student));
    if (!arr) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_lock);
        return DB_ERR;
    }
    size_t n = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (n == cap) {
            cap *= 2;
            Student *bigger = realloc(arr, cap * sizeof(Student));
            if (!bigger) {
                free(arr);
                sqlite3_finalize(stmt);
                pthread_mutex_unlock(&g_lock);
                return DB_ERR;
            }
            arr = bigger;
        }
        row_to_student(stmt, &arr[n]);
        n++;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_lock);

    *out = arr;
    *count = n;
    return DB_OK;
}

int db_get(int id, Student *out) {
    pthread_mutex_lock(&g_lock);
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db,
        "SELECT id, name, id_doc, email FROM students WHERE id = ?",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_lock);
        return DB_ERR;
    }
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    int result;
    if (rc == SQLITE_ROW) {
        row_to_student(stmt, out);
        result = DB_OK;
    } else if (rc == SQLITE_DONE) {
        result = DB_NOT_FOUND;
    } else {
        result = DB_ERR;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_lock);
    return result;
}

int db_insert(const Student *in, int *new_id) {
    pthread_mutex_lock(&g_lock);
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db,
        "INSERT INTO students (name, id_doc, email) VALUES (?, ?, ?)",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_lock);
        return DB_ERR;
    }
    sqlite3_bind_text(stmt, 1, in->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, in->id_doc, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, in->email, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    int result;
    if (rc == SQLITE_DONE) {
        *new_id = (int)sqlite3_last_insert_rowid(g_db);
        result = DB_OK;
    } else if (rc == SQLITE_CONSTRAINT) {
        result = DB_CONFLICT;
    } else {
        result = DB_ERR;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_lock);
    return result;
}

int db_update(int id, const Student *in) {
    pthread_mutex_lock(&g_lock);

    sqlite3_stmt *check = NULL;
    int rc = sqlite3_prepare_v2(g_db,
        "SELECT 1 FROM students WHERE id = ?", -1, &check, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_lock);
        return DB_ERR;
    }
    sqlite3_bind_int(check, 1, id);
    rc = sqlite3_step(check);
    sqlite3_finalize(check);
    if (rc == SQLITE_DONE) {
        pthread_mutex_unlock(&g_lock);
        return DB_NOT_FOUND;
    }
    if (rc != SQLITE_ROW) {
        pthread_mutex_unlock(&g_lock);
        return DB_ERR;
    }

    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(g_db,
        "UPDATE students SET name = ?, id_doc = ?, email = ? WHERE id = ?",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_lock);
        return DB_ERR;
    }
    sqlite3_bind_text(stmt, 1, in->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, in->id_doc, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, in->email, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, id);
    rc = sqlite3_step(stmt);
    int result;
    if (rc == SQLITE_DONE) {
        result = DB_OK;
    } else if (rc == SQLITE_CONSTRAINT) {
        result = DB_CONFLICT;
    } else {
        result = DB_ERR;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_lock);
    return result;
}

int db_delete(int id) {
    pthread_mutex_lock(&g_lock);
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db,
        "DELETE FROM students WHERE id = ?", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_lock);
        return DB_ERR;
    }
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    int result;
    if (rc == SQLITE_DONE) {
        result = (sqlite3_changes(g_db) > 0) ? DB_OK : DB_NOT_FOUND;
    } else {
        result = DB_ERR;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_lock);
    return result;
}
