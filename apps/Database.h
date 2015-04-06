/*
 *  Database.h
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#ifndef _DATABASE_H
#define _DATABASE_H

#include <asterisk.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <libpq-fe.h>


#define AST_MODULE "app_ipbx"

#define MAXSIZE 1024



/* PGSQL includes */


//PGresult *result;
//PGresult *result2;

char *pghostname;
char *pgdbname;
char *pgdbuser;
char *pgpassword;
char *pgdbport;


int CheckConnection(void);
int insertQuery(struct ast_str *sql);
PGresult *executeQuery(struct ast_str *sql);
char *getResultByField(PGresult* result, const char *field, int rowIndex);
void FinishDatabase(void);


#endif
