/*
 *  Database.c
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */


#include <asterisk.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <libpq-fe.h>

#include <asterisk/utils.h>
#include <asterisk/pbx.h>
#include <asterisk/lock.h>
#include <asterisk/manager.h>
#include <asterisk/file.h>
#include <asterisk/channel.h>
#include <asterisk/module.h>
#include <asterisk/config.h>

#include "Database.h"

static int version;
static time_t connect_time = 0;

static PGconn *conn = NULL;

AST_MUTEX_DEFINE_STATIC(pgsql_lock);

int connected = 0;

int insertQuery(struct ast_str *sql)
{
	char *pgerror;
    PGresult *result;
	ast_mutex_lock(&pgsql_lock);	

	if (PQstatus(conn) == CONNECTION_OK) {
		connected = 1;
	} else {
		ast_log(LOG_ERROR, "Connection was lost... attempting to reconnect.\n");
		PQreset(conn);
		if (PQstatus(conn) == CONNECTION_OK) {
			ast_log(LOG_ERROR, "Connection reestablished.\n");
			connected = 1;
		} else {
			pgerror = PQerrorMessage(conn);
			ast_log(LOG_ERROR, "Unable to reconnect to database server %s. Calls will not be logged!\n", pghostname);
			ast_log(LOG_ERROR, "Reason: %s\n", pgerror);
			PQfinish(conn);
			conn = NULL;
			connected = 0;
			ast_mutex_unlock(&pgsql_lock);
			return -1;
		}
	}
	
	//Connection OK! execute query
	result = PQexec(conn, ast_str_buffer(sql));

	if (PQresultStatus(result) != PGRES_COMMAND_OK)
	{
		pgerror = PQresultErrorMessage(result);
		ast_log(LOG_ERROR, "Failed to insert call detail record into database!\n");
		ast_log(LOG_ERROR, "Reason: %s\n", pgerror);
		ast_log(LOG_ERROR, "Connection may have been lost... attempting to reconnect.\n");

		//Try again
		
		PQreset(conn);
		
		if (PQstatus(conn) == CONNECTION_OK) 
		{
			ast_log(LOG_ERROR, "Connection reestablished.\n");
			connected = 1;
			PQclear(result);
			
			//Try again...
			result = PQexec(conn, ast_str_buffer(sql));
			
			if (PQresultStatus(result) != PGRES_COMMAND_OK)
			{
				pgerror = PQresultErrorMessage(result);
				ast_log(LOG_ERROR, "HARD ERROR!  Attempted reconnection failed.  DROPPING CALL RECORD!\n");
				ast_log(LOG_ERROR, "Reason: %s\n", pgerror);
				PQclear(result);
				ast_mutex_unlock(&pgsql_lock);
				return -1;
			}
		} 
	}
	PQclear(result);
	ast_mutex_unlock(&pgsql_lock);
	return 0;
}

char* getResultByField(PGresult* result, const char *field, int rowIndex ){
	char *fieldValue = NULL;
	int found = 0;
	int i;
	
	if (!result) 
    {
        ast_log(LOG_ERROR, "RESULT is NULL: %p \n", result);
        return NULL;
    }
	if ((PQntuples(result)) > 0) {
		int numFields = PQnfields(result);
		for (i = 0; i < numFields; i++) {
			if (!strcmp(PQfname(result, i),field)) {
				found = 1;
				break;
			}
		}
		
		if (found == 0) {
			ast_log(LOG_ERROR, "Column not found: %s \n", field);
			return NULL;
		}
		
		fieldValue = PQgetvalue(result, rowIndex, i);
		if (!fieldValue) {
			ast_log(LOG_ERROR, "Column %s is NULL!!! \n", field);
			return NULL; 
		}
		//ast_log(LOG_NOTICE, "Column: %s = %s \n",field, fieldValue);
	}
	return fieldValue;
}


