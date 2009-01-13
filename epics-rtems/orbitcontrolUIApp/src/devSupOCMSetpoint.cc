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
#include <OrbitController.h>
#include <string>
using std::string;

/* some routines called from "C": avoid name-mangling */
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
} devSupOCMSetpoint={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_longout,
};

epicsExportAddress(dset,devSupOCMSetpoint);

static long
init_record(void* lor) {
	longoutRecord* lorp = (longoutRecord*)lor;
	/*chk INP type:*/
	if (lorp->out.type != INST_IO) {
		syslog(LOG_INFO, "%s: OUT field type should be INST_IO\n", lorp->name);
		return (S_db_badField);
	}

	/* strip off the ":dac" from the record name; this will form the OCM's id */
	string name(lorp->name);
	size_t pos = name.find_first_of(":dac");
	name = name.substr(0,pos);
	char cbuf[128] = {0};
	strncpy(cbuf,lorp->out.value.instio.string,sizeof(cbuf)/sizeof(cbuf[0]));
	/* <rant> WTF doesn't c++ have a string tokenizer method ?!?!? </rant> */
	/* parse out the params */
	uint32_t crateId = strtoul(strtok(cbuf," "),NULL,10);
	uint32_t vmeBaseAddr = strtoul(strtok(NULL," "),NULL,16);
	uint8_t channel = (epicsUInt8)strtoul(strtok(NULL," "),NULL,10);
	OcmController *ocmCtlr = OrbitController::getInstance();
	Ocm *ocm = ocmCtlr->registerOcm(name,crateId,vmeBaseAddr,channel);
	if(ocm == NULL) {
		syslog(LOG_INFO, "devSupOCMSetpoint: %s -- couldn't register OCM!!\n",lorp->name);
		return -1;
	}
	lorp->dpvt = (void*)ocm;
	return 0;
}

static long
write_longout(void* lor) {
	longoutRecord* lorp = (longoutRecord*)lor;
	Ocm *ocm = (Ocm*)lorp->dpvt;
	OcmController *ocmCtlr = OrbitController::getInstance();

	syslog(LOG_INFO, "Setting single channel, %s to %d\n",lorp->name,lorp->val);

	ocmCtlr->setOcmSetpoint(ocm, lorp->val);
	return 0;//SetSingleSetpoint(pvt->ctlr, lorp->val);
}

#ifdef __cplusplus
}
#endif
