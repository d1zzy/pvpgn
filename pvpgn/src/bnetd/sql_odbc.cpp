/*
 * Copyright (C) 2005 CreepLord (creeplord@pvpgn.org)
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
#ifdef WITH_SQL_ODBC
#include "common/setup_before.h"
#ifdef WIN32
# include <windows.h>
#endif
#include <sqlext.h>
#include <stdlib.h>
#include <ctype.h>
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "storage_sql.h"
#include "sql_odbc.h"
#include "common/setup_after.h"

/* t_sql_engine Interface methods. */
static int sql_odbc_init(const char *, const char *, const char *, const char *, const char *, const char *);
static int sql_odbc_close(void);
static t_sql_res * sql_odbc_query_res(const char *);
static int sql_odbc_query(const char *);
static t_sql_row * sql_odbc_fetch_row(t_sql_res *);
static void sql_odbc_free_result(t_sql_res *);
static unsigned int sql_odbc_num_rows(t_sql_res *);
static unsigned int sql_odbc_affected_rows(void);
static unsigned int sql_odbc_num_fields(t_sql_res *);
static t_sql_field * sql_odbc_fetch_fields(t_sql_res *);
static int sql_odbc_free_fields(t_sql_field *);
static void sql_odbc_escape_string(char *, const char *, int);

t_sql_engine sql_odbc = {
	sql_odbc_init,
	sql_odbc_close,
	sql_odbc_query_res,
	sql_odbc_query,
	sql_odbc_fetch_row,
	sql_odbc_free_result,
	sql_odbc_num_rows,
	sql_odbc_num_fields,
	sql_odbc_affected_rows,
	sql_odbc_fetch_fields,
	sql_odbc_free_fields,
	sql_odbc_escape_string
};

struct t_odbc_rowSet_{
	t_sql_row *row;
	struct t_odbc_rowSet_ *next;
};
typedef struct t_odbc_rowSet_ t_odbc_rowSet;

typedef struct{
	HSTMT stmt;
	t_odbc_rowSet *curRow;
	t_odbc_rowSet *rowSet;
	unsigned int rowCount;
} t_odbc_res;

static t_sql_row *odbc_alloc_row(t_odbc_res *result, SQLINTEGER *sizes);
static t_odbc_rowSet *odbc_alloc_rowSet();
static int odbc_Result(SQLRETURN retCode);
static void odbc_Error(SQLSMALLINT type, void *obj, t_eventlog_level level, const char *function);
static int odbc_Fail();

static HENV env = SQL_NULL_HENV;
static HDBC con = SQL_NULL_HDBC;

static SQLINTEGER ROWCOUNT = 0;

/************************************************
	t_sql_engine Interface methods.
************************************************/

static int sql_odbc_init(const char *host, const char *port, const char *socket, const char *name, const char *user, const char *pass)
{
	/* Create environment. */
	if(odbc_Result(SQLAllocEnv(&env)) && odbc_Result(SQLAllocConnect(env, &con))) {
		eventlog(eventlog_level_debug, __FUNCTION__, "Created ODBC environment.");
	} else {
		eventlog(eventlog_level_error, __FUNCTION__, "Unable to allocate ODBC environment.");
		return odbc_Fail();
	}
	if(odbc_Result(SQLConnect(con, (SQLCHAR*)name, SQL_NTS, (SQLCHAR*)user, SQL_NTS, (SQLCHAR*)pass, SQL_NTS))) {
		eventlog(eventlog_level_debug, __FUNCTION__, "Connected to ODBC datasource \"%s\".", name);
	} else {
		eventlog(eventlog_level_error, __FUNCTION__, "Unable to connect to ODBC datasource \"%s\".", name);
		odbc_Error(SQL_HANDLE_DBC, con, eventlog_level_error, __FUNCTION__);
		return odbc_Fail();
	}
	return 0;
}

static int sql_odbc_close(void)
{
	if(con) {
		SQLDisconnect(con);
		SQLFreeHandle(SQL_HANDLE_DBC, con);
		con = NULL;
	}
	if(env) {
		SQLFreeHandle(SQL_HANDLE_ENV, env);
		env = NULL;
	}
	eventlog(eventlog_level_debug, __FUNCTION__, "ODBC connection closed.");
	return 0;
}