PGresult *executeQuery(struct ast_str *sql) {
	
	PGresult* result = NULL;
	int num_rows = 0;
	//char *pgerror;
	
	ast_mutex_lock(&pgsql_lock);

	if (!CheckConnection()) {
		return NULL;
	}	
	
	if (!(result = PQexec(conn, ast_str_buffer(sql)))) {
		ast_log(LOG_WARNING,
				"PostgreSQL IPBX: Failed to query '%s'. Check debug for more info.\n", pgdbname);
		ast_log(LOG_WARNING, "PostgreSQL IPBX: Query: %s\n", ast_str_buffer(sql));
		ast_log(LOG_WARNING, "PostgreSQL IPBX: Query Failed because: %s\n", PQerrorMessage(conn));
		ast_mutex_unlock(&pgsql_lock);
		return NULL;
	} else {
		ExecStatusType result_status = PQresultStatus(result);
		if (result_status != PGRES_COMMAND_OK
			&& result_status != PGRES_TUPLES_OK
			&& result_status != PGRES_NONFATAL_ERROR) {
			ast_log(LOG_WARNING,
					"PostgreSQL IPBX: Failed to query '%s'. Check debug for more info.\n", pgdbname);
			ast_log(LOG_WARNING, "PostgreSQL IPBX: Query: %s\n", ast_str_buffer(sql));
			ast_log(LOG_WARNING, "PostgreSQL IPBX: Query Failed because: %s (%s)\n",
					  PQresultErrorMessage(result), PQresStatus(result_status));
			ast_mutex_unlock(&pgsql_lock);
			return NULL;
		}
	}
	
	if ((num_rows = PQntuples(result)) < 1) {
		//ast_log(LOG_NOTICE, "[No Result]  Result=%p Query: %s\n", result, ast_str_buffer(sql));
		ast_mutex_unlock(&pgsql_lock);
		return NULL;
	}
	//ast_log(LOG_NOTICE, "Result=%p Query: %s\n", result, ast_str_buffer(sql));	
	ast_mutex_unlock(&pgsql_lock);
	return result;
}


int CheckConnection(void)
{
	ast_mutex_lock(&pgsql_lock);

	if (conn && PQstatus(conn) != CONNECTION_OK) {
		PQfinish(conn);
		ast_mutex_unlock(&pgsql_lock);
		conn = NULL;
	}
	
	if ((!conn) && !ast_strlen_zero(pghostname) && !ast_strlen_zero(pgdbuser) && !ast_strlen_zero(pgdbname))
    {
		struct ast_str *connInfo = ast_str_create(32);
		ast_str_set(&connInfo, 0, "host=%s port=%s dbname=%s user=%s",
					pghostname, pgdbport, pgdbname, pgdbuser);
		if (!ast_strlen_zero(pgpassword)) 
        {
			ast_str_append(&connInfo, 0, " password=%s", pgpassword);
		}
		ast_debug(1, "%u connInfo=%s\n", (unsigned int)ast_str_size(connInfo), ast_str_buffer(connInfo));
		conn = PQconnectdb(ast_str_buffer(connInfo));
		ast_free(connInfo);
		connInfo = NULL;
		
		ast_debug(1, "Conn=%p\n", conn);
		if (conn && PQstatus(conn) == CONNECTION_OK) 
        {
			ast_debug(1, "PostgreSQL IPBX: Successfully connected to database.\n");
			connect_time = time(NULL);
			version = PQserverVersion(conn);
			ast_mutex_unlock(&pgsql_lock);
			return 1;
		} else {
			ast_log(LOG_ERROR,
					"PostgreSQL IPBX: Failed to connect database %s on %s: %s\n",
					pgdbname, pghostname, PQresultErrorMessage(NULL));
			ast_mutex_unlock(&pgsql_lock);
			return 0;
		}
	} 
	else {
		ast_debug(1, "PostgreSQL IPBX: Connection Ok.\n");
		ast_mutex_unlock(&pgsql_lock);
		return 1;
	}
}


void FinishDatabase() 
{
	PQfinish(conn);
	if (pghostname)
		ast_free(pghostname);
	if (pgdbname)
		ast_free(pgdbname);
	if (pgdbuser)
		ast_free(pgdbuser);
	if (pgpassword)
		ast_free(pgpassword);
	if (pgdbport)
		ast_free(pgdbport);
}


