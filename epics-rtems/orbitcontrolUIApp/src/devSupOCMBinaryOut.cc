/*
 * devSupOCMBinaryOut.cc
 *
 *  Created on: Jan 13, 2009
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
} devSupOCMBinaryOut={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    write_bo,
};

epicsExportAddress(dset,devSupOCMBinaryOut);

static long init_record(void* bor) {
	boRecord *pbo = (boRecord*)bor;
	OcmController *ocmCtlr = OrbitController::getInstance();

	if (pbo->out.type != INST_IO) {
		syslog(LOG_INFO, "%s: OUT field type should be INST_IO\n", pbo->name);
		return(S_db_badField);
	}
	string name(pbo->name);
	size_t pos = name.find_first_of(":");
	string id = name.substr(0,pos);
	Ocm *ocm = ocmCtlr->getOcmById(id);
	if(ocm == NULL) {
		char cbuf[128] = {0};
		strncpy(cbuf,pbo->out.value.instio.string,sizeof(cbuf)/sizeof(cbuf[0]));
		/* parse out the params */
		uint32_t crateId = strtoul(strtok(cbuf," "),NULL,10);
		uint32_t vmeBaseAddr = strtoul(strtok(NULL," "),NULL,16);
		uint8_t channel = (epicsUInt8)strtoul(strtok(NULL," "),NULL,10);
		ocm = ocmCtlr->registerOcm(id,crateId,vmeBaseAddr,channel);
		if(ocm==NULL) {
			syslog(LOG_INFO, "%s: failure creating OCM %s!!!\n",pbo->name,id.c_str());
			return -1;
		}
	}
	/* From Record Reference Manual:
	 * ---------------------------------
	 * If DOL is constant than VAL is initialized to 1 or 0, dependent upon (non)zero DOL value.
	 */
	if(pbo->val != 0) { ocm->setEnabled(true); }
	else { ocm->setEnabled(false); }
	pbo->dpvt = (void*)ocm;
	return 0;
}


static long write_bo(void* bor) {
	boRecord *pbo = (boRecord*)bor;
	Ocm *ocm = (Ocm*)pbo->dpvt;
	if(pbo->rval != 0) { ocm->setEnabled(true); }
	else { ocm->setEnabled(false); }
	return 0;
}


#ifdef __cplusplus
}
#endif
