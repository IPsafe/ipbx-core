/*
 *  app_ipbx.c
 *  ipbx_core
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Author: Henrique Caetano Chehad <hchehad@gmail.com>
 *
 *	Changelog:
 *	-> 2011: Ported to Asterisk 1.8
 *	-> 2012: Added Blacklist functionality
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
 
/*** MODULEINFO
 <depend>pgsql</depend>
***/

#include <asterisk.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libpq-fe.h>

//#include <sys/signal.h>
//#include <sys/ioctl.h>




/* asterisk includes */
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
#include <asterisk/app.h>
#include <asterisk/cli.h>


#include "app_ipbx.h"
#include "Functions.h"
#include "Database.h"
#include "Provider.h"
#include "Calldata.h"

#include "Report.h"
#include "Configuration.h"


#define LIMIT_CALLS 1

#if LIMIT_CALLS
static int calls_limit = 1000;
#endif

int active_calls_count = 0;

int SHOW_QUERIES = 1;
int IPBX_DEBUG = 1;
int PORTABILITY = 1;

static int ipbx_exec(struct ast_channel *chan, const char *data)
{
	
	int ret = 0;
	struct ast_module_user *u;
	
	struct ast_flags flags;
	char *parse, *opts[1];
    
    PGresult *result;
		
	AST_DECLARE_APP_ARGS(args, 
						AST_APP_ARG(tipo);
	);
	
	
	if (!(parse = ast_strdupa((char *)data))) {
		ast_log(LOG_ERROR, "Out of memory!\n");
		return -1;
	}
	
	AST_STANDARD_APP_ARGS(args, parse);
	
	//Calldata contains all information about call	
	Calldata calldata;
	//Report is a structure responsible to get call data and save into database for log
	Report report;
	
	active_calls_count++;
		
	#if LIMIT_CALLS
	if (active_calls_count > calls_limit){
	    if (IPBX_DEBUG) ast_log(LOG_ERROR, "Limit of concurrent calls reached: [%i]\n", calls_limit);
		//	ret = ast_streamfile(chan, "contact-admin", chan->language);
		//		if (!ret) ret = ast_waitstream(chan, "");
		active_calls_count--;
		return -1;
	}
	#endif
	
	if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Active Calls[%i]\n", active_calls_count);
	
	struct ast_str *sql = ast_str_create(MAXSIZE);
	
	if (!sql) {
		ast_free(sql);
		active_calls_count--;
		return -1;
	}
    
    //Prevent to unload module if have calls in curse
    u = ast_module_user_add(chan);
		
    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>> Starting new call.....\n" );
	
	//Init calldata
	InitCalldata(&calldata, chan);
	//Load information in channel to calldata structure
	LoadCalldata(&calldata, chan);
	
	if(IPBX_DEBUG)
	{
		ast_log(LOG_NOTICE, "---------------------------------------------------------------------\n");
		ast_log(LOG_NOTICE, "TECH_TYPE.......: %s\n", ast_channel_tech(chan)->type);
		ast_log(LOG_NOTICE, "CHANNEL.........: %s\n", calldata.channel);
		ast_log(LOG_NOTICE, "PEERNAME........: %s\n", calldata.peername);
		ast_log(LOG_NOTICE, "PEERIP..........: %s\n", calldata.peerip);
		ast_log(LOG_NOTICE, "RECVIP..........: %s\n", calldata.recvip);
		ast_log(LOG_NOTICE, "URI.............: %s\n", calldata.uri);
		ast_log(LOG_NOTICE, "USERAGENT.......: %s\n", calldata.useragent);
		ast_log(LOG_NOTICE, "CLID............: %s\n", calldata.clid);
		ast_log(LOG_NOTICE, "UNIQUEID........: %s\n", calldata.uniqueid);
		ast_log(LOG_NOTICE, "LINKEDID........: %s\n", calldata.linkedid);
		ast_log(LOG_NOTICE, "BLINDFTRANSFER..: %s\n", calldata.blindtransfer);
		ast_log(LOG_NOTICE, "TRANSFERERNAME..: %s\n", calldata.transferername);
        ast_log(LOG_NOTICE, "FLAG............: %d\n", args.argc);
		ast_log(LOG_NOTICE, "EXTENSION.......: %s\n", calldata.destination);
		ast_log(LOG_NOTICE, "---------------------------------------------------------------------\n");
	}
	
	//if has a flag
	if (args.argc == 1) {
		ast_log(LOG_NOTICE, ">>> Has a flag\n");
		ast_app_parse_options(ipbx_opts, &flags, opts, args.tipo);
		//if flag inbound has setted, jump the steps to detect what the type of call
		if (ast_test_flag(&flags, INBOUND))
		{
			ast_log(LOG_NOTICE, ">>> Inbound Call\n");
			ret = ipbx_inbound(chan, &calldata, sql, &report);
			
		//if flag transference are set, special prevents are need	
		} else if (ast_test_flag(&flags, TRANSFERENCE)) {
			ast_log(LOG_NOTICE, ">>> Transference\n");
			char *orig;
			char tmporig[80];
			calldata.transference = 1;

			//if peername has provider, than the call comes from a trunk
			ast_str_set(&sql, 0, "SELECT name FROM devices WHERE name = '%s' AND device_type = 'provider' LIMIT 1", calldata.peername);
			if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n", ast_str_buffer(sql));
			result = executeQuery(sql);
			if (result != NULL)
			{
				ast_log(LOG_NOTICE, ">>> Transference - Provider\n");
				ast_copy_string(tmporig, calldata.uri, sizeof(tmporig));
				if ((orig = strchr(tmporig, '@')))
				{
					*orig++ = '\0';
				}
				if ((orig = strchr(tmporig, ':'))) 
				{
					*orig++ = '\0';
				}
				ast_copy_string(calldata.origin, orig, sizeof(calldata.origin)-1);
				sprintf(calldata.clid,"%s <%s>", orig, orig);
				ret = ipbx_inbound(chan, &calldata, sql, &report);
			} else {
				//is a internal call
				ast_log(LOG_NOTICE, ">>> Transference - Internal Call\n");
				ret = ipbx_internal(chan, &calldata, sql, &report);	
			}
		}
	} else
	{
		//if origin is a extension, the call is internal or outbound
		ast_str_set(&sql, 0, "SELECT name FROM devices WHERE name = '%s' AND device_type = 'extension' LIMIT 1", calldata.origin);
		if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n", ast_str_buffer(sql));
		result = executeQuery(sql);
		if (result != NULL)
		{
			//if destination in dialplan, the call is internal

	        ast_str_set(&sql, 0, "SELECT "
	        			"CASE WHEN (SELECT id FROM devices WHERE device_type = 'extension' AND name = '%s') IS NOT NULL THEN 'extension' "
	        			"WHEN (SELECT id FROM conference WHERE confno = '%s') IS NOT NULL THEN 'conference' "
	        			"WHEN (SELECT id FROM queues WHERE name = '%s') IS NOT NULL THEN 'queue' "
	        			"END as type ", calldata.destination,calldata.destination,calldata.destination);

			if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
			
			result = executeQuery(sql);
			
			if (strcmp(getResultByField(result, "type", 0), ""))
			{
				ret = ipbx_internal(chan, &calldata, sql, &report);
			} else {
				ret = ipbx_outbound(chan, &calldata, sql, &report);
			}
		} else {
			ret = ipbx_inbound(chan, &calldata, sql, &report);
		}
		
	}
	
	//Save info about call in database
	SaveReport(&report);
	
	active_calls_count--;
	ast_free(sql);
	
	if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>> End of call\n" );
	
	ast_module_user_remove(u);
	return ret;
	
}

