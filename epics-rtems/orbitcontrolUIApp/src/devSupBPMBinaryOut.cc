/*
 * devSupBPMBinaryOut.cc
 *
 *  Created on: Jan 8, 2009
 *      Author: chabotd
 */

#include <boRecord.h>
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

static long init_record(void* bor);
static long write_bo(void* bor);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   write_bo;
} devSupBPMBinaryOut={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_bo,
};

epicsExportAddress(dset,devSupBPMBinaryOut);

/**
 * This routine will instantiate the object representing this BPM and
 * register it with a BpmController. The BPM object
 * is then also utilized in various other Device Support routines.
 *
 * @param bor ptr to binary out record
 * @return
 */
static long init_record(void* bor) {
	boRecord *pbo = (boRecord*)bor;
	OrbitController *oc = OrbitController::getInstance();

	if (pbo->out.type != INST_IO) {
		syslog(LOG_INFO, "%s: OUT field type should be INST_IO\n", pbo->name);
		return(S_db_badField);
	}

	string name(pbo->name);
	size_t pos = name.find(":isInCorrection");
	Bpm *bpm = new Bpm(name.substr(0,pos));
	/* From Record Reference Manual:
	 * ---------------------------------
	 * If DOL is constant than VAL is initialized to 1 or 0, dependent upon (non)zero DOL value.
	 */
	bpm->setEnabled((bool)pbo->val);
	syslog(LOG_INFO, "%s: isEnabled=%s\n",bpm->getId().c_str(),(bpm->isEnabled()?"true":"false"));
	BpmController *bpmctlr = oc->getBpmController();
	bpmctlr->registerBpm(bpm);

	return 0;
}

/**
 * Simply update this BPM object's "enabled" property.
 *
 * @param bor
 * @return
 */
static long write_bo(void* bor) {
	boRecord *pbo = (boRecord*)bor;
	Bpm *bpm = (Bpm*)pbo->dpvt;

	bpm->setEnabled((bool)pbo->val);
	return 0;
}

#ifdef __cplusplus
}
#endif

