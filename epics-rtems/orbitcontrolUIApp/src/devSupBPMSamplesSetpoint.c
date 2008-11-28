/*
 * devSupBPMSamplesSetpoint.c
 *
 *  Created on: Nov 28, 2008
 *      Author: chabotd
 */

#include <longoutRecord.h>
#include <dbCommon.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
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
} devSupBPMSamplesSetpoint={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_longout,
};

epicsExportAddress(dset,devSupBPMSamplesSetpoint);


static long
init_record(struct longoutRecord* lor) {

	/*chk INP type:*/
	if (lor->out.type != INST_IO) {
		syslog(LOG_INFO,"%s: OUT field type should be INST_IO\n", lor->name);
		return (S_db_badField);
	}
	syslog(LOG_INFO,"%s->out=%s\n",lor->name,lor->out.value.instio.string);

	return 0;
}

static long
write_longout(struct longoutRecord* lor) {
	extern uint32_t SamplesPerAvg;

	if((lor->val < lor->hopr) && (lor->val > lor->lopr)) {
		SamplesPerAvg = lor->val;
	}
	else {
		syslog(LOG_INFO," val %d outside acceptable range!!\n",lor->val);
		return -1;
	}

	return 0;
}
