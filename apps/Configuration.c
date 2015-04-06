/*
 *  Configuration.c
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#include <asterisk.h>
#include <asterisk/utils.h>
#include <asterisk/channel.h>

#include "Configuration.h"
#include "Database.h"

static char *config = "ipbx.conf";

int SHOW_QUERIES;
int IPBX_DEBUG;

PGresult *result;
PGresult *result2;

struct GlobalConfig {
    char example[60];

} globalconfig;

/*
 * LoadGeneralConfig will be used to load general data, 
 * like path to record files.
 */

int LoadGeneralConfig()
{
    
	int ret = 0;
	struct ast_str *sql = ast_str_create(MAXSIZE);
    char key[32] = "";
	
	if (!sql) {
		ast_free(sql);
		return -1;
	}
    
    ast_str_set(&sql, 0, "SELECT key, value FROM general_config");
    
    if (SHOW_QUERIES) ast_log(LOG_NOTICE, "SQL: %s\n",ast_str_buffer(sql));
    
    result = executeQuery(sql);
    
    if (result != NULL)
    {					        
        int lines = PQntuples(result);        
        int n_line;
        
        for (n_line = 0; n_line < lines; n_line++)
        {
            
            ast_copy_string(key, getResultByField(result, "key", n_line), sizeof(key));
            if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Key: %s\n", key);
            
            //Example how to use this.
            if (!(strcmp(key, "EXAMPLE")))
            {
                ast_copy_string(globalconfig.example, getResultByField(result, "value", n_line), sizeof(globalconfig.example));
                    if (IPBX_DEBUG) ast_log(LOG_NOTICE, "Value: %s\n",globalconfig.example);      
            }
        }
        
    }
    
}

int ConfigIpbx(int reload)
{
	struct ast_variable *var;
	//	struct columns *cur;
	//	PGresult *result;
	const char *tmp;
    
	struct ast_config *cfg;
	struct ast_flags config_flags = { reload ? CONFIG_FLAG_FILEUNCHANGED : 0 };
	
	if ((cfg = ast_config_load(config, config_flags)) == NULL || cfg == CONFIG_STATUS_FILEINVALID) {
		ast_log(LOG_WARNING, "Unable to load config for IPBX PostgreSQL: %s\n", config);
		return -1;
	} else if (cfg == CONFIG_STATUS_FILEUNCHANGED)
		return 0;
	
	if (!(var = ast_variable_browse(cfg, "global"))) {
		ast_config_destroy(cfg);
		return 0;
	}
	
	if (!(tmp = ast_variable_retrieve(cfg, "global", "hostname"))) {
		ast_log(LOG_WARNING, "PostgreSQL server hostname not specified.  Assuming unix socket connection\n");
		tmp = "";	/* connect via UNIX-socket by default */
	}
	
	if (pghostname)
		ast_free(pghostname);
	if (!(pghostname = ast_strdup(tmp))) {
		ast_config_destroy(cfg);
		return -1;
	}
	
	if (!(tmp = ast_variable_retrieve(cfg, "global", "dbname"))) {
		ast_log(LOG_WARNING, "PostgreSQL database not specified.  Assuming ipbx\n");
		tmp = "ipbx";
	}
	
	if (pgdbname)
		ast_free(pgdbname);
	if (!(pgdbname = ast_strdup(tmp))) {
		ast_config_destroy(cfg);
		return -1;
	}
	
	if (!(tmp = ast_variable_retrieve(cfg, "global", "user"))) {
		ast_log(LOG_WARNING, "PostgreSQL database user not specified.  Assuming ipsafe\n");
		tmp = "ipsafe";
	}
	
	if (pgdbuser)
		ast_free(pgdbuser);
	if (!(pgdbuser = ast_strdup(tmp))) {
		ast_config_destroy(cfg);
		return -1;
	}
	
	if (!(tmp = ast_variable_retrieve(cfg, "global", "password"))) {
		ast_log(LOG_WARNING, "PostgreSQL database password not specified.  Assuming default\n");
		tmp = "--ipsafe#db";
	}
	
	if (pgpassword)
		ast_free(pgpassword);
	if (!(pgpassword = ast_strdup(tmp))) {
		ast_config_destroy(cfg);
		return -1;
	}
	
	if (!(tmp = ast_variable_retrieve(cfg, "global", "port"))) {
		ast_log(LOG_WARNING, "PostgreSQL database port not specified.  Using default 5432.\n");
		tmp = "5432";
	}
	
	if (pgdbport)
		ast_free(pgdbport);
	if (!(pgdbport = ast_strdup(tmp))) {
		ast_config_destroy(cfg);
		return -1;
	}
	
	ast_config_destroy(cfg);
	return 1;
}