static t_sql_res* sql_odbc_query_res(const char *query)
{
	t_odbc_res *res = NULL;
	HSTMT stmt = SQL_NULL_HSTMT;
	SQLRETURN result = 0;
	t_odbc_rowSet *rowSet = NULL;
	ROWCOUNT = 0;
	
	/* Validate query. */
	if(query == NULL) {
		eventlog(eventlog_level_error, __FUNCTION__, "Got a NULL query!");
		return NULL;
	}
//	eventlog(eventlog_level_trace, __FUNCTION__, "%s", query);

	/* Run query and check for success. */
	SQLAllocStmt(con, &stmt);
	result = SQLExecDirect(stmt, (SQLCHAR*)query, SQL_NTS);
	if(!odbc_Result(result)) {
//		odbc_Error(SQL_HANDLE_STMT, stmt, eventlog_level_debug, __FUNCTION__);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return NULL;
	}

	/* Create a result. */
	res = xmalloc(sizeof *res);
	res->stmt = stmt;
	rowSet = odbc_alloc_rowSet();
	res->rowSet = rowSet;
	res->curRow = rowSet;
	res->rowCount = 0;

	/* Store rows. */
	do {
		SQLINTEGER *sizes = NULL;
		t_sql_row *row = odbc_alloc_row(res, sizes);
		if(!row) {
			sql_odbc_free_result(res);
			return NULL;
		}
		result = SQLFetch(stmt);
		if(odbc_Result(result)) {
			rowSet->row = row;
			rowSet->next = odbc_alloc_rowSet();
			rowSet = rowSet->next;
			res->rowCount++;
		} else {
			sql_odbc_free_fields(row);
		}
		if(sizes) xfree(sizes);
	} while(odbc_Result(result));
	ROWCOUNT = res->rowCount;
	if(result == SQL_NO_DATA_FOUND) {
		return res;
	} else if(!odbc_Result(result)) {
		eventlog(eventlog_level_error, __FUNCTION__, "Unable to fetch row - ODBC error %i.", result);
		odbc_Error(SQL_HANDLE_STMT, stmt, eventlog_level_error, __FUNCTION__);
		sql_odbc_free_result(res);
		return NULL;
	}
	return res;
}

static int sql_odbc_query(const char *query)
{
	int result;
	HSTMT stmt = SQL_NULL_HSTMT;

	/* Validate query. */
	if(query == NULL) {
		eventlog(eventlog_level_error, __FUNCTION__, "Got a NULL query!");
		return -1;
	}
//	eventlog(eventlog_level_trace, __FUNCTION__, "%s", query);

	SQLAllocStmt(con, &stmt);
	result = odbc_Result(SQLExecDirect(stmt, (SQLCHAR*)query, SQL_NTS));
	if(result) {
		SQLRowCount(stmt, &ROWCOUNT);
	} else {
//		odbc_Error(SQL_HANDLE_STMT, stmt, eventlog_level_trace, __FUNCTION__);
	}
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	result = result == -1 ? 0 : -1;
	return result;
}

static t_sql_row* sql_odbc_fetch_row(t_sql_res *result)
{
	t_odbc_res *res;
	t_sql_row *row;
	if(!result) {
		eventlog(eventlog_level_error, __FUNCTION__, "Got NULL result.");
		return NULL;
	}
	res = (t_odbc_res*)result;
	row = res->curRow->row;
	if(row) {
		res->curRow = res->curRow->next;
	}
	return row;
}

static void sql_odbc_free_result(t_sql_res *result)
{
	if(result) {
		t_odbc_res *res = (t_odbc_res*)result;
		if(res->stmt) {
			SQLFreeHandle(SQL_HANDLE_STMT, res->stmt);
			res->stmt = NULL;
		}
		if(res->rowSet) {
			t_odbc_rowSet *rowSet = res->rowSet;
			while(rowSet) {
				t_odbc_rowSet *temp = (rowSet->next);
				sql_odbc_free_fields(rowSet->row);
				xfree(rowSet);
				rowSet = temp;
			}
		}
		xfree(result);
	}
}

static unsigned int sql_odbc_num_rows(t_sql_res *result)
{
	if(!result) {
		eventlog(eventlog_level_error, __FUNCTION__, "Got NULL result.");
		return 0;
	}
	return ((t_odbc_res*)result)->rowCount;
}

