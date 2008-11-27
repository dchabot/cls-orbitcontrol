/*
 * devSupOCMSetpoint.c
 *
 *  Created on: Nov 19, 2008
 *      Author: chabotd
 */
#include <longoutRecord.h>
#include <dbCommon.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <devSup.h>

#include <dbDefs.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <epicsExport.h>

#include <syslog.h>

static long init_record(struct longoutRecord* lor);
static long write_longout(struct longoutRecord* lor);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   write_longout;
} devSupOCMSetpoint={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_longout,
};

epicsExportAddress(dset,devSupOCMSetpoint);

typedef struct {
	epicsUInt8 	crateId;
	epicsUInt32 vmeBaseAddr;
	epicsUInt8 	channel;
} OcmSetpointPvt;

static long
init_record(struct longoutRecord* lor) {
	#define cbufSize 128
	char cbuf[cbufSize] = {0};
	char *instioStr = NULL;
	OcmSetpointPvt *pvt = NULL;

	/*chk INP type:*/
	if (lor->out.type != INST_IO) {
		printf("%s: OUT field type should be INST_IO\n", lor->name);
		return (S_db_badField);
	}

	pvt = (OcmSetpointPvt*)calloc(1, sizeof(OcmSetpointPvt));
	if(pvt == NULL) {
		printf("Can't alloc-mem for OcmSetpointPvt!!\n");
		return -1;
	}

	instioStr = lor->out.value.instio.string;
	printf("%s->out=%s\n",lor->name,instioStr);
	strncpy(cbuf,instioStr,cbufSize-1);
	/* parse out the params */
	pvt->crateId = (epicsUInt8)strtoul(strtok(cbuf," "),NULL,10);
	pvt->vmeBaseAddr = strtoul(strtok(NULL," "),NULL,16);
	pvt->channel = (epicsUInt8)strtoul(strtok(NULL," "),NULL,10);

	lor->dpvt = pvt;
	return 0;
}

static long
write_longout(struct longoutRecord* lor) {
	OcmSetpointPvt* pvt = lor->dpvt;

	return 0;
}
