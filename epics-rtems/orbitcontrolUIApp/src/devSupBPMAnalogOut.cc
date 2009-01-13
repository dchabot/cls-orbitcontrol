/*
 * devSupBPMAnalogOut.cc
 *
 *  Created on: Jan 8, 2009
 *      Author: chabotd
 */
#include <aoRecord.h>
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
#include <stdexcept>
using std::runtime_error;
using std::string;

/* some routines called from "C": avoid name-mangling */
#ifdef __cplusplus
	extern "C" {
#endif

static long init_record(void* aor);
static long write_ao(void* aor);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   write_ao;
} devSupBPMAnalogOut={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_ao,
};

epicsExportAddress(dset,devSupBPMAnalogOut);

enum aoType {xRef,yRef,xOffs,yOffs};

struct aoData {
	aoData(Bpm* b, aoType t):bpm(b),type(t){};
	~aoData(){};
	Bpm* bpm;
	aoType type;
};

static aoType getRecType(string& type) {
	if(type.compare("xreference")==0) { return xRef; }
	else if(type.compare("yreference")==0) { return yRef; }
	else if(type.compare("xoffset")==0) { return xOffs; }
	else if(type.compare("yoffset")==0) { return yOffs; }
	else {
		type.append(": unknown record type!!! WTF ?!?!?!?");
		throw runtime_error(type.c_str());
	}
}


static long init_record(void* aor) {
	aoRecord *aop = (aoRecord*)aor;

	if(aop->out.type != INST_IO) {
		syslog(LOG_INFO, "%s: OUT field type should be INST_IO\n", aop->name);
		return(S_db_badField);
	}

	BpmController* bpmctlr = OrbitController::getInstance();
	string id(aop->name);
	size_t pos = id.find_first_of(":");
	Bpm *bpm = bpmctlr->getBpm(id.substr(0,pos));

	if(bpm == 0) {
		syslog(LOG_INFO, "%s: can't find BPM object with id=%s\n!!!",
						aop->name,id.substr(0,pos).c_str());
		return -1;
	}

	string type(aop->out.value.instio.string);
	syslog(LOG_INFO, "%s: OUT=%s\n",bpm->getId().c_str(),type.c_str());
	try {
		aoData *aod = new aoData(bpm,getRecType(type));
		//hook our aoData object into this record instance
		aop->dpvt = (void*)aod;
	}
	catch(runtime_error& err) {
		syslog(LOG_INFO, "%s",err.what());
		return -1;
	}
	return 0;
}

static long write_ao(void* aor) {
	aoRecord *aop = (aoRecord*)aor;
	aoData *aod = (aoData*)aop->dpvt;

	switch(aod->type) {
		case(xRef):
			aod->bpm->setXRef(aop->val);
			return 0;
		case(yRef):
			aod->bpm->setYRef(aop->val);
			return 0;
		case(xOffs):
			aod->bpm->setXOffs(aop->val);
			return 0;
		case(yOffs):
			aod->bpm->setYOffs(aop->val);
			return 0;
		default:
			syslog(LOG_INFO,"devSupBPMAnalogOut: write_ao() default case, AAARRRGGG!!!\n");
			return -1;
	}
}

#ifdef __cplusplus
}
#endif
