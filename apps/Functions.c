/*
 *  Funcoes.c
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#include <asterisk.h>
#include <asterisk/utils.h>
#include <asterisk/channel.h>

#include "Functions.h"
#include "Database.h"

#include "Device.h"
#include "Report.h"


int SHOW_QUERIES;
int IPBX_DEBUG;
int ACTIVE_CALLS = 1;

PGresult *result;
PGresult *result2;

int IpbxDial(struct ast_channel *chan, Calldata *calldata)
{
	int ret = 0;
	int res = 0;
    
    struct ast_app *app;
    struct ast_str *sql = ast_str_create(MAXSIZE);
        
    
	if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Dial %s\n", calldata->dialargs);

    // Save to active calls list
    if (ACTIVE_CALLS) {
        struct tm *tm;
        time_t t = time(NULL);
        tm = localtime(&t);
        char calldate[120];
        strftime(calldate, sizeof(calldate), "%Y-%m-%d %T", tm);
        ast_str_set(&sql, 0, "INSERT INTO active_calls (calldate,channel,origin,destination,forward,provider,call_type,callerid,uniqueid) VALUES ('%s','%s','%s','%s','%s','%s','%s','%s','%s')",
            calldate,calldata->channel,calldata->origin,calldata->destination,calldata->forward,calldata->dialargs,calldata->type_call,calldata->clid,calldata->uniqueid);
        ast_log(LOG_NOTICE, "Calldate: %s\n",calldate);
        if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
        result = executeQuery(sql);
    }
	
	app = pbx_findapp("Dial");
	res = pbx_exec(chan, app, calldata->dialargs);
	ast_copy_string(calldata->hangup_cause, pbx_builtin_getvar_helper(chan,"DIALSTATUS"), sizeof(calldata->hangup_cause));
	if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Status = %s\n", calldata->hangup_cause);
	
    // Delete active call record
    if (ACTIVE_CALLS) {
        ast_str_set(&sql, 0, "DELETE FROM active_calls WHERE uniqueid = '%s'",calldata->uniqueid);
        if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
        result = executeQuery(sql);
    }    
	
	if(pbx_builtin_getvar_helper(chan, "ANSWEREDTIME"))
		calldata->duration = atoi(pbx_builtin_getvar_helper(chan, "ANSWEREDTIME"));
    
	
	if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Duration  = %d\n", calldata->duration);
		
	
	if(!strcmp(calldata->hangup_cause, "ANSWER"))
	{
		ast_copy_string(calldata->hangup_cause, "Answered", sizeof(calldata->hangup_cause)); 
		return 0;
	}
	
	if(!strcmp(calldata->hangup_cause, "NOANSWER"))
	{
		ast_copy_string(calldata->hangup_cause, "No Answer", sizeof(calldata->hangup_cause)); 
		return 11;
	}
	
	if(!strcmp(calldata->hangup_cause, "CHANUNAVAIL"))
	{
		ast_copy_string(calldata->hangup_cause, "No Channel", sizeof(calldata->hangup_cause)); 
		return 12;
	}
	
	if(!strcmp(calldata->hangup_cause, "BUSY"))
	{
		ast_copy_string(calldata->hangup_cause, "Busy", sizeof(calldata->hangup_cause)); 
		return 13;
	}
	
	if(!strcmp(calldata->hangup_cause, "CANCEL"))
	{
		ast_copy_string(calldata->hangup_cause, "Canceled", sizeof(calldata->hangup_cause)); 
		return 14;
	}
	
	if(!strcmp(calldata->hangup_cause, "HANGUP"))
	{
		ast_copy_string(calldata->hangup_cause, "Answered", sizeof(calldata->hangup_cause)); 
		return 15;
	}
	
	if(!strcmp(calldata->hangup_cause, "CONGESTION"))
	{
		ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause)); 
		return 16;
	}	
	
	ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause)); 
	return ret;
}


int GetDestination(struct ast_channel *chan, char *destination, Device *device, Service *service)
{
    
    char *name;
    struct ast_str *sql = ast_str_create(MAXSIZE);
	
	if (!sql) {
		ast_free(sql);
		return -1;
	}

    ast_copy_string(device->description, "", sizeof(device->description));
    ast_copy_string(device->exten, "", sizeof(device->exten));
    ast_copy_string(device->user_name, "", sizeof(device->user_name));
	device->user_id = 0;

    
    
    ast_str_set(&sql, 0, "SELECT "
    			"CASE WHEN (SELECT id FROM devices WHERE device_type = 'extension' AND name = '%s') IS NOT NULL THEN 'extension' "
    			"WHEN (SELECT id FROM conference WHERE confno = '%s') IS NOT NULL THEN 'conference' "
    			"WHEN (SELECT id FROM queues WHERE name = '%s') IS NOT NULL THEN 'queue' "
    			"END as type "
    		, destination,destination,destination);


    if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n", ast_str_buffer(sql));
    result = executeQuery(sql);
    if (result != NULL)
    {
        name = getResultByField(result, "type", 0);
			
        if (!strcmp(name, "extension"))
        {
            /*ast_str_set(&sql, 0, "SELECT ramais.id, ramais.usuario_id, usuario.name as usuario_nome, ramais.descricao, ramal, naoperturbe, sigame, "
            		"sigame_ocupado, sigame_naoatende, transbordo, cadeado, grupochamada_id, (CASE WHEN caixa_mensagem is True THEN 1 ELSE 0 END) as caixa_mensagem, "
            		"(CASE WHEN enviar_msg_email is True THEN 1 ELSE 0 END) as enviar_msg_email, "
            		"(CASE WHEN usuario.forcar_gravacao is True OR ramais.forcar_gravacao is True THEN 1 else 0 END) as gravacao_admin, "
            		"(CASE WHEN usuario.permitir_gravacao is True AND ramais.gravar_ligacoes is True THEN 1 else 0  END) as gravacao_usuario "
            		"FROM ramais LEFT JOIN usuario ON ramais.usuario_id = usuario.id  "
            		"WHERE ramal = '%s' LIMIT 1", destination);
            */
        	ast_str_set(&sql, 0, "SELECT devices.id, devices.user_id as userid, auth_user.username as username, devices.callerid, devices.name, (CASE WHEN dnd is TRUE THEN 1 END) as dnd, followme, "
        	            		"followme_busy, followme_noanswer, overflow, (CASE WHEN mailbox is NOT NULL THEN 1 ELSE 0 END) as mailbox, "
        	//            		"(CASE WHEN enviar_msg_email is True THEN 1 ELSE 0 END) as enviar_msg_email, "
        	            		"(CASE WHEN devices.force_monitor is True OR devices.force_monitor is True THEN 1 else 0 END) as monitor_admin, "
        	            		"(CASE WHEN devices.monitor is True AND devices.monitor is True THEN 1 else 0  END) as monitor_user "
        	            		"FROM devices LEFT JOIN auth_user ON devices.user_id = auth_user.id  "
        	            		"WHERE devices.name = '%s' AND devices.device_type = 'extension' LIMIT 1", destination);


            if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n", ast_str_buffer(sql));
				
            result = executeQuery(sql);
            if (result != NULL)
            {				
				device->id = atoi(getResultByField(result,"id",0));
                ast_copy_string(device->description, getResultByField(result, "callerid", 0), sizeof(device->description));
                ast_copy_string(device->exten, getResultByField(result, "name", 0), sizeof(device->exten));
                ast_copy_string(device->followme, getResultByField(result, "followme", 0), sizeof(device->followme));
                ast_copy_string(device->forward, getResultByField(result, "overflow", 0), sizeof(device->forward));
                //device->callgroup = atoi(getResultByField(result, "grupochamada_id", 0));
                device->voicemail = atoi(getResultByField(result, "mailbox", 0));
                device->dnd = atoi(getResultByField(result, "dnd", 0));
                device->user_id = atoi(getResultByField(result, "userid", 0));
                ast_copy_string(device->user_name, getResultByField(result, "username", 0), sizeof(device->user_name));
	                
            } else {
                ast_log(LOG_NOTICE, "Exten Not Found.\n", destination);
                return -1;
            }
            
            return 0;
        }
				
        else if (!strcmp(name,"conference"))
        {
                    
            ast_verbose(VERBOSE_PREFIX_3 "Conference\n");
                    
            ast_str_set(&sql, 0, "SELECT confno, pin FROM conference WHERE confno = '%s' LIMIT 1", destination);
            if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
                    
            result = executeQuery(sql);
            if (result != NULL)
            {										
                ast_copy_string(service->name, getResultByField(result, "confno", 0), sizeof(service->name));
                ast_copy_string(service->password, getResultByField(result, "pin", 0), sizeof(service->password));
                snprintf(service->args, sizeof(service->args), "%s", destination);

            } else {
                ast_log(LOG_NOTICE, "Conference Room Not Found.\n", destination);
                return -1;
            }
            
            return 1;
           					
        } 
        
        else if (!strcmp(name,"queue"))
        {
            ast_verbose(VERBOSE_PREFIX_3 "Queue\n");
            
            //ast_copy_string(service->name, getResultByField(result, "app", 0), sizeof(service->name));
            snprintf(service->args, sizeof(service->args), "%s,ntkx", destination);	

            return 2;
        }
        
        else if (!strcmp(name,"ivr"))
        {
          //Not implemented in this version
            return 3;			
        }
        
        else if (!strcmp(name,"mailbox"))
        {
            ast_verbose(VERBOSE_PREFIX_3 "Mailbox\n");
            
            return 4;
        }    
	
	}
    ast_verbose(VERBOSE_PREFIX_3 "Destination Not Found\n");

    return -1;

}


