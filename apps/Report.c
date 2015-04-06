/*
 *  Report.c
 *  ipbx_core
 *
 *  Author: Guilherme Rezende <guilhermebr@gmail.com>
 *  Copyright (C) 2010 Guilherme Bessa Rezende
 */

#include <asterisk.h>

#include <sys/time.h>
#include <asterisk/utils.h>

#include "Report.h"
#include "Database.h"

int SHOW_QUERIES;
int IPBX_DEBUG;

int InitReport(Report *report) {

	struct tm *tm;
	time_t t = time(NULL);

	tm = localtime(&t);
	strftime(report->calldate, sizeof(report->calldate), DATE_R_FORMAT, tm);

	strncpy(report->origin, "", sizeof(report->origin));
	strncpy(report->forward, "", sizeof(report->forward));
	strncpy(report->status, "", sizeof(report->status));
	strncpy(report->user_name, "", sizeof(report->user_name));
	strncpy(report->exten_description, "", sizeof(report->exten_description));
	strncpy(report->call_type, "", sizeof(report->call_type));
	strncpy(report->observation, "", sizeof(report->observation));
	strncpy(report->uniqueid, "", sizeof(report->uniqueid));
	strncpy(report->linkedid, "", sizeof(report->linkedid));
	strncpy(report->uri, "", sizeof(report->uri));

	report->duration = 0;
	report->duration_billing = 0;

	report->value = 0;
	report->sequence = 0;

	strncpy(report->destination, "", sizeof(report->destination));
	strncpy(report->provider, "", sizeof(report->provider));

	report->origin_user_id = 0;
	report->destination_user_id = 0;
	report->origin_exten_id = 0;
	report->destination_exten_id = 0;
	report->queue_id = 0;
	report->ivr_id = 0;
	report->provider_id = 0;
	//report->rotaentrada_provider_id = 0;

	return 0;
}

int SaveReportOutbound(Report *report, Calldata *calldata, Outbound *outbound) {

	if (!calldata) {
		return -1;
	}

	ast_copy_string(report->origin, outbound->device_origin.exten,
			sizeof(report->origin));

	if (strcmp(outbound->device_origin.description, ""))
		ast_copy_string(report->exten_description,
				outbound->device_origin.description,
				sizeof(report->exten_description));

	report->origin_user_id = outbound->device_origin.user_id;

	// else ast_copy_string(report->name, calldata->clid, sizeof(report->name));

	if (strcmp(outbound->device_origin.user_name, ""))
		ast_copy_string(report->user_name, outbound->device_origin.user_name,
				sizeof(report->user_name));

	report->origin_exten_id = outbound->device_origin.id;

	if (strcmp(calldata->destination2, ""))
		ast_copy_string(report->destination, calldata->destination2,
				sizeof(report->destination));

	ast_copy_string(report->provider, outbound->provider.name,
			sizeof(report->provider));

	report->provider_id = outbound->provider.id;
	report->duration = calldata->duration;
	report->duration_billing = calldata->duration2;
	report->value = outbound->value;

	ast_copy_string(report->call_type, "Outbound", sizeof(report->call_type));

	if (strcmp(calldata->observation, "")) {
		ast_copy_string(report->observation, calldata->observation,
				sizeof(report->observation));
	}

	ast_copy_string(report->status, calldata->hangup_cause,
			sizeof(report->status));
	ast_copy_string(report->uniqueid, calldata->uniqueid,
			sizeof(report->uniqueid));
	ast_copy_string(report->linkedid, calldata->linkedid,
			sizeof(report->linkedid));
	ast_copy_string(report->uri, calldata->uri, sizeof(report->uri));

	if (calldata->transference) report->transference = 1;

	return 0;

}

int SaveReportInbound(Report *report, Calldata *calldata, Inbound *inbound) {

	if (!calldata) {
		return -1;
	}

	ast_copy_string(report->origin, calldata->origin, sizeof(report->origin));
	ast_copy_string(report->destination, calldata->destination,
			sizeof(report->destination));
	ast_copy_string(report->forward, calldata->forward,
			sizeof(report->forward));
	ast_copy_string(report->provider, inbound->provider.name,
			sizeof(report->provider));
	report->provider_id = inbound->provider.id;

	report->duration = calldata->duration;

	ast_copy_string(report->call_type, "Inbound", sizeof(report->call_type));

	if (strcmp(calldata->observation, "")) {
		ast_copy_string(report->observation, calldata->observation,
				sizeof(report->observation));
	}

	ast_copy_string(report->status, calldata->hangup_cause,
			sizeof(report->status));
	ast_copy_string(report->uniqueid, calldata->uniqueid,
			sizeof(report->uniqueid));
	ast_copy_string(report->linkedid, calldata->linkedid,
			sizeof(report->linkedid));
	ast_copy_string(report->uri, calldata->uri, sizeof(report->uri));

	if (inbound->device_destination.id) {
		report->destination_exten_id = inbound->device_destination.id;
		report->destination_user_id = inbound->device_destination.user_id;
		ast_copy_string(report->user_name,
				inbound->device_destination.user_name,
				sizeof(report->user_name));
		ast_copy_string(report->exten_description,
				inbound->device_destination.description,
				sizeof(report->exten_description));

	}

	return 0;

}

int SaveReportInternal(Report *report, Calldata *calldata, Internal *internal) {
	if (!calldata) {
		return -1;
	}

	ast_copy_string(report->origin, internal->device_origin.exten,
			sizeof(report->origin));

	if (strcmp(internal->device_origin.user_name, ""))
		ast_copy_string(report->user_name, internal->device_origin.user_name,
				sizeof(report->user_name));

	if (strcmp(internal->device_origin.description, ""))
		ast_copy_string(report->exten_description,
				internal->device_origin.description,
				sizeof(report->exten_description));

	report->origin_user_id = internal->device_origin.user_id;
	report->origin_exten_id = internal->device_origin.id;

	if (strcmp(internal->device_destination.exten, "")) {
		ast_copy_string(report->destination, internal->device_destination.exten,
				sizeof(report->destination));
		report->destination_exten_id = internal->device_destination.id;
		report->destination_user_id = internal->device_destination.user_id;

	} else {
		ast_copy_string(report->destination, calldata->destination,
				sizeof(report->destination));
	}
	if (calldata->forward) {
		ast_copy_string(report->forward, calldata->forward,
				sizeof(report->forward));
		report->duration = calldata->duration;

	}

	ast_copy_string(report->call_type, "Internal", sizeof(report->call_type));

	if (strcmp(calldata->observation, "")) {
		ast_copy_string(report->observation, calldata->observation,
				sizeof(report->observation));
	}

	ast_copy_string(report->status, calldata->hangup_cause,
			sizeof(report->status));
	ast_copy_string(report->uniqueid, calldata->uniqueid,
			sizeof(report->uniqueid));
	ast_copy_string(report->linkedid, calldata->linkedid,
			sizeof(report->linkedid));
	ast_copy_string(report->uri, calldata->uri, sizeof(report->uri));

	return 0;

}

