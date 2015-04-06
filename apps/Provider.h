/*
 *  Provider.h
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */


#ifndef _PROVIDER_H
#define _PROVIDER_H

typedef struct _Provider {
	int id;
	char name[32];
	char device_name[32];
	char technology[6];
	int dialrules;
	char destination[32];
	char host[40];
} Provider;

#endif