int CallExten(struct ast_channel *chan, Calldata *calldata, Device *device, Report *report, char *flag_type, Service *service)
{
    ast_verbose(VERBOSE_PREFIX_3 "Exten\n");

    char *name;
    char featureargs[62] = "";
    struct ast_app *app;

    
    int ret = 0;
    struct ast_str *sql = ast_str_create(MAXSIZE);
	
	if (!sql) {
		ast_free(sql);
		return -1;
	}

    if (strcmp(device->followme, ""))
    {
        return 1;
    }
    
    if (strcmp(device->forward, ""))
    {
        device->dial_time = 15;
    } else {
        device->dial_time = 40;
    }
    
    if (device->dnd)
    {
        ast_copy_string(calldata->observation, "Do Not Disturb", sizeof(calldata->observation));
        ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));   
        
        ret = 17;
    } else
    {
        snprintf(calldata->dialargs, sizeof(calldata->dialargs), "SIP/%s,%d,%s", device->exten ,device->dial_time, flag_type);	
             			
        ret = IpbxDial(chan, calldata);
	}	
	
			
    if((ret == 11 || ret==12 || ret==13 || ret == 14 || ret==16 || ret == 17) && strcmp(device->forward, ""))
    {
        ast_str_set(&sql, 0, "SELECT "
        			"CASE WHEN (SELECT id FROM devices WHERE device_type = 'extension' AND name = '%s') IS NOT NULL THEN 'extension' "
        			"WHEN (SELECT id FROM conference WHERE confno = '%s') IS NOT NULL THEN 'conference' "
        			"WHEN (SELECT id FROM queues WHERE name = '%s') IS NOT NULL THEN 'queue' "
        			"END as type "
        		, device->forward,device->forward,device->forward);

        result = executeQuery(sql);
       
        if (result != NULL)
        {
            name = getResultByField(result,"type",0);
            ast_verbose(VERBOSE_PREFIX_3 "type =  %s\n", name);
					
            if (!strcmp(name, "extension"))
            {
                ast_verbose(VERBOSE_PREFIX_3 "Forwarding to exten: %s \n", device->forward);
                snprintf(calldata->dialargs, sizeof(calldata->dialargs), "SIP/%s,%d,%s", device->forward, device->dial_time + 15, flag_type);	
                                        
                ret = IpbxDial(chan, calldata);
                        
            } else if (!strcmp(name,"queue"))
            {
                ast_verbose(VERBOSE_PREFIX_3 "Forwarding to Queue: %s \n", device->forward);
						
                //name = getResultByField(result,"app",0);
                snprintf(service->args, sizeof(service->args), "%s,ntkx", device->forward);		
                
                ret = CallQueue(chan, calldata, service);
                
                ast_copy_string(calldata->forward, device->forward, sizeof(calldata->forward));
                ast_copy_string(calldata->observation, "Forward Queue", sizeof(calldata->observation));
                ast_copy_string(calldata->hangup_cause, "Answered", sizeof(calldata->hangup_cause));
                
                ret = 1;
            }
					
                    
            if (ret == 0)
            {
                ast_copy_string(calldata->forward, device->forward, sizeof(calldata->forward));
                ast_copy_string(calldata->observation, "Forward", sizeof(calldata->observation));
            }
            
        } else {
            ast_copy_string(calldata->forward, device->forward, sizeof(calldata->forward));
            ast_copy_string(calldata->observation, "Forward Not Found", sizeof(calldata->observation));
            ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));
					
        }	
		
        return 0;
                
    }
			
    if((ret == 11|| ret==12 || ret==13 || ret == 14 || ret==16) && device->voicemail)
    {
        char vmargs[80];
        snprintf(vmargs, sizeof(vmargs), "%s", device->exten);		
        app = pbx_findapp("Voicemail");
        ret = pbx_exec(chan,app,vmargs);
				
        ast_copy_string(calldata->forward, device->exten, sizeof(calldata->forward));
        ast_copy_string(calldata->observation, "Mailbox", sizeof(calldata->observation));
        
        return 0;				
    }
			
}


