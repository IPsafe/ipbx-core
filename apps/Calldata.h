/*
 *  Calldata.h
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#ifndef _CALLDATA_H
#define _CALLDATA_H

#include <asterisk.h>
#include <asterisk/channel.h>

#include "Device.h"
#include "Provider.h"

typedef struct _Service {
    char name[32];
    char password[12];
    char args[62];
    
    
} Service;

typedef struct _Outbound {
	Device device_origin;
	Provider provider;

	int lcr_id;
	int rates_id;
	int dialrules_id;

	char calldata_rate_name[30];
	char provider_rate_name[30];
	char provider_lcr_name[30];

	char prefix_name[30];
	char prefix[30];
	double prefix_rate;
	
	int increase;
	int minute;
	double value;
    
} Outbound;

typedef struct _Inbound {
	Device device_destination;
	char number[32];
	char destination[32];
	Provider provider;
    Service service;

} Inbound;


typedef struct _Internal {
	Device device_origin;
	Device device_destination;
    Service service;

} Internal;
	

typedef struct _Calldata {
	char peerip[32];
    char recvip[32];
    char from[128];
    char uri[128];
    char useragent[128];
    char peername[32];
    char channel[32];
	char blindtransfer[32];
	char transferername[32];
	
    char origin[32];
    char destination[64];
	char destination2[64];
	char forward[32];
			
    char uniqueid[64];
    char linkedid[64];
    int sequence;
    char clid[100];
	
    int duration2;
    int duration;
	
    char hangup_cause[32];
	char type_call[6];
	
	char dialargs[128];
	
	char observation[60];
	int transference;
} Calldata;


int InitCalldata(Calldata *calldata, struct ast_channel *chan);
void LoadCalldata(Calldata *calldata, struct ast_channel *chan);

#endif
