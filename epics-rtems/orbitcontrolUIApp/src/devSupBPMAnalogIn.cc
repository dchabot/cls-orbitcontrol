/*
 * devSupBPMAnalogIn.cc
 *
 *  Created on: Jan 8, 2009
 *      Author: chabotd
 */
#include <aiRecord.h>
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

static long init_record(void* air);
static long read_ai(void* air);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   read_ai;
} devSupBPMAnalogIn={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    read_ai,
};

epicsExportAddress(dset,devSupBPMAnalogIn);

enum aiType {xval,yval};

struct aiData {
	aiData(Bpm* b, aiType t):bpm(b),type(t){};
	~aiData(){};
	Bpm* bpm;
	aiType type;
};

static aiType getRecType(string& type) {
	if(type.compare("xval")==0) { return xval; }
	else if(type.compare("yval")==0) { return yval; }
	else {
		type.append(": unknown record type!!! WTF ?!?!?!?");
		throw runtime_error(type.c_str());
	}
}

static long init_record(void* air) {
	aiRecord *aip = (aiRecord*)air;

	if(aip->inp.type != INST_IO) {
		syslog(LOG_INFO, "%s: INP field type should be INST_IO\n", aip->name);
		return(S_db_badField);
	}

	OrbitController *oc = OrbitController::getInstance();
	BpmController* bpmctlr = oc->getBpmController();
	string id(aip->name);
	size_t pos = id.find_first_of(":");
	Bpm *bpm = bpmctlr->getBpm(id.substr(0,pos));

	if(bpm == 0) {
		syslog(LOG_INFO, "%s: can't find BPM object with id=%s\n!!!",
						aip->name,id.substr(0,pos).c_str());
		return -1;
	}

	string type(aip->inp.value.instio.string);
	syslog(LOG_INFO, "%s: INP=%s\n",bpm->getId().c_str(),type.c_str());
	try {
		aiData *aid = new aiData(bpm,getRecType(type));
		//FIXME -- what happens if aip->aslo is changed via caput ??
		// Nothing: that's what!! Should refactor UI-controlled Bpm class-attributes
		// into *pointers*. That way we could hook record fields into each object instance...
		if(aid->type==xval) { bpm->setXVoltsPerMilli(aip->aslo); }
		else { bpm->setYVoltsPerMilli(aip->aslo); }
		//hook our aiData object into this record instance
		aip->dpvt = (void*)aid;
	}
	catch(runtime_error& err) {
		syslog(LOG_INFO, "%s",err.what());
		return -1;
	}

	return 0;
}

static long read_ai(void* air) {
	aiRecord *aip = (aiRecord*)air;
	aiData *aid = (aiData*)aip->dpvt;

	if(aid->type == xval) { aip->val = aid->bpm->getX(); }
	else { aip->val = aid->bpm->getY(); }

	return 0;
}

#ifdef __cplusplus
}
#endif