static unsigned int sql_odbc_num_fields(t_sql_res *result)
{
	SQLSMALLINT numCols = 0;
	if(!result) {
		eventlog(eventlog_level_error, __FUNCTION__, "Got NULL result.");
	} else {
		SQLNumResultCols(((t_odbc_res*)result)->stmt, &numCols);
	}
	return numCols;
}

static unsigned int sql_odbc_affected_rows(void)
{
	return ROWCOUNT;
}

static t_sql_field* sql_odbc_fetch_fields(t_sql_res *result)
{
	int i;
	t_odbc_res *res;
	int fieldCount;
	t_sql_field *fields;
	if(!result) {
		eventlog(eventlog_level_error, __FUNCTION__, "Got NULL result.");
		return NULL;
	}
	res = (t_odbc_res*)result;
	fieldCount = sql_odbc_num_fields(result);
	fields = xcalloc(sizeof *fields, fieldCount+1);
	if(!fields) {
		return NULL;
	}
	fields[fieldCount] = NULL;
	for(i=0; i<fieldCount; i++) {
		TCHAR *fName;
		TCHAR *tmp;
		SQLSMALLINT fNameSz;
		SQLColAttribute(res->stmt, i+1, SQL_DESC_NAME, NULL, 0, &fNameSz, NULL);
		fName = xmalloc(fNameSz);
		if(!fName) {
			return NULL;
		}
		SQLColAttribute(res->stmt, i+1, SQL_DESC_NAME, fName, fNameSz, &fNameSz, NULL);
		tmp = fName;
		for ( ; *tmp; ++tmp) *tmp = toupper(*tmp);
		fields[i] = fName;
	}
	return fields;
}

static int sql_odbc_free_fields(t_sql_field *fields)
{
	if(fields) {
		int i=0;
		while(fields[i]) {
			xfree(fields[i]);
			i++;
		}
		xfree(fields);
	}
	return 0;
}

static void sql_odbc_escape_string(char *escape, const char *from, int len)
{
	int i;
	for (i=0; i<len && from[i]!=0; i++) {
		if (from[i] == '\'') {
			escape[i]='"';
		} else {
			escape[i]=from[i];
		}
	}
	escape[i] = 0;
}

/************************************************
	End t_sql_engine Interface methods.
************************************************/

static t_sql_row* odbc_alloc_row(t_odbc_res *result, SQLINTEGER *sizes)
{
	int i, fieldCount;
	HSTMT stmt = result->stmt;
	t_sql_row *row;

	fieldCount = sql_odbc_num_fields(result);
	/* Create a new row. */
	row = xcalloc(sizeof *row, fieldCount+1);
	if(!row) {
		return NULL;
	}
	row[fieldCount] = NULL;
	sizes = xcalloc(sizeof *sizes, fieldCount);

	for(i=0; i<fieldCount; i++) {
		TCHAR *cell;
		SQLLEN cellSz;
		SQLColAttribute(stmt, i+1, SQL_DESC_DISPLAY_SIZE, NULL, 0, NULL, &cellSz);
		cell = xcalloc(sizeof *cell, ++cellSz);
		if(!cell) {
			return NULL;
		}
		SQLBindCol(stmt, i+1, SQL_C_CHAR, cell, cellSz, &sizes[i]);

		row[i] = cell;
	}
	return row;
}

static t_odbc_rowSet* odbc_alloc_rowSet()
{
	t_odbc_rowSet *rowSet = xmalloc(sizeof *rowSet);
	rowSet->row = NULL;
	rowSet->next = NULL;
	return rowSet;
}

static void odbc_Error(SQLSMALLINT type, void *obj, t_eventlog_level level, const char *function)
{
	SQLCHAR mState[6] = "\0";
	long native = 0;
	SQLSMALLINT mTextLen;
	short i = 0;

	while(SQLGetDiagRec(type, obj, ++i, NULL, NULL, NULL, 0, &mTextLen) != SQL_NO_DATA) {
		SQLCHAR *mText = xcalloc(sizeof *mText, ++mTextLen);
		SQLGetDiagRec(type, obj, i, mState,	&native, mText, mTextLen, NULL);
		eventlog(level, function, "ODBC Error: State %s, Native %i: %s", mState, native, mText);
		xfree(mText);
	}
}

static int odbc_Result(SQLRETURN retCode)
{
	int result = 0;
	if(retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
		result = -1;
	}
	return result;
}

static int odbc_Fail()
{
	sql_odbc_close();
	return -1;
}

#endif
