/*
 *  app_ipbx.h
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#ifndef _APP_IPBX_H
#define _APP_IPBX_H

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libpq-fe.h>

/* asterisk includes */
#include <asterisk.h>
#include <asterisk/utils.h>
#include <asterisk/pbx.h>
#include <asterisk/logger.h>
#include <asterisk/lock.h>
#include <asterisk/frame.h>
#include <asterisk/manager.h>
#include <asterisk/dsp.h>
#include <asterisk/translate.h>
#include <asterisk/file.h>
#include <asterisk/channel.h>
#include <asterisk/module.h>
#include <asterisk/threadstorage.h>
#include <asterisk/callerid.h>
#include <asterisk/localtime.h>

#include "Calldata.h"
#include "Report.h"

#define IPBX_VERSION "1"

/* Define SCHED_MULTITHREADED to run the scheduler in a special
 multithreaded mode. */
#define SCHED_MULTITHREADED

/* Define DEBUG_SCHED_MULTITHREADED to keep track of where each
 thread is actually doing. */
#define DEBUG_SCHED_MULTITHREAD

#define DATE_FORMAT "%Y-%m-%d"
#define TIME_FORMAT "%H:%M:%S"

//AST_MUTEX_DEFINE_STATIC(pgsql_lock);


extern int SHOW_QUERIES;
extern int IPBX_DEBUG;

struct ast_app *app;

enum {
	INBOUND = (1 << 0),
	TRANSFERENCE = (1 << 1),
	NORMAL = (1 << 2),
} option_flags;

AST_APP_OPTIONS(ipbx_opts, {
	AST_APP_OPTION('e', INBOUND ),
	AST_APP_OPTION('t', TRANSFERENCE ),
	AST_APP_OPTION('n', NORMAL ),
});

static char *app_ipbx = "IPBX";

static int ipbx_outbound(struct ast_channel *chan, Calldata *calldata, struct ast_str *sql, Report *report);
static int ipbx_inbound(struct ast_channel *chan, Calldata *calldata, struct ast_str *sql, Report *report);
static int ipbx_internal(struct ast_channel *chan, Calldata *calldata, struct ast_str *sql, Report *report);
static int ipbx_exec(struct ast_channel *chan, const char *data);
static char *handle_cli_ipbx_set_queries(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a);
static char *handle_cli_ipbx_set_debug(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a);
static char *handle_cli_ipbx_show_calls(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a);
static char *handle_cli_ipbx_show_status(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a);


#endif /* APP_IPBX_H */
