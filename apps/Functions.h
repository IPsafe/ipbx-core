/*
 *  Funcoes.h
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

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

#include "Device.h"
#include "Report.h"

#define OUTBOUND_DIALFLAGS "oTXKeg"
#define INTERNAL_DIALFLAGS "otTxXkKeg"
#define INBOUND_DIALFLAGS "otxkeg"


int IpbxDial(struct ast_channel *chan, Calldata *calldata);

int GetDestination(struct ast_channel *chan, char *destination, Device *device_destination, Service *service);
int CallExten(struct ast_channel *chan, Calldata *calldata, Device *device_destination, Report *report, char *flag_type, Service *service);
int CallConference(struct ast_channel *chan, Calldata *calldata, Service *service);
int CallQueue(struct ast_channel *chan, Calldata *calldata, Service *service);

#endif

