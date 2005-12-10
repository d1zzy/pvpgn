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
#ifdef WITH_SQL_PGSQL

#include "common/setup_before.h"
#include <libpq-fe.h>
#include <stdlib.h>
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "storage_sql.h"
#include "sql_pgsql.h"
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

static int sql_pgsql_init(const char *, const char *, const char *, const char *, const char *, const char *);
static int sql_pgsql_close(void);
static t_sql_res * sql_pgsql_query_res(const char *);
static int sql_pgsql_query(const char *);
static t_sql_row * sql_pgsql_fetch_row(t_sql_res *);
static void sql_pgsql_free_result(t_sql_res *);
static unsigned int sql_pgsql_num_rows(t_sql_res *);
static unsigned int sql_pgsql_num_fields(t_sql_res *);
static unsigned int sql_pgsql_affected_rows(void);
static t_sql_field * sql_pgsql_fetch_fields(t_sql_res *);
static int sql_pgsql_free_fields(t_sql_field *);
static void sql_pgsql_escape_string(char *, const char *, int);

t_sql_engine sql_pgsql = {
    sql_pgsql_init,
    sql_pgsql_close,
    sql_pgsql_query_res,
    sql_pgsql_query,
    sql_pgsql_fetch_row,
    sql_pgsql_free_result,
    sql_pgsql_num_rows,
    sql_pgsql_num_fields,
    sql_pgsql_affected_rows,
    sql_pgsql_fetch_fields,
    sql_pgsql_free_fields,
    sql_pgsql_escape_string
};

static PGconn *pgsql = NULL;
static unsigned int lastarows = 0;

typedef struct {
    int crow;
    char ** rowbuf;
    PGresult *pgres;
} t_pgsql_res;

static int sql_pgsql_init(const char *host, const char *port, const char *socket, const char *name, const char *user, const char *pass)
{
    const char *tmphost;

    if (name == NULL || user == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL parameter");
        return -1;
    }

    tmphost = host != NULL ? host : socket;

    if ((pgsql = PQsetdbLogin(host, port, NULL, NULL, name, user, pass)) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "not enougn memory for new pgsql connection");
        return -1;
    }

    if (PQstatus(pgsql) != CONNECTION_OK) {
        eventlog(eventlog_level_error, __FUNCTION__, "error connecting to database (db said: '%s')", PQerrorMessage(pgsql));
	PQfinish(pgsql);
	pgsql = NULL;
        return -1;
    }

    return 0;
}

static int sql_pgsql_close(void)
{
    if (pgsql) {
	PQfinish(pgsql);
	pgsql = NULL;
    }

    return 0;
}

static t_sql_res * sql_pgsql_query_res(const char * query)
{
    t_pgsql_res *res;
    PGresult *pgres;

    if (pgsql == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "pgsql driver not initilized");
        return NULL;
    }

    if (query == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL query");
        return NULL;
    }

    if ((pgres = PQexec(pgsql, query)) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "not enough memory for query (%s)", query);
	return NULL;
    }

    if (PQresultStatus(pgres) != PGRES_TUPLES_OK) {
/*        eventlog(eventlog_level_debug, __FUNCTION__, "got error from query (%s)", query); */
	PQclear(pgres);
	return NULL;
    }

    res = (t_pgsql_res *)xmalloc(sizeof(t_pgsql_res));
    res->rowbuf = xmalloc(sizeof(char *) * PQnfields(pgres));
    res->pgres = pgres;
    res->crow = 0;

/*    eventlog(eventlog_level_debug, __FUNCTION__, "res: %p res->rowbuf: %p res->crow: %d res->pgres: %p", res, res->rowbuf, res->crow, res->pgres); */
    return res;
}

static void _pgsql_update_arows (const char *str)
{
    if (!str || str[0] == '\0') lastarows = 0;
    lastarows = (unsigned int)atoi(str);
}

static int sql_pgsql_query(const char * query)
{
    PGresult *pgres;
    int res;

    if (pgsql == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "pgsql driver not initilized");
        return -1;
    }

    if (query == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "got NULL query");
        return -1;
    }

    if ((pgres = PQexec(pgsql, query)) == NULL) {
        eventlog(eventlog_level_error, __FUNCTION__, "not enough memory for result");
        return -1;
    }

    res = PQresultStatus(pgres) == PGRES_COMMAND_OK ? 0 : -1;
    /* Dizzy: HACK ALERT! cache affected rows here before destroying result */
    if (!res) _pgsql_update_arows(PQcmdTuples(pgres));
    PQclear(pgres);

    return res;
}

static t_sql_row * sql_pgsql_fetch_row(t_sql_res *result)
{
    int nofields, i;
    t_pgsql_res *res = (t_pgsql_res *) result;

    if (res == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return NULL;
    }

    if (res->crow < 0) {
	eventlog(eventlog_level_error, __FUNCTION__, "got called without a proper res query");
	return NULL;
    }

    if (res->crow >= PQntuples(res->pgres)) return NULL; /* end of result */

    nofields = PQnfields(res->pgres);
    for(i = 0; i < nofields; i++) {
	res->rowbuf[i] = PQgetvalue(res->pgres, res->crow, i);
	/* the next line emulates the mysql way where NULL containing fields return NULL */
	if (res->rowbuf[i] && res->rowbuf[i][0] == '\0') res->rowbuf[i] = NULL;
    }

    res->crow++;

/*    eventlog(eventlog_level_debug, __FUNCTION__, "res: %p res->rowbuf: %p res->crow: %d res->pgres: %p", res, res->rowbuf, res->crow, res->pgres); */
    return res->rowbuf;
}

static void sql_pgsql_free_result(t_sql_res *result)
{
    t_pgsql_res *res = (t_pgsql_res *) result;

    if (res == NULL) return;
/*    eventlog(eventlog_level_debug, __FUNCTION__, "res: %p res->rowbuf: %p res->crow: %d res->pgres: %p", res, res->rowbuf, res->crow, res->pgres); */

    if (res->pgres) PQclear(res->pgres);
    if (res->rowbuf) xfree((void*)res->rowbuf);
    xfree((void*)res);
}

static unsigned int sql_pgsql_num_rows(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return 0;
    }

    return PQntuples(((t_pgsql_res *)result)->pgres);
}

static unsigned int sql_pgsql_num_fields(t_sql_res *result)
{
    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return 0;
    }

    return PQnfields(((t_pgsql_res *)result)->pgres);
}

static unsigned int sql_pgsql_affected_rows(void)
{
    return lastarows;
}

static t_sql_field * sql_pgsql_fetch_fields(t_sql_res *result)
{
    t_pgsql_res *res = (t_pgsql_res *) result;
    unsigned fieldno, i;
    t_sql_field *rfields;

    if (result == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL result");
	return NULL;
    }

    fieldno = PQnfields(res->pgres);

    rfields = xmalloc(sizeof(t_sql_field) * (fieldno + 1));
    for(i = 0; i < fieldno; i++)
	rfields[i] = PQfname(res->pgres, i);
    rfields[i] = NULL;

    return rfields;
}

static int sql_pgsql_free_fields(t_sql_field *fields)
{
    if (fields == NULL) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL fields");
	return -1;
    }

    xfree((void*)fields);
    return 0; /* PQclear() should free the rest properly */
}

static void sql_pgsql_escape_string(char *escape, const char *from, int len)
{
    PQescapeString(escape, from, len);
}

}

}

#endif /* WITH_SQL_PGSQL */
