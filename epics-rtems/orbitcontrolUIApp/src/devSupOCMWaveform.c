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
#include <epicsThread.h>
#include <epicsMessageQueue.h>
#include "devSupOCMWaveform.h"
#include <syslog.h>

static long init_record(struct waveformRecord* wfr);
static long read_wf(struct waveformRecord* wfr);

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


static long init_record(struct waveformRecord* wfr) {
	static epicsMessageQueueId qid = NULL;
	extern rtems_id OcmSetpointQID;

	syslog(LOG_INFO,"Init OCM Waveform\n");
	/* chk INP type: has to be "caput()-able" */
	if (wfr->inp.type != CONSTANT) {
		syslog(LOG_INFO,"%s: INP field type must be CONSTANT\n", wfr->name);
		return (S_db_badField);
	}
	syslog(LOG_INFO,"wfr->inp.value.constantStr=%s\n",wfr->inp.value.constantStr);
	/* XXX -- Record Ref Manual says mem is allocated
	 * for buffer before we get here (pg 301)
	 */
	if(wfr->bptr == NULL) {
		/*allocate mem for record buffer using NELM and FTVL.*/
		syslog(LOG_INFO,"BPTR is NULL !!\n");
		return -1;
	}
	/* initialize and block on msgQ */
	qid = epicsMessageQueueCreate(OcmSetpointQueueCapacity,
									sizeof(spMsg));
	if(qid == NULL) {
		syslog(LOG_INFO,"Couldn't create OCM-setpoint MsgQ!!\n");
		return -1;
	}
	OcmSetpointQID = qid->id;
	/* we'll need the QID in read_wf() */
	wfr->dpvt = qid;

	return 0;
}

/*
 * copy BPTR contents to a malloc'd buf and ship off a message to
 * the OrbitController
 */
static long read_wf(struct waveformRecord* wfr) {
	int rc;
	spMsg msg;
	int32_t *buf = NULL;
	ssize_t numBytes = wfr->nelm*dbValueSize(wfr->ftvl);

	buf = calloc(1, numBytes);
	if(buf==NULL) {
		syslog(LOG_INFO, "Couldn't alloc-mem for OCM setpoint buffer!!\n");
		return -1;
	}
	memcpy(buf, wfr->bptr, numBytes);
	msg.buf = buf;
	msg.numsp = wfr->nelm;

	rc = epicsMessageQueueTrySend((epicsMessageQueueId)(wfr->dpvt),&msg,sizeof(spMsg));
	if(rc < 0) {
		syslog(LOG_INFO,"Failure of epicsMessageQueueTrySend()!! rc=%d\n",rc);
		if(buf) { free(buf); }
		return -1;
	}
	wfr->nord = wfr->nelm;
    return 0;
}
