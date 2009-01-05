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
#include <syslog.h>

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


static long init_record(void* wfr) {
	waveformRecord* wfrp = (waveformRecord*)wfr;
	static epicsMessageQueueId qid = NULL;
	extern rtems_id OcmSetpointQID;

	syslog(LOG_INFO,"Init OCM Waveform\n");
	/* chk INP type: has to be "caput()-able" */
	if (wfrp->inp.type != CONSTANT) {
		syslog(LOG_INFO,"%s: INP field type must be CONSTANT\n", wfrp->name);
		return (S_db_badField);
	}
	syslog(LOG_INFO,"wfrp->inp.value.constantStr=%s\n",wfrp->inp.value.constantStr);
	/* XXX -- Record Ref Manual says mem is allocated
	 * for buffer before we get here (pg 301)
	 */
	if(wfrp->bptr == NULL) {
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
	wfrp->dpvt = qid;

	return 0;
}

/*
 * copy BPTR contents to a malloc'd buf and ship off a message to
 * the OrbitController
 */
static long read_wf(void* wfr) {
	waveformRecord* wfrp = (waveformRecord*)wfr;
	int rc;
	spMsg msg;
	int32_t *buf = NULL;
	ssize_t numBytes = wfrp->nelm*dbValueSize(wfrp->ftvl);

	buf = calloc(1, numBytes);
	if(buf==NULL) {
		syslog(LOG_INFO, "Couldn't alloc-mem for OCM setpoint buffer!!\n");
		return -1;
	}
	memcpy(buf, wfrp->bptr, numBytes);
	msg.buf = buf;
	msg.numsp = wfrp->nelm;

	rc = epicsMessageQueueTrySend((epicsMessageQueueId)(wfrp->dpvt),&msg,sizeof(spMsg));
	if(rc < 0) {
		syslog(LOG_INFO,"Failure of epicsMessageQueueTrySend()!! rc=%d\n",rc);
		if(buf) { free(buf); }
		return -1;
	}
	wfrp->nord = wfrp->nelm;
    return 0;
}

#ifdef __cplusplus
}
#endif
