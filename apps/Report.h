/*
 *  Report.h
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#ifndef _REPORT_H
#define _REPORT_H

#include "Calldata.h"

#define DATE_R_FORMAT "%Y-%m-%d %T"

typedef struct _Report {
    char calldate[120];
    char origin[30];
    char destination[80];
    char forward[40];
    char provider[60];
    int duration;
    int duration_billing;
    char status[20];
    char user_name[50];
    char exten_description[50];
    float value;
    char call_type[32];
    int transference;
    char observation[62];
    char uniqueid[64];
    char linkedid[64];
    char uri[64];
    int origin_user_id;
    int destination_user_id;
    int origin_exten_id;
    int destination_exten_id;
    int queue_id;
    int ivr_id;
    int lcr_id;
    int sequence;
    int provider_id;
	
} Report;

int InitReport(Report *);

int SaveReportOutbound(Report *report, Calldata *calldata, Outbound *outbound);
int SaveReportInbound(Report *report, Calldata *calldata, Inbound *inbound);
int SaveReportInternal(Report *report, Calldata *calldata, Internal *internal);


int SaveReport(Report*);

#endif