/*
 * Outbond call
 */

static int ipbx_outbound(struct ast_channel *chan, Calldata *calldata, struct ast_str *sql, Report *report)
{
	
	int ret = 0;
	int res = 0;

	char *portabilityData;

    PGresult *result;
    PGresult *result2;

    //Initializing report structure
    InitReport(report);
	if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>> IPBX outbound CDR:Start: %s\n", report->calldate);

	//Initializing Outbound structure
	Outbound outbound;

	outbound.value = 0;
	outbound.minute = 0;
	outbound.increase = 0;
	outbound.provider.id = 0;
	ast_copy_string(outbound.provider.name, "", sizeof(outbound.provider.name));
	outbound.device_origin.id = 0;
	outbound.device_origin.user_id = 0;
	ast_copy_string(outbound.device_origin.user_name, "", sizeof(outbound.device_origin.user_name));
	int lcr_id = 0;
	//Getting device info in database
	ast_str_set(&sql, 0, "SELECT devices.id, devices.user_id, devices.callerid, devices.name, auth_user.username as username, "
			"(CASE WHEN devices.force_monitor is True OR devices.force_monitor is True THEN 1 else 0 END) as monitor_admin, "
			"(CASE WHEN devices.monitor is True AND devices.monitor is True THEN 1 else 0 END) as monitor_user "
			"FROM devices LEFT JOIN auth_user ON devices.user_id = auth_user.id "
			"WHERE devices.name = '%s' LIMIT 1", calldata->origin);

	if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
	result = executeQuery(sql);
	if (result != NULL)
	{
		ast_copy_string(outbound.device_origin.description, getResultByField(result,"callerid",0),sizeof(outbound.device_origin.description));
		ast_copy_string(outbound.device_origin.exten, getResultByField(result, "name", 0), sizeof(outbound.device_origin.exten));
		ast_copy_string(outbound.device_origin.user_name, getResultByField(result, "username" ,0), sizeof(outbound.device_origin.user_name));
        outbound.device_origin.id = atoi(getResultByField(result, "id", 0));
        outbound.device_origin.user_id = atoi(getResultByField(result, "user_id", 0));
	} else {
		//no have origin in database, something are wrong!
		ast_verbose(VERBOSE_PREFIX_3 "Error finding origin in database! \n");	
        ast_copy_string(calldata->observation, "No Origin Found", sizeof(calldata->observation));
        ast_copy_string(calldata->hangup_cause, "Error", sizeof(calldata->hangup_cause));         
		ret = -1;
	}
	
	if (ret == 0)
	{		

		if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>> Getting Dialrules\n" );

			//get dialrules to apply in destination number before search in rates	
			if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>> Applying Dial Rules\n" );
			
			ast_str_set(&sql, 0,
				"SELECT cut, add, (coalesce(add,'')|| regexp_replace('%s', '^' || coalesce(replace(cut, '*', '(.+)'), ''),'')) as destination "
				"FROM public.dialrules "
				"INNER JOIN dialrules_groups ON dialrules.dialrules_groups_id = dialrules_groups.id "
				"INNER JOIN devices ON devices.dialrules_groups_id = dialrules_groups.id "
				"WHERE devices.id=%d AND (LENGTH('%s') >= min_len AND LENGTH('%s') <= max_len ) "
				"AND  ('%s' ~ ('^' || replace(cut, '*', '(.+)')) OR replace(cut, '*', '(.+)') IS NULL) "
				"ORDER BY LENGTH(cut) DESC NULLS LAST ", calldata->destination,outbound.device_origin.id,calldata->destination,calldata->destination,calldata->destination);

			if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
			result = executeQuery(sql);

			if (result != NULL)
			{
				ast_copy_string(calldata->destination2, getResultByField(result ,"destination", 0), sizeof(calldata->destination2));
				ast_verbose(VERBOSE_PREFIX_3 "Destination number after dialrules: %s (cut: %s, add: %s)\n", calldata->destination2, getResultByField(result ,"cut", 0), getResultByField(result ,"add", 0));
			} else {
				ast_copy_string(calldata->destination2, calldata->destination, sizeof(calldata->destination2));
				ast_verbose(VERBOSE_PREFIX_3 "Not found dialrules for this destination\n");
			}

			/**************************
				NUMERIC PORTABILITY
			 **************************/
			if(PORTABILITY) {
				ast_copy_string(calldata->destination, calldata->destination2, sizeof(calldata->destination)); // Make a copy of destination to remove portability tech prefix
				ast_channel_exten_set(chan, calldata->destination2);
				app = pbx_findapp("Portability");
				res = pbx_exec(chan, app, portabilityData);
				ast_copy_string(calldata->destination2, ast_channel_exten(chan), sizeof(calldata->destination2)); // Copy the return of Portability Consult
				ast_channel_exten_set(chan, calldata->destination);
			}
			/**********************
			 **********************/

			//search if user has rates to this destination
			if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>> Consulting user rates\n" );
			
			ast_str_set(&sql, 0, " SELECT rates_groups.name, rates.increment, rates.min_time, rates.price, rates.prefix "
					"FROM rates_groups "
					"INNER JOIN rates ON rates_groups.id = rates.rates_groups_id  "
					"INNER JOIN devices ON devices.rate_groups_id = rates_groups.id "
					"WHERE devices.id=%d "
					"AND (prefix = SUBSTRING('%s',1,length(prefix)) OR prefix = '*') "
					"ORDER BY length(prefix) DESC LIMIT 1",outbound.device_origin.id, calldata->destination2);
			if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n", ast_str_buffer(sql));
			
			result = executeQuery(sql);
			if (result != NULL)
			{	
				ast_copy_string(outbound.calldata_rate_name, getResultByField(result, "name", 0), sizeof(outbound.calldata_rate_name));
				ast_verbose(VERBOSE_PREFIX_3 "RATES: prefix (%s) / price (%s) / min_time (%s) / increment (%s) / \n\n",
						getResultByField(result, "prefix", 0), getResultByField(result, "price", 0), getResultByField(result, "min_time", 0), getResultByField(result, "increment", 0));
			} else {
				ast_verbose(VERBOSE_PREFIX_3 "No rate to route! \n");	
				/*
                ast_copy_string(calldata->observation, "No User Rate to Route", sizeof(calldata->observation));
                ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));         
				app = pbx_findapp("Playback");
				res = pbx_exec(chan,app,"invalid");
				ret = -1;
				*/
			}
	}
    
    if (ret == 0)
    {
        
        if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>> Searching LCR\n" );
                
        ast_str_set(&sql, 0,"SELECT lcr.id as lcr_id, lcr.name as lcr_name, lcr.order FROM devices INNER JOIN lcr ON lcr.id = devices.lcr_id WHERE devices.id = '%d' LIMIT 1", outbound.device_origin.id);
        if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
        
        result = executeQuery(sql);
        if (result != NULL)
        {
            ast_copy_string(outbound.provider_lcr_name, getResultByField(result,"lcr_name",0), sizeof(outbound.provider_lcr_name));
            lcr_id = atoi(getResultByField(result, "lcr_id", 0));

            ast_verbose(VERBOSE_PREFIX_3 "LCR Name: %s / ID: %d \n", outbound.provider_lcr_name, lcr_id);
            if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>> Getting providers in LCR\n");
            
        	ast_str_set(&sql, 0, "SELECT lcr.name as lcrname, devices.id as iddevice, devices.description as description, devices.name as devicename, device_type, devices.host as devicehost, devices.fromuser, devices.secret, rates_groups.name as rate_groups_name, "
        						"rates.increment, rates.min_time, rates.price, rates.prefix, devices.dialrules_groups_id "
        						"FROM lcr "
        						"LEFT JOIN lcr_providers ON lcr_id = lcr.id "
        						"LEFT JOIN devices ON provider_id = devices.id "
        						"LEFT JOIN rates_groups ON devices.rate_groups_id = rates_groups.id "
        						"LEFT JOIN rates ON rates_groups.id = rates.rates_groups_id  "
        						"WHERE lcr.id = %d "
        						"AND (prefix = SUBSTRING('%s',1,length(prefix)) OR prefix = '*') AND lcr_providers.active = TRUE "
        						"ORDER BY length(prefix) DESC, priority",lcr_id, calldata->destination2);

            if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n", ast_str_buffer(sql));
            
            result = executeQuery(sql);
            if (result != NULL)
            {

            	ast_copy_string(outbound.calldata_rate_name, getResultByField(result, "rate_groups_name", 0), sizeof(outbound.calldata_rate_name));
            	ast_verbose(VERBOSE_PREFIX_3 "RATES: prefix (%s) / price (%s) / min_time (%s) / increment (%s) \n\n",
            				getResultByField(result, "prefix", 0), getResultByField(result, "price", 0), getResultByField(result, "min_time", 0), getResultByField(result, "increment", 0));


                int attempts = PQntuples(result);

                //if (IPBX_DEBUG) ast_verbose(VERBOSE_PREFIX_3 "Destination before remove Portability tech prefix: %s \n", calldata->destination2);
                if (IPBX_DEBUG) ast_verbose(VERBOSE_PREFIX_3 "Destination after remove Portability tech prefix: %s \n\n", calldata->destination);
                if (IPBX_DEBUG) ast_log(LOG_NOTICE, "ATTEMPTS TERMINATIONS: %d\n", attempts);

                int n_try;
             
                for (n_try = 0; n_try < attempts; n_try++) {

					outbound.provider.id = atoi(getResultByField(result, "iddevice", n_try));
                    ast_copy_string(outbound.provider.name, getResultByField(result, "description", n_try), sizeof(outbound.provider.name));
                    ast_copy_string(outbound.provider.device_name, getResultByField(result, "devicename", n_try), sizeof(outbound.provider.device_name));
                    ast_copy_string(outbound.provider.technology, getResultByField(result, "device_type", n_try), sizeof(outbound.provider.technology));
                    outbound.provider.dialrules =  atoi(getResultByField(result, "dialrules_groups_id", n_try));
                    ast_copy_string(outbound.provider.host, getResultByField(result, "devicehost", n_try), sizeof(outbound.provider.host));
                    
                    //Provider rates
                    ast_copy_string(outbound.provider_rate_name, getResultByField(result, "lcrname", n_try), sizeof(outbound.provider_rate_name));
                    outbound.increase = atoi(getResultByField(result, "increment", n_try)); //TODO v2
                    outbound.minute = atoi(getResultByField(result, "min_time", n_try));			//TODO v2
                    outbound.prefix_rate = atof(getResultByField(result, "price", n_try));    //TODO v3
                    ast_copy_string(outbound.prefix_name, getResultByField(result, "prefix", n_try), sizeof(outbound.prefix_name));
                    ast_copy_string(outbound.prefix, getResultByField(result, "prefix", n_try), sizeof(outbound.prefix));

                    // Get dial rules
                    ast_str_set(&sql, 0,
                    	"SELECT cut, add, (coalesce(add,'')|| regexp_replace('%s', '^' || coalesce(replace(cut, '*', '(.+)'), ''),'')) as destination "
                    	"FROM public.dialrules "
                    	"INNER JOIN dialrules_groups ON dialrules.dialrules_groups_id = dialrules_groups.id "
                    	"INNER JOIN devices ON devices.dialrules_groups_id = dialrules_groups.id "
                    	"WHERE devices.id=%d AND (LENGTH('%s') >= min_len AND LENGTH('%s') <= max_len ) "
                    	"AND  ('%s' ~ ('^' || replace(cut, '*', '(.+)')) OR replace(cut, '*', '(.+)') IS NULL) "
                    	"ORDER BY LENGTH(cut) DESC NULLS LAST ", calldata->destination,outbound.provider.id,calldata->destination,calldata->destination,calldata->destination);

                    if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
                    result = executeQuery(sql);
                    if (result != NULL) {
                        ast_copy_string(outbound.provider.destination, getResultByField(result ,"destination", 0), sizeof(outbound.provider.destination));
                        ast_verbose(VERBOSE_PREFIX_3 "Destination number after dialrules: %s (cut: %s, add: %s)\n", calldata->destination2, getResultByField(result ,"cut", 0), getResultByField(result ,"add", 0));
                    } else {
                        ast_copy_string(outbound.provider.destination, calldata->destination, sizeof(outbound.provider.destination));
                    }
                    
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>> Calling provider %d\n", n_try+1 );
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, "--------------------------------\n");
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>>> Provider ID: %d\n",outbound.provider.id );
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>>> Provider: %s\n",outbound.provider.name );
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>>> Device Name: %s\n",outbound.provider.device_name );
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>>> IP: %s\n",outbound.provider.host );
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>>> Prefix Name: %s\n",outbound.prefix_name );
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>>> Prefix: %s\n",outbound.prefix );
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>>> Rate: %.2f\n",outbound.prefix_rate );
                    if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>>>> Destination: %s\n",outbound.provider.destination );

                    snprintf(calldata->dialargs, sizeof(calldata->dialargs), "SIP/%s@%s,60,%s", outbound.provider.destination, outbound.provider.device_name, OUTBOUND_DIALFLAGS );
                    
                    ret = IpbxDial(chan, calldata);
                    
                    //if return code is not error in provider then dont need to try next provider in lcr
                    if (!(ret == 12 || ret == 16 ))
                    {
                        break;
                    }
                }
                
                ast_copy_string(calldata->destination2, outbound.provider.destination, sizeof(calldata->destination2));
                
                //3s is the grace time. TODO: turn this configurable
                if (IPBX_DEBUG) ast_log(LOG_NOTICE, "CALL DURATION: %d\n", calldata->duration);
                if(calldata->duration > 3)
                {
                    calldata->duration2 = 0;			
                    if(calldata->duration  > outbound.minute){
                        calldata->duration2 = ( (int)ceil (( (double)calldata->duration - (double)outbound.minute) / (double)outbound.increase ) * (double)outbound.increase ) + outbound.minute;
                    }else{
                        calldata->duration2 = outbound.minute;
                    }
                    
                    outbound.value = (outbound.prefix_rate/60) * calldata->duration2;	
                    if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Prefix Rate: %f / Duration: %d / Value: %f \n", outbound.prefix_rate, calldata->duration2, outbound.value);
                } else {
                    outbound.value = 0;
                }
                
            } 
            else {
                ast_verbose(VERBOSE_PREFIX_3 "No provider for this route! \n");	
                ast_copy_string(calldata->observation, "No provider for this route", sizeof(calldata->observation));
                ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));         

                app = pbx_findapp("Playback");
                res = pbx_exec(chan,app,"no-rights");
                ret = -1;
            }	
            
        } 
        else {
            ast_verbose(VERBOSE_PREFIX_3 "No lcr for this route! \n");
            ast_copy_string(calldata->observation, "No lcr for this route", sizeof(calldata->observation));
            ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));         

            app = pbx_findapp("Playback");
            res = pbx_exec(chan,app,"no-rights");
            ret = -1;
        }
    }
    
	if (IPBX_DEBUG) ast_log(LOG_WARNING, "Dial Status: %s\n",calldata->hangup_cause);
	SaveReportOutbound(report, calldata, &outbound);
	
	return 0;
}

