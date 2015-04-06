/*
 *  RamalDestino.h
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#ifndef _DEVICE_H
#define _DEVICE_H

typedef struct _Device {
	int id;
    int user_id;
	char user_name[50];
	char description[50];
	char host[32];
	char exten[32];
	int dnd;
	char followme[40];
	char forward[30];
	int  voicemail;
	int pickupgroup;
	int callgroup;
	int status;
	int dial_time;	
} Device;

#endif
