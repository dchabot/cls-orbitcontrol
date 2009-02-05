/*
 * Device Support for OCM dac and dac:fbk values
 *
 *  Created on: Nov 19, 2008
 *      Author: chabotd
 */
#include <longoutRecord.h>
#include <longinRecord.h>
#include <dbCommon.h>
#include <stdlib.h>
#include <ctype.h>
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
#include <stdexcept>
using std::runtime_error;
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
} devSupOCMLongOut={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_longout,
};

epicsExportAddress(dset,devSupOCMLongOut);

enum loType {setpoint,xStep,yStep};

struct OcmLongoutData {
	loType type;
	Ocm *ocm;
};

static long
init_record(void* lor) {
	longoutRecord* lorp = (longoutRecord*)lor;
	OcmController *ocmCtlr = OrbitController::getInstance();

	/*chk OUT type:*/
	if (lorp->out.type != INST_IO) {
		syslog(LOG_INFO, "%s: OUT field type should be INST_IO\n", lorp->name);
		return (S_db_badField);
	}

	//check OCM LongOut loType:
	int needOcm = !isalpha((int)lorp->out.value.instio.string[0]);
	if(needOcm) {
		//this is a "setpoint"-type record
		/* strip off the ":dac" from the record name; this will form the OCM's id */
		string name(lorp->name);
		size_t pos = name.find(":dac");
		string id = name.substr(0,pos);
		/* <rant> WTF doesn't c++ have a string tokenizer method ?!?!? </rant> */
		char cbuf[128] = {0};
		strncpy(cbuf,lorp->out.value.instio.string,sizeof(cbuf)/sizeof(cbuf[0]));
		/* parse out the params */
		uint32_t crateId = strtoul(strtok(cbuf," "),NULL,10);
		uint32_t vmeBaseAddr = strtoul(strtok(NULL," "),NULL,16);
		uint8_t channel = (epicsUInt8)strtoul(strtok(NULL," "),NULL,10);
		uint32_t position = strtoul(strtok(NULL," "),NULL,10);
		Ocm *ocm = ocmCtlr->registerOcm(id,crateId,vmeBaseAddr,channel, position);
		if(ocm==NULL) {
			syslog(LOG_INFO, "%s: failure creating OCM %s!!!\n",lorp->name,id.c_str());
			return -1;
		}
		OcmLongoutData *old = new OcmLongoutData();
		old->type = setpoint;
		old->ocm = ocm;
		lorp->dpvt = (void*)old;
	}
	else {
		//this is an xStep or yStep loType. Use OcmController interface only (no Ocm instance).
		syslog(LOG_INFO, "%s.OUT=%s\n",lorp->name,lorp->out.value.instio.string);
		string type(lorp->out.value.instio.string);
		OcmLongoutData *old = NULL;
		if(type.compare("xStep")==0) {
			 old = new OcmLongoutData();
			 old->type = xStep;
		}
		else if(type.compare("yStep")==0) {
			old = new OcmLongoutData();
			old->type = yStep;
		}
		else {
			type.append(": unknown OUT type!!! WTF ?!?!?!?");
			throw runtime_error(type.c_str());
		}
		lorp->dpvt = (void*)old;
	}
	return 0;
}

static long
write_longout(void* lor) {
	longoutRecord* lorp = (longoutRecord*)lor;
	OcmLongoutData *old = (OcmLongoutData*)lorp->dpvt;
	OcmController *ocmCtlr = OrbitController::getInstance();

	switch(old->type) {
		case(setpoint):
#ifdef OC_DEBUG
			syslog(LOG_INFO, "Setting single channel, %s to %d\n",lorp->name,lorp->val);
#endif
			ocmCtlr->setOcmSetpoint(old->ocm, lorp->val);
			break;
		case(xStep):
			ocmCtlr->setMaxHorizontalStep(lorp->val);
			break;
		case(yStep):
			ocmCtlr->setMaxVerticalStep(lorp->val);
			break;
		default:
			syslog(LOG_INFO,"%s: unknown loType=%i !!\n",lorp->name,old->type);
			return -1;
	}
	lorp->udf=0;
	return 0;
}


static long init_lirecord(void* lir);
static long read_longin(void* lir);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   read_longin;
} devSupOCMLongIn={
    5,
    NULL,
    NULL,
    init_lirecord,
    NULL,
    read_longin,
};

epicsExportAddress(dset,devSupOCMLongIn);

static long init_lirecord(void* lir) {
	longinRecord* lirp = (longinRecord*)lir;
	OcmController *ocmCtlr = OrbitController::getInstance();

	/*chk OUT type:*/
	if (lirp->inp.type != INST_IO) {
		syslog(LOG_INFO, "%s: INP field type should be INST_IO\n", lirp->name);
		return (S_db_badField);
	}

	string name(lirp->name);
	size_t pos = name.find(":dac:fbk");
	string id = name.substr(0,pos);
	char cbuf[128] = {0};
	strncpy(cbuf,lirp->inp.value.instio.string,sizeof(cbuf)/sizeof(cbuf[0]));
	/* parse out the params */
	uint32_t crateId = strtoul(strtok(cbuf," "),NULL,10);
	uint32_t vmeBaseAddr = strtoul(strtok(NULL," "),NULL,16);
	uint8_t channel = (epicsUInt8)strtoul(strtok(NULL," "),NULL,10);
	uint32_t position = strtoul(strtok(NULL," "),NULL,10);
	Ocm *ocm = ocmCtlr->registerOcm(id,crateId,vmeBaseAddr,channel,position);
	if(ocm==NULL) {
		syslog(LOG_INFO, "%s: failure creating %s!!!\n",lirp->name,id.c_str());
		return -1;
	}
	lirp->dpvt = (void*)ocm;
	return 0;
}

static long read_longin(void* lir) {
	longinRecord *lirp = (longinRecord*)lir;
	Ocm *ocm = (Ocm*)lirp->dpvt;

	lirp->val = ocm->getSetpoint();
	return 0;
}

#ifdef __cplusplus
}
#endif