int SaveReport(Report *report) {

	int ret = 0;
	struct ast_str *sql = ast_str_create(MAXSIZE);

	if (!sql) {
		ast_free(sql);
		return -1;
	}

	ast_verbose(VERBOSE_PREFIX_2 "::IPBX CDR::\n");
	if (report->transference) {
	ast_verbose(VERBOSE_PREFIX_3 "Transference Call: %s\n", report->uri);
	}
	ast_verbose(VERBOSE_PREFIX_3 "Calldate: %s\n", report->calldate);
	ast_verbose(VERBOSE_PREFIX_3 "Origin: %s\n", report->origin);
	ast_verbose(VERBOSE_PREFIX_3 "Exten CallerID: %s\n", report->exten_description);
	ast_verbose(VERBOSE_PREFIX_3 "User Name: %s\n", report->user_name);
	ast_verbose(VERBOSE_PREFIX_3 "User ID: %d\n", report->origin_user_id);
	ast_verbose(VERBOSE_PREFIX_3 "Exten Description: %s\n", report->exten_description);
	ast_verbose(VERBOSE_PREFIX_3 "Destination: %s\n", report->destination);
	ast_verbose(VERBOSE_PREFIX_3 "Forward: %s\n", report->forward);
	ast_verbose(VERBOSE_PREFIX_3 "Provider: %s\n", report->provider);
	ast_verbose(VERBOSE_PREFIX_3 "Duration: %d\n", report->duration);
	ast_verbose(VERBOSE_PREFIX_3 "Billing Duration: %d\n", report->duration_billing);
	ast_verbose(VERBOSE_PREFIX_3 "Value: %.3f\n", report->value);
	ast_verbose(VERBOSE_PREFIX_3 "Status: %s\n", report->status);
	ast_verbose(VERBOSE_PREFIX_3 "Call Type: %s\n", report->call_type);
	ast_verbose(VERBOSE_PREFIX_3 "Observation: %s\n", report->observation);
	ast_verbose(VERBOSE_PREFIX_3 "Uniqueid: %s\n", report->uniqueid);
	ast_verbose(VERBOSE_PREFIX_3 "Linkedid: %s\n", report->linkedid);
	ast_verbose(VERBOSE_PREFIX_3 "URI: %s\n", report->uri);
	ast_verbose(VERBOSE_PREFIX_2 "::::::::::::::::::::::::::::::::::::::::::::::\n");

	if (!strcmp(report->call_type, "Internal")) {
		if (report->transference) ast_copy_string(report->call_type, "Transference", sizeof(report->call_type));
		ast_str_set(&sql, 0,
				"INSERT INTO report (user_id, origin, destination, forward, status, duration, calldate, name, call_type, observation, uniqueid, linkedid, uri, exten_description) ");
		ast_str_append(&sql, 0,
				"VALUES ('%d', '%s', '%s', '%s', '%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s' )",
				report->origin_user_id, report->origin, report->destination,
				report->forward, report->status, report->duration,
				report->calldate, report->user_name, report->call_type,
				report->observation, report->uniqueid, report->linkedid, report->uri,
				report->exten_description);

	} else if (!strcmp(report->call_type, "Outbound")) {
		ast_str_set(&sql, 0,
				"INSERT INTO report (user_id, origin, destination, forward, status, duration, duration_billing, calldate, provider, name, value, call_type, observation, uniqueid, linkedid, uri, exten_description) ");
		ast_str_append(&sql, 0,
				"VALUES ('%d', '%s', '%s', '%s', '%s', '%d', '%d', '%s', '%s', '%s', '%.3f', '%s', '%s', '%s', '%s', '%s', '%s')",
				report->origin_user_id, report->origin, report->destination,
				report->forward, report->status, report->duration,
				report->duration_billing, report->calldate, report->provider,
				report->user_name, report->value, report->call_type,
				report->observation, report->uniqueid, report->uniqueid, report->uri,
				report->exten_description);
	} else if (!strcmp(report->call_type, "Inbound")) {
		if (report->transference) ast_copy_string(report->call_type, "Transference", sizeof(report->call_type));
		ast_str_set(&sql, 0,
				"INSERT INTO report (user_id, origin, destination, forward, status, duration, calldate, provider, call_type, observation, uniqueid, linkedid, uri, name, exten_description, provider_id) ");
		ast_str_append(&sql, 0,
				"VALUES ((SELECT user_id FROM devices WHERE id = %d), '%s', '%s', '%s', '%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d' )",
				report->provider_id, report->origin, report->destination,
				report->forward, report->status, report->duration,
				report->calldate, report->provider, report->call_type,
				report->observation, report->uniqueid, report->uniqueid, report->uri,
				report->user_name, 
				report->exten_description,				
				report->provider_id);
	}

	if (SHOW_QUERIES)
		ast_log(LOG_NOTICE, "SQL: %s\n", ast_str_buffer(sql));

	ret = insertQuery(sql);

	if (ret == 0) {
		if (IPBX_DEBUG)
			ast_log(LOG_NOTICE, "Report Saved.\n");
	} else {
		if (IPBX_DEBUG)
			ast_log(LOG_NOTICE, "Error Saving Report!!\n");
	}

	ast_free(sql);

	return ret;

}