/*
 * Inbound call
 */

static int ipbx_inbound(struct ast_channel *chan, Calldata *calldata, struct ast_str *sql, Report *report)
{
	
	int ret = 0;
    
    PGresult *result;
	
	InitReport(report);
	if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>> IPBX inbound CDR:Start: %s\n", report->calldate);

	//Initializing inbound structure
	Inbound inbound;
	inbound.device_destination.id = 0;
	inbound.device_destination.user_id = 0;
	ast_copy_string(inbound.device_destination.user_name,"", sizeof(inbound.device_destination.user_name));
	ast_copy_string(inbound.number, "", sizeof(inbound.number));
	ast_copy_string(inbound.destination , "", sizeof(inbound.destination));
	inbound.provider.id = 0;
	ast_copy_string(inbound.provider.name , "", sizeof(inbound.provider.name));
	
	char dayWeek[100];
	char timeNow[100];
	
	time_t t;
	struct tm *tm;
	
	char *name;
	char featureargs[80];
	
	ast_copy_string(calldata->origin, calldata->clid, sizeof(calldata->origin));
	
	/*
	 * Blacklist
	 */

	ast_str_set(&sql, 0, "SELECT did FROM blacklist where did = '%s';", calldata->origin);
	if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL BLACKLIST: %s\n", ast_str_buffer(sql));
	result = executeQuery(sql);
	if (result != NULL)
	{
		ast_log(LOG_WARNING, "Number in Blacklist: %s \n", calldata->origin);
        ast_copy_string(calldata->observation, "Blacklist", sizeof(calldata->observation));
        ast_copy_string(calldata->hangup_cause, "Blacklist", sizeof(calldata->hangup_cause));
        ret = 1;
	}
	else {
		ast_log(LOG_NOTICE, "NOT IN BLACKLIST: %s \n", calldata->origin);
	}

	/*
	* End(Blacklist)
	*/

	if (ret == 0) 
	{
		 ast_str_set(&sql, 0, "SELECT id, name FROM devices WHERE name = '%s' AND device_type = 'provider' LIMIT 1", calldata->peername);
    	if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
    
    	result = executeQuery(sql);
    	if (result != NULL)
    	{
    	    ast_copy_string(inbound.provider.name, getResultByField(result,"name",0),sizeof(inbound.provider.name));
    	    inbound.provider.id = atoi(getResultByField(result,"id",0));

   		} else 
		{
			ret = 1;
			ast_copy_string(calldata->observation, "Provider Not Found", sizeof(calldata->observation));
        	ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));
		}

    }
	
	if (calldata->transference == 0 && ret == 0)
	{		
		//get destination to forward (postgres procedure)
		 ast_str_set(&sql, 0, "SELECT inbound_forward('%s')", calldata->destination2);
        
        if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
		
		result = executeQuery(sql);
		if (result != NULL)
		{		
			ast_copy_string(inbound.destination, getResultByField(result,"inbound_forward",0),sizeof(inbound.destination));
                        
            if (!strcmp(inbound.destination, "RULES_NOT_FOUND"))
            {
                if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Forward Rule not Found in Inbound Rules: %s. Hangout\n",calldata->destination2);
                ast_copy_string(calldata->observation, "No Forward Rule in Inbound Rules", sizeof(calldata->observation));
                ret = -1;
            } else if (!strcmp(inbound.destination, "ROUTE_NOT_FOUND"))
            {
                if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Route Rule not Found in Inbound Rules: %s. Hangout\n",calldata->destination2);

                ast_copy_string(calldata->observation, "No Route Rule in Inbound Rules", sizeof(calldata->observation));
                ret = -1;
            } else {
                ast_copy_string(inbound.number, calldata->destination2, sizeof(inbound.number));
                ret = 0;
            }
		} else {	
			
            if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Route Rule not Found in Inbound Rules: %s.\n",calldata->destination2);

            ast_copy_string(calldata->observation, "No Route Rule in Inbound Rules (SYSTEM_ERROR)", sizeof(calldata->observation));
			ret = -1;
		}
		
	} else {
		if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>> IPBX TRANSFERENCE \n");
		ast_copy_string(inbound.destination, calldata->destination,sizeof(inbound.destination));
	}

    if (ret == 0) 
	{
		
        //Find destination
        ret = GetDestination(chan, inbound.destination, &inbound.device_destination, &inbound.service);
        
        //Extension
        if (ret == 0)
        {
            ast_copy_string(calldata->forward, inbound.device_destination.exten, sizeof(calldata->forward));
            ast_copy_string(calldata->observation, "Extension", sizeof(calldata->observation));

            ret = CallExten(chan, calldata, &inbound.device_destination, report, INBOUND_DIALFLAGS, &inbound.service);
         
            if (ret == 1)
            {
                Calldata calldata2;
                InitCalldata(&calldata2, chan);
                
                ast_copy_string(calldata2.origin, inbound.device_destination.exten, sizeof(calldata2.origin));
                ast_copy_string(calldata2.destination, inbound.device_destination.followme, sizeof(calldata2.destination));
                
                ipbx_outbound(chan,&calldata2, sql, report);
                SaveReport(report);
                InitReport(report);
                
                ast_copy_string(calldata->forward, inbound.device_destination.followme, sizeof(calldata->forward));
                ast_copy_string(calldata->observation, "Follow Me", sizeof(calldata->observation));
                
                ret = 0;
            }
            
        }
        
        else if(ret == 1) //Conference
        {
            
            ret = CallConference(chan, calldata, &inbound.service);
            
            ast_copy_string(calldata->forward, inbound.destination, sizeof(calldata->forward));
        }
        
        else if(ret == 2) //Queue
        {
            ret = CallQueue(chan, calldata, &inbound.service);
            ast_copy_string(calldata->forward, inbound.destination, sizeof(calldata->forward));
        }
        
        else if(ret == 3) //IVR
        {
        	//IVR Not implemented in this version.
            ast_copy_string(calldata->forward, inbound.destination, sizeof(calldata->forward));

        }
        
        else if(ret == 4) //Mailbox
        { 
            app = pbx_findapp("VoicemailMain");
            pbx_exec(chan,app,"");
            ast_copy_string(calldata->observation, "Mail Box", sizeof(calldata->observation));
            ast_copy_string(calldata->hangup_cause, "Answered", sizeof(calldata->hangup_cause));
        }
        
        else if(ret == -1)
        {
            ast_copy_string(calldata->observation, "Destination Not Found!", sizeof(calldata->observation));
            ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));

        }
    }

	SaveReportInbound(report,calldata,&inbound);
	
	return 0;
}

