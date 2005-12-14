/*
 * Copyright (C) 2002,2003 Dizzy
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

#ifdef WITH_SQL_MYSQL
#include "common/setup_before.h"
#ifdef WIN32
#include <windows.h>
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif
#include <stdlib.h>
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "storage_sql.h"
#include "sql_mysql.h"
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

static int sql_mysql_init(const char *, const char *, const char *, const char *, const char *, const char *);
static int sql_mysql_close(void);
static t_sql_res * sql_mysql_query_res(const char *);
static int sql_mysql_query(const char *);
static t_sql_row * sql_mysql_fetch_row(t_sql_res *);
static void sql_mysql_free_result(t_sql_res *);
static unsigned int sql_mysql_num_rows(t_sql_res *);
static unsigned int sql_mysql_num_fields(t_sql_res *);
static unsigned int sql_mysql_affected_rows(void);
static t_sql_field * sql_mysql_fetch_fields(t_sql_res *);
static int sql_mysql_free_fields(t_sql_field *);
static void sql_mysql_escape_string(char *, const char *, int);

t_sql_engine sql_mysql = {
    sql_mysql_init,
    sql_mysql_close,
    sql_mysql_query_res,
    sql_mysql_query,
    sql_mysql_fetch_row,
    sql_mysql_free_result,
    sql_mysql_num_rows,
    sql_mysql_num_fields,
    sql_mysql_affected_rows,
    sql_mysql_fetch_fields,
    sql_mysql_free_fields,
    sql_mysql_escape_string
};

static MYSQL *mysql = NULL;

static int sql_mysql_init(const char *host, const char *port, const char *socket, const char *name, const char *user, const char *pass)
{
    if (name == NULL || user == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL parameter");
        return -1;
    }

    if ((mysql = mysql_init(NULL)) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got error from mysql_init");
        return -1;
    }

    if (mysql_real_connect(mysql, host, user, pass, name, port ? atoi(port) : 0, socket, CLIENT_FOUND_ROWS) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "error connecting to database (db said: '%s')", mysql_error(mysql));
	mysql_close(mysql);
        return -1;
    }

    return 0;
}

static int sql_mysql_close(void)
{
    if (mysql) {
	mysql_close(mysql);
	mysql = NULL;
    }

    return 0;
}

static t_sql_res * sql_mysql_query_res(const char * query)
{
    t_sql_res *res;

    if (mysql == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "mysql driver not initilized");
        return NULL;
    }

    if (query == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL query");
        return NULL;
    }

    if (mysql_query(mysql, query)) {
//        eventlog(eventlog_level_debug, __FUNCTION__, "got error from query (%s)", query);
	return NULL;
    }

    res = mysql_store_result(mysql);
    if (res == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got error from store result");
        return NULL;
    }

    return res;
}

static int sql_mysql_query(const char * query)
{
    if (mysql == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "mysql driver not initilized");
        return -1;
    }

    if (query == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL query");
        return -1;
    }

    return mysql_query(mysql, query) == 0 ? 0 : -1;
}

static t_sql_row * sql_mysql_fetch_row(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return NULL;
    }

    return mysql_fetch_row((MYSQL_RES *)result);
}

static void sql_mysql_free_result(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return;
    }

    mysql_free_result((MYSQL_RES *)result);
}

static unsigned int sql_mysql_num_rows(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return 0;
    }

    return mysql_num_rows((MYSQL_RES *)result);
}

static unsigned int sql_mysql_num_fields(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return 0;
    }

    return mysql_num_fields((MYSQL_RES *)result);
}

static unsigned int sql_mysql_affected_rows(void)
{
    return mysql_affected_rows(mysql);
}

static t_sql_field * sql_mysql_fetch_fields(t_sql_res *result)
{
    MYSQL_FIELD *fields;
    unsigned fieldno, i;
    t_sql_field *rfields;

    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return NULL;
    }

    fieldno = mysql_num_fields((MYSQL_RES *)result);
    fields = mysql_fetch_fields((MYSQL_RES *)result);

    rfields = (t_sql_field *)xmalloc(sizeof(t_sql_field) * (fieldno + 1));
    for(i = 0; i < fieldno; i++)
	rfields[i] = fields[i].name;
    rfields[i] = NULL;

    return rfields;
}

static int sql_mysql_free_fields(t_sql_field *fields)
{
    if (fields == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL fields");
	return -1;
    }

    xfree((void*)fields);
    return 0; /* mysql_free_result() should free the rest properly */
}

static void sql_mysql_escape_string(char *escape, const char *from, int len)
{
    if (mysql == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "mysql driver not initilized");
	return;
    }
    mysql_real_escape_string(mysql, escape, from, len);
}

}

}

#endif /* WITH_SQL_MYSQL */
