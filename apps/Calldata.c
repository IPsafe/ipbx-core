/*
 *  Calldata.c
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#include <asterisk.h>
#include <asterisk/utils.h>
#include <asterisk/channel.h>
#include <asterisk/pbx.h>

#include "Calldata.h"



int InitCalldata(Calldata *calldata, struct ast_channel *chan){
    int ret = 0;
    char *num;

	ast_channel_lock(chan);

    num = S_COR(ast_channel_caller(chan)->id.number.valid, ast_channel_caller(chan)->id.number.str,
                S_COR(ast_channel_caller(chan)->id.number.valid, ast_channel_caller(chan)->id.number.str, NULL));
    
    ast_copy_string(calldata->clid, S_OR(num, ""), sizeof(calldata->clid));


	ast_copy_string(calldata->clid, S_OR(num, ""), sizeof(calldata->clid));




	ast_copy_string(calldata->origin, "", sizeof(calldata->origin));
    
	ast_copy_string(calldata->destination,"",sizeof(calldata->destination)-1);
	ast_copy_string(calldata->destination2,"",sizeof(calldata->destination2)-1);
    ast_copy_string(calldata->forward,"",sizeof(calldata->forward));
	
	ast_copy_string(calldata->blindtransfer,"",sizeof(calldata->blindtransfer));
	ast_copy_string(calldata->transferername,"",sizeof(calldata->transferername));
	
    ast_copy_string(calldata->uniqueid,"",sizeof(calldata->uniqueid));
    ast_copy_string(calldata->linkedid,"",sizeof(calldata->linkedid));
	
	ast_copy_string(calldata->peerip,"", sizeof(calldata->peerip));
	ast_copy_string(calldata->recvip,"", sizeof(calldata->recvip));
	ast_copy_string(calldata->peername,"", sizeof(calldata->peername));
	ast_copy_string(calldata->uri,"", sizeof(calldata->uri));
	ast_copy_string(calldata->useragent,"", sizeof(calldata->useragent));
	
    calldata->duration = 0;
	calldata->duration2 = 0;
	
    ast_copy_string(calldata->type_call,"",sizeof(calldata->type_call));
	
	ast_copy_string(calldata->hangup_cause,"",sizeof(calldata->hangup_cause));
	
	ast_copy_string(calldata->observation,"",sizeof(calldata->observation));
    
	calldata->transference = 0;
			
	ast_channel_unlock(chan);
	
    return ret;
}

void LoadCalldata(Calldata *calldata, struct ast_channel *chan)
{
	char temp[512];
	
	char *orig;
	char tmpchan[256];
    
//	ast_copy_string(orig, "", sizeof(orig));
	ast_copy_string(tmpchan, "", sizeof(tmpchan));
	
	//Loading Calldata
	
	ast_channel_lock(chan);
	
	//pbx_substitute_variables_helper(chan, "${CHANNEL}", ast_channel_name(chan), sizeof(calldata->channel));
	strcpy(calldata->channel, ast_channel_name(chan));
	
    //Destination

    ast_copy_string(calldata->destination, ast_channel_exten(chan), sizeof(calldata->destination)-1);
    ast_copy_string(calldata->destination2, ast_channel_exten(chan), sizeof(calldata->destination2)-1);
    
	if(strcmp(ast_channel_tech(chan)->type,"Local"))
	{
		pbx_substitute_variables_helper(chan, "${CHANNEL(peerip)}", calldata->peerip, sizeof(calldata->peerip));
		pbx_substitute_variables_helper(chan, "${CHANNEL(recvip)}", calldata->recvip, sizeof(calldata->recvip));
		pbx_substitute_variables_helper(chan, "${CHANNEL(peername)}", calldata->peername, sizeof(calldata->peername));
		pbx_substitute_variables_helper(chan, "${CHANNEL(uri)}", calldata->uri, sizeof(calldata->uri));
		pbx_substitute_variables_helper(chan, "${CHANNEL(useragent)}", calldata->useragent, sizeof(calldata->useragent));
	}


	pbx_substitute_variables_helper(chan, "${FROM_IP}", calldata->recvip, sizeof(calldata->recvip));
	pbx_substitute_variables_helper(chan, "${BLINDTRANSFER}", calldata->blindtransfer, sizeof(calldata->blindtransfer));
	pbx_substitute_variables_helper(chan, "${TRANSFERERNAME}", calldata->transferername, sizeof(calldata->transferername));
	
    ast_copy_string(calldata->origin, calldata->peername, sizeof(calldata->origin));    	


	if (!strcmp(ast_channel_tech(chan)->type,"Local"))
	{	
		
		ast_copy_string(tmpchan ,calldata->transferername, sizeof(tmpchan));
		
		if ((orig = strchr(tmpchan, '-'))) {
			*orig++ = '\0';
            
		}
		if ((orig = strchr(tmpchan, '/'))) {
			*orig++ = '\0';
		}
        
		ast_copy_string(calldata->origin, orig, sizeof(calldata->origin)-1);
	}
	
	ast_copy_string(calldata->uniqueid, ast_channel_uniqueid(chan), sizeof(calldata->uniqueid));
	ast_copy_string(calldata->linkedid, ast_channel_linkedid(chan), sizeof(calldata->linkedid));
	
	ast_channel_unlock(chan);
}