/*
 * Internal call
 */

static int ipbx_internal(struct ast_channel *chan, Calldata *calldata, struct ast_str *sql, Report *report)
{
	
	Internal internal;
	
	int ret = 0;
	char *name, *tmp;
	char featureargs[80];
    PGresult *result;
		
	InitReport(report);	
	
	if(IPBX_DEBUG) ast_log(LOG_NOTICE, ">>> IPBX INTERNAL CDR:Start: %s\n", report->calldate);
	
	ast_str_set(&sql, 0, "SELECT devices.id, devices.user_id, devices.name, devices.callerid, auth_user.username as username, "
			"(CASE WHEN devices.force_monitor is True OR devices.force_monitor is True THEN 1  else 0  END) as monitor_admin, "
			"(CASE WHEN devices.monitor is True AND devices.monitor is True THEN 1 else 0 END) as monitor_user "
			"FROM devices "
			"LEFT JOIN auth_user ON devices.user_id = auth_user.id "
			"WHERE devices.name = '%s' AND devices.device_type = 'extension' LIMIT 1", calldata->origin);
	if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
	result = executeQuery(sql);
	if (result != NULL)
	{
		ast_copy_string(internal.device_origin.description, getResultByField(result, "callerid", 0), sizeof(internal.device_origin.description));
		ast_copy_string(internal.device_origin.exten, getResultByField(result, "name", 0), sizeof(internal.device_origin.exten));
		internal.device_origin.callgroup = atoi(getResultByField(result, "name", 0));
        internal.device_origin.id = atoi(getResultByField(result, "id", 0));
        internal.device_origin.user_id = atoi(getResultByField(result, "user_id", 0));
		ast_copy_string(internal.device_origin.user_name, getResultByField(result, "username" ,0), sizeof(internal.device_origin.user_name));
	} else {
		ret = -1;
	}

	//Find Destination
    if (ret == 0)
    {
   
        ret = GetDestination(chan, calldata->destination, &internal.device_destination, &internal.service);	
	
        if (ret == 0) //Extension
        {
            
            ret = CallExten(chan, calldata, &internal.device_destination, report, INTERNAL_DIALFLAGS, &internal.service);
        
            if (ret == 1)
            {
                Calldata calldata2;
                InitCalldata(&calldata2, chan);
                
                ast_copy_string(calldata2.origin, internal.device_destination.exten, sizeof(calldata2.origin));
                ast_copy_string(calldata2.destination, internal.device_destination.followme, sizeof(calldata2.destination));
                
                ipbx_outbound(chan, &calldata2, sql, report);
                SaveReport(report);
                InitReport(report);
                
                ast_copy_string(calldata->forward, internal.device_destination.followme, sizeof(calldata->forward));
                ast_copy_string(calldata->observation, "Followme", sizeof(calldata->observation));
                
                ret = 0;
            }
            
            //FollowmeBusy and FollowmeNotAnswer not implemented in this version.

        } 
        else if(ret == 1) //Conference
        {
            
            ret = CallConference(chan, calldata, &internal.service);
            
        }
        else if(ret == 2) //Queue
        {
            ret = CallQueue(chan, calldata, &internal.service);
        }
        
        else if(ret == 3) //IVR
        {
        	//IVR Not implemented in this version.
        }
        
        else if(ret == 4) //Mailbox
        { 
            app = pbx_findapp("VoicemailMain");
            pbx_exec(chan,app,"");
            ast_copy_string(calldata->observation, "Mail Box", sizeof(calldata->observation));
            ast_copy_string(calldata->hangup_cause, "Answered", sizeof(calldata->hangup_cause));
        }
        else if (ret == -1)
        {
            ast_copy_string(calldata->observation, "Destination Not Found", sizeof(calldata->observation));
            ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));
            
        }
    }
    	
	if (IPBX_DEBUG) ast_log(LOG_WARNING, "Status: %s\n",calldata->hangup_cause);
	SaveReportInternal(report,calldata, &internal);
	
	return 0;
}