int CallConference(struct ast_channel *chan, Calldata *calldata, Service *service)
{
    struct ast_app *app;
    int ret = 0;

    app = pbx_findapp("Answer");
    pbx_exec(chan ,app, "");
    
    if (strlen(service->password) > 0)
    {
        app = pbx_findapp("Authenticate");
        ret = pbx_exec(chan, app, service->password);
        
        if (ret != 0)
        {
            ast_copy_string(calldata->observation, "Conference - Wrong Password", sizeof(calldata->observation));
            ast_copy_string(calldata->hangup_cause, "Lost", sizeof(calldata->hangup_cause));
            return -1;
            
        }
        
    }
    
    app = pbx_findapp("ConfBridge");
    pbx_exec(chan, app, service->args);
    
    ast_copy_string(calldata->observation, "Conference", sizeof(calldata->observation));
    ast_copy_string(calldata->hangup_cause, "Answered", sizeof(calldata->hangup_cause));
    
    return 0;
    
}

int CallQueue(struct ast_channel *chan, Calldata *calldata, Service *service)
{
    
    struct ast_app *app;
    int ret = 0;

    if (ast_channel_state(chan) != AST_STATE_UP)
		ret = ast_answer(chan);




  //  app = pbx_findapp("Answer");
   // pbx_exec(chan,app,"");	
        
    app = pbx_findapp("queue");
    pbx_exec(chan, app, service->args);
    
    if(pbx_builtin_getvar_helper(chan, "ANSWEREDTIME"))
        calldata->duration = atoi(pbx_builtin_getvar_helper(chan, "ANSWEREDTIME"));
        
    ast_copy_string(calldata->observation, "Queue", sizeof(calldata->observation));
    ast_copy_string(calldata->hangup_cause, "Answered", sizeof(calldata->hangup_cause));
        
    if (IPBX_DEBUG) ast_log(LOG_NOTICE,"Duration  = %d\n", calldata->duration);

    return 0;
}