#include <waveformRecord.h>
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
#include <OrbitController.h>
#include <syslog.h>
#include <string>
#include <stdexcept>
using std::runtime_error;
using std::string;


#ifdef __cplusplus
	extern "C" {
#endif

static long init_record(void* wfr);
static long read_wf(void* wfr);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   read_wf;
} devSupOCMWaveform={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    read_wf,
};
epicsExportAddress(dset,devSupOCMWaveform);

enum wfType {xResp,yResp,xDisp};

static wfType getRecType(string type) {
	if(type.compare("x:responseVector")==0) { return xResp; }
	else if(type.compare("y:responseVector")==0) { return yResp; }
	else if(type.compare("x:dispersionVector")==0) { return xDisp; }
	else {
		type.append(": unknown record type!!! WTF ?!?!?!?");
		throw runtime_error(type.c_str());
	}
}

static long init_record(void* wfr) {
	waveformRecord* wfrp = (waveformRecord*)wfr;

	syslog(LOG_INFO,"Init OCM Waveform: %s\n",wfrp->name);
	/* chk INP type: has to be "caput()-able" */
	if (wfrp->inp.type != CONSTANT) {
		syslog(LOG_INFO,"%s: INP field type must be CONSTANT\n", wfrp->name);
		return (S_db_badField);
	}
	/* XXX -- Record Ref Manual says mem is allocated
	 * for buffer before we get here (pg 301)
	 */
	if(wfrp->bptr == NULL) {
		/*FIXME -- this condition is recoverable.*/
		syslog(LOG_INFO,"BPTR is NULL !!\n");
		return -1;
	}

	try {
		string name(wfrp->name);
		size_t pos = name.find_first_of(":");
		wfrp->dpvt = (void*)getRecType(name.substr(pos,name.npos));
	}
	catch(runtime_error& err) {
		syslog(LOG_INFO, "%s",err.what());
		return -1;
	}

	return 0;
}

/*
 * we're using read_wf() to write new values
 */
static long read_wf(void* wfr) {
	waveformRecord* wfrp = (waveformRecord*)wfr;
	OcmController *ocmCtlr = OrbitController::getInstance();
	uint32_t type = (uint32_t)wfrp->dpvt;

	switch(type) {
	case xResp:
		ocmCtlr->setHorizontalResponseMatrix((double*)wfrp->bptr);
		break;
	case yResp:
		ocmCtlr->setVerticalResponseMatrix((double*)wfrp->bptr);
		break;
	case xDisp:
		ocmCtlr->setDispersionVector((double*)wfrp->bptr);
		break;
	default:
		syslog(LOG_INFO,"devSupOCMWaveform: read_wf() default case, AAARRRGGG!!!\n");
		return -1;
	}

	wfrp->nord = wfrp->nelm;
    return 0;
}

#ifdef __cplusplus
}
#endif