static char *handle_cli_ipbx_set_queries(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
		case CLI_INIT:
            e->command = "ipbx set queries {on|off}";
            e->usage =
            "Usage: ipbx set queries [on|off]\n";
            return NULL;
        case CLI_GENERATE:
            return NULL;
    }
    
    if (a->argc != e->args)
        return CLI_SHOWUSAGE;
    
    if(!strcasecmp(a->argv[3],"on")){
        ast_cli(a->fd, "\n");
        ast_cli(a->fd, ">>> Query Debug ON\n");
        SHOW_QUERIES = 1;
    }else if(!strcasecmp(a->argv[3],"off")){
        ast_cli(a->fd, "\n");
        ast_cli(a->fd, ">>> Query Debug OFF\n");
        SHOW_QUERIES = 0;
    }else
        return CLI_SHOWUSAGE;
    
    return CLI_SUCCESS;
}

static char *handle_cli_ipbx_set_debug(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
    switch (cmd) {
        case CLI_INIT:
            e->command = "ipbx set debug {on|off}";
            e->usage =
            "Usage: ipbx set debug [on|off]\n";            
            return NULL;
        case CLI_GENERATE:
            return NULL;
    }
    
    if (a->argc != e->args)
        return CLI_SHOWUSAGE;
    
    if(!strcasecmp(a->argv[3],"on")){
        ast_cli(a->fd, "\n");
        ast_cli(a->fd, ">>> IPBX Debug ON\n");
        IPBX_DEBUG = 1;
    }else if(!strcasecmp(a->argv[3],"off")){
        ast_cli(a->fd, "\n");
        ast_cli(a->fd, ">>> IPBX Debug OFF\n");
		IPBX_DEBUG = 0;
	}else
		return CLI_SHOWUSAGE;
    
	return CLI_SUCCESS;
}

