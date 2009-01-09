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
#include <OrbitController.h>


#ifdef __cplusplus
	extern "C" {
#endif

static long init_record(void* lor);
static long write_longout(void* lor);

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
init_record(void* lor) {
	struct longoutRecord* lorp = (longoutRecord*)lor;
	/*chk INP type:*/
	if (lorp->out.type != INST_IO) {
		syslog(LOG_INFO,"%s: OUT field type should be INST_IO\n", lorp->name);
		return (S_db_badField);
	}
	syslog(LOG_INFO,"%s->out=%s\n",lorp->name,lorp->out.value.instio.string);

	return 0;
}

static long
write_longout(void* lor) {
	struct longoutRecord* lorp = (longoutRecord*)lor;
	//extern uint32_t SamplesPerAvg;

	if((lorp->val < lorp->hopr) && (lorp->val > lorp->lopr)) {
		//SamplesPerAvg = lorp->val;
	}
	else {
		syslog(LOG_INFO," val %d outside acceptable range!!\n",lorp->val);
		return -1;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif
