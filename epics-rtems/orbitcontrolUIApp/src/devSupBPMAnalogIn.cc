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
    DEVSUPFUN	special_linconv;
} devSupBPMAnalogIn={
    6,
    NULL,
    NULL,
    init_record,
    NULL,
    read_ai,
    NULL
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

//callback (Observer pattern) must be registered ONCE with BpmController
static void onBpmValueChange(void* arg) {
	int ev = (int)arg;
	post_event(ev);
}

static long init_record(void* air) {
	aiRecord *aip = (aiRecord*)air;
	static int once = 1;

	if(aip->inp.type != INST_IO) {
		syslog(LOG_INFO, "%s: INP field type should be INST_IO\n", aip->name);
		return(S_db_badField);
	}

	BpmController* bpmctlr = OrbitController::getInstance();
	string name(aip->name);
	size_t pos = name.find_first_of(":");
	string id = name.substr(0,pos);
	Bpm *bpm = bpmctlr->getBpmById(id);

	if(bpm == 0) {
		syslog(LOG_INFO, "%s -- creating BPM %s\n",aip->name,id.c_str());
		bpm = new Bpm(id);
		bpmctlr->registerBpm(bpm);
		if(bpm==0) {
			syslog(LOG_INFO, "%s -- failed to create/register BPM!!\n",aip->name);
			return -1;
		}
	}

	string type(aip->inp.value.instio.string);
	try {
		aiData *aid = new aiData(bpm,getRecType(type));
		//FIXME -- what happens if aip->eslo is changed via caput ??
		// Nothing: that's what!! Could refactor UI-controlled Bpm class-attributes
		// into *pointers*. That way we could hook record fields into each object instance...
		if(aid->type==xval) { bpm->setXVoltsPerMilli(aip->eslo); }
		else { bpm->setYVoltsPerMilli(aip->eslo); }
		//hook our aiData object into this record instance
		aip->dpvt = (void*)aid;
	}
	catch(runtime_error& err) {
		syslog(LOG_INFO, "%s",err.what());
		return -1;
	}
	//register ONCE ONLY for BPM value-change events:
	if(once) {
		once=0;
		bpmctlr->setBpmValueChangeCallback(onBpmValueChange,(void*)aip->evnt);
	}
	return 0;
}

// if we return 0 here, aiRecord.c will call convert() which will set val=0 if LINR=NO CONVERSION
static long read_ai(void* air) {
	aiRecord *aip = (aiRecord*)air;
	aiData *aid = (aiData*)aip->dpvt;

	if(aid->type == xval) {
		aip->val = aid->bpm->getX();
		aid->bpm->setXVoltsPerMilli(aip->eslo);
	}
	else {
		aip->val = aid->bpm->getY();
		aid->bpm->setYVoltsPerMilli(aip->eslo);
	}
	aip->udf=0;
	return 1;
}

#ifdef __cplusplus
}
#endif