static char *handle_cli_ipbx_show_calls(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
		case CLI_INIT:
			e->command = "ipbx show calls";
			e->usage =
			"Usage: ipbx show calls\n"
			"       Show number of active calls\n";
			return NULL;
		case CLI_GENERATE:
			return NULL;
	}
    
	ast_cli(a->fd, "\n");
	ast_cli(a->fd, ">>> Active Calls: [%d]\n", active_calls_count);
	ast_cli(a->fd, "\n");
    
	return CLI_SUCCESS;
}

static char *handle_cli_ipbx_show_status(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
		case CLI_INIT:
			e->command = "ipbx show status";
			e->usage =
			"Usage: ipbx show status\n"
			"       Show information about app_ipbx.so\n";
			return NULL;
		case CLI_GENERATE:
			return NULL;
	}
    
	ast_cli(a->fd, "\n");
	ast_cli(a->fd, "--------------------------------------------------\n");
	ast_cli(a->fd, "-- 					IPBX 						--\n");
	ast_cli(a->fd, "--------------------------------------------------\n");
	ast_cli(a->fd, "|      Version: [%s]                             |\n", IPBX_VERSION);
	ast_cli(a->fd, "|      Debug: [%d]                               |\n", IPBX_DEBUG);
	ast_cli(a->fd, "|      Queries: [%d]                             |\n", SHOW_QUERIES);
	ast_cli(a->fd, "--------------------------------------------------\n");
	ast_cli(a->fd, "\n");
    
	return CLI_SUCCESS;
}

