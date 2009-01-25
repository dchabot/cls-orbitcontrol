/*
 * devSupOCMAnalogOut.cc
 *
 *  Created on: Jan 15, 2009
 *      Author: chabotd
 */

#include <aoRecord.h>
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
    DEVSUPFUN	special_linconv;
} devSupOCMAnalogOut={
    6,
    NULL,
    NULL,
    init_record,
    NULL,
    write_ao,
    NULL
};

epicsExportAddress(dset,devSupOCMAnalogOut);

enum ocmAOType {xFrac,yFrac};

struct OcmFraction {
	OcmFraction(ocmAOType t):type(t){}
	ocmAOType type;
};

static long init_record(void* aor) {
	aoRecord *aop = (aoRecord*)aor;
	/*chk OUT type:*/
	if (aop->out.type != INST_IO) {
		syslog(LOG_INFO, "%s: OUT field type should be INST_IO\n", aop->name);
		return (S_db_badField);
	}
	string type(aop->out.value.instio.string);
	if(type.compare("xFraction")==0) { aop->dpvt = (void*)new OcmFraction(xFrac); }
	else if(type.compare("yFraction")==0) { aop->dpvt = (void*)new OcmFraction(yFrac); }
	else {
		type.append(": unknown OUT type!!! WTF ?!?!?!?");
		throw runtime_error(type.c_str());
	}
	syslog(LOG_INFO, "%s is ocmAOType=%s\n",aop->name,type.c_str());
	return 0;
}
static long write_ao(void* aor) {
	aoRecord *aop = (aoRecord*)aor;
	OcmController *ocmCtlr = OrbitController::getInstance();
	OcmFraction *of = (OcmFraction*)aop->dpvt;

	switch(of->type) {
		case(xFrac):
			ocmCtlr->setMaxHorizontalFraction(aop->val);
			break;
		case(yFrac):
			ocmCtlr->setMaxVerticalFraction(aop->val);
			break;
		default:
			syslog(LOG_INFO, "%s: unknown ocmAOType=%i !!\n",aop->name,of->type);
			return -1;
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
