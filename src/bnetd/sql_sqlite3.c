/*
 * Copyright (C) 2005 Dizzy 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef WITH_SQL_SQLITE3
#include "common/setup_before.h"
#ifdef HAVE_SQLITE3_H
# include <sqlite3.h>
#endif
#include <stdlib.h>
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "storage_sql.h"
#include "sql_sqlite3.h"
#include "common/setup_after.h"

typedef struct {
    char** results;
    int rows;
    int columns;
    int crow;
} t_sqlite3_res;

static int sql_sqlite3_init(const char *, const char *, const char *, const char *, const char *, const char *);
static int sql_sqlite3_close(void);
static t_sql_res * sql_sqlite3_query_res(const char *);
static int sql_sqlite3_query(const char *);
static t_sql_row * sql_sqlite3_fetch_row(t_sql_res *);
static void sql_sqlite3_free_result(t_sql_res *);
static unsigned int sql_sqlite3_num_rows(t_sql_res *);
static unsigned int sql_sqlite3_num_fields(t_sql_res *);
static unsigned int sql_sqlite3_affected_rows(void);
static t_sql_field * sql_sqlite3_fetch_fields(t_sql_res *);
static int sql_sqlite3_free_fields(t_sql_field *);
static void sql_sqlite3_escape_string(char *, const char *, int);

t_sql_engine sql_sqlite3 = {
    sql_sqlite3_init,
    sql_sqlite3_close,
    sql_sqlite3_query_res,
    sql_sqlite3_query,
    sql_sqlite3_fetch_row,
    sql_sqlite3_free_result,
    sql_sqlite3_num_rows,
    sql_sqlite3_num_fields,
    sql_sqlite3_affected_rows,
    sql_sqlite3_fetch_fields,
    sql_sqlite3_free_fields,
    sql_sqlite3_escape_string
};

static sqlite3 *db = NULL;

static int sql_sqlite3_init(const char *host, const char *port, const char *socket, const char *name, const char *user, const char *pass)
{
    /* SQLite3 has no host, port, socket, user or password and the database name is the path to the db file */
    if (sqlite3_open(name, &db) != SQLITE_OK) {
        eventlog(eventlog_level_error, __FUNCTION__, "got error from sqlite3_open (%s)", sqlite3_errmsg(db));
	sqlite3_close(db);
        return -1;
    }

    return 0;
}

static int sql_sqlite3_close(void)
{
    if (db) {
	if (sqlite3_close(db) != SQLITE_OK) {
    	    eventlog(eventlog_level_error, __FUNCTION__, "got error from sqlite3_close (%s)", sqlite3_errmsg(db));
    	    return -1;
	}
	db = NULL;
    }

    return 0;
}

static t_sql_res * sql_sqlite3_query_res(const char* query)
{
    t_sqlite3_res *res;

    if (db == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "sqlite3 driver not initilized");
        return NULL;
    }

    if (query == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL query");
        return NULL;
    }

    res = xmalloc(sizeof(t_sqlite3_res));

    if (sqlite3_get_table(db, query, &res->results, &res->rows, &res->columns, NULL) != SQLITE_OK) {
/*        eventlog(eventlog_level_debug, __FUNCTION__, "got error (%s) from query (%s)", sqlite3_errmsg(db), query); */
	xfree((void*)res);
	return NULL;
    }

    res->crow = 0;

    return res;
}

static int sql_sqlite3_query(const char* query)
{
    if (db == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "sqlite3 driver not initilized");
        return -1;
    }

    if (query == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL query");
        return -1;
    }

    return sqlite3_exec(db, query, NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

static t_sql_row * sql_sqlite3_fetch_row(t_sql_res *result)
{
    t_sqlite3_res *res = (t_sqlite3_res*)result;

    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return NULL;
    }

    return res->crow < res->rows ? (res->results + res->columns * (++res->crow)) : NULL;
}

static void sql_sqlite3_free_result(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return;
    }

    sqlite3_free_table(((t_sqlite3_res *)result)->results);
    xfree(result);
}

static unsigned int sql_sqlite3_num_rows(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return 0;
    }

    return ((t_sqlite3_res *)result)->rows;
}

static unsigned int sql_sqlite3_num_fields(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return 0;
    }

    return ((t_sqlite3_res *)result)->columns;
}

static unsigned int sql_sqlite3_affected_rows(void)
{
    return sqlite3_changes(db);
}

static t_sql_field * sql_sqlite3_fetch_fields(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return NULL;
    }

    /* the first "row" is actually the field names */
    return ((t_sqlite3_res*)result)->results;
}

static int sql_sqlite3_free_fields(t_sql_field *fields)
{
    if (fields == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL fields");
	return -1;
    }

    return 0; /* sqlite3_free_table() should free it properly */
}

static void sql_sqlite3_escape_string(char *escape, const char *from, int len)
{
    if (db == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "sqlite3 driver not initilized");
	return;
    }
    sqlite3_snprintf(len * 2 + 1, escape, "%q", from);
}

#endif /* WITH_SQL_SQLITE3 */