static struct ast_cli_entry cli_ipbx[] = 
{
	AST_CLI_DEFINE(handle_cli_ipbx_set_queries,		"Set SQL querys debug of module app_ipbx.so"),
	AST_CLI_DEFINE(handle_cli_ipbx_set_debug,		"Set IPBX debug of module app_ipbx.so"),
	AST_CLI_DEFINE(handle_cli_ipbx_show_calls,		"Show active calls of module app_ipbx.so"),
	AST_CLI_DEFINE(handle_cli_ipbx_show_status,		"Show informations about app_ipbx.so")
};

static int load_module(void)
{

	if (ConfigIpbx(0)) {
	
		if (!CheckConnection())
			return AST_MODULE_LOAD_DECLINE;

		ast_cli_register_multiple(cli_ipbx, ARRAY_LEN(cli_ipbx));
        
        //LoadGeneralConfigs();
        
        return ast_register_application_xml(app_ipbx, ipbx_exec) ? AST_MODULE_LOAD_DECLINE : AST_MODULE_LOAD_SUCCESS;
	} else {
		return AST_MODULE_LOAD_DECLINE;
	}
}

static int unload_module(void)
{
	ast_cli_unregister_multiple(cli_ipbx, ARRAY_LEN(cli_ipbx));

	FinishDatabase();

	if (option_verbose) {
		ast_verbose(">>> IPBX  unloaded\n");
	}

	ast_module_user_hangup_all();

	return 	ast_unregister_application(app_ipbx);
}

static int reload(void)
{
	ConfigIpbx(1);
    //LoadGeneralConfigs();
	return CheckConnection();
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_GLOBAL_SYMBOLS, "IPBX MODULE",
				.load = load_module,
				.unload = unload_module,
				.reload = reload
				);
