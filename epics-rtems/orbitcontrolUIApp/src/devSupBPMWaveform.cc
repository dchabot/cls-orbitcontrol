#include <waveformRecord.h>
#include <dbCommon.h>
#include <stdlib.h>
#include <stdio.h>
#include <devSup.h>

#include <dbDefs.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <epicsThread.h>
#include <syslog.h>
#include <DataHandler.h>

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
} devSupBPMWaveform={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    read_wf,
};
epicsExportAddress(dset,devSupBPMWaveform);


static void bpmAveragesQueueListener(void* arg);

static rtems_id tid;

static long init_record(void* wfr) {
	waveformRecord* wfrp = (waveformRecord*)wfr;

	syslog(LOG_INFO,"Init BPM Waveform\n");
	/* chk INP type: */
	if (wfrp->inp.type != INST_IO) {
		syslog(LOG_INFO,"%s: INP field type must be INST_IO\n", wfrp->name);
		return (S_db_badField);
	}
	syslog(LOG_INFO,"wfrp->inp.value.instio.string=%s\n",wfrp->inp.value.instio.string);
	syslog(LOG_INFO,"event#=%hd\n", wfrp->evnt);
	/* XXX -- Record Ref Manual says mem is allocated
	 * for buffer before we get here (pg 301)
	 */
	if(wfrp->bptr == NULL) {
		/*allocate mem for record buffer using NELM and FTVL.*/
		syslog(LOG_INFO,"BPTR is NULL !!\n");
		return -1;
	}
	/* hookup to the DataHandler Singleton */
	DataHandler* dh = DataHandler::getInstance();
	wfrp->dpvt = (void*)dh;
	/*fire up msgQ-listener thread:*/
	tid = (rtems_id)epicsThreadCreate("BPMAvgs",
							epicsThreadPriorityHigh,
							epicsThreadGetStackSize(epicsThreadStackBig),
							bpmAveragesQueueListener,
							(void*)wfrp);
	if(tid == 0) {
		syslog(LOG_INFO, "Problem creating EPICS thread (averaging)!!\n");
		return -1;
	}
	return 0;
}

/* nothing to do here: is this really needed ??? */
static long read_wf(void* wfr) {
    return 0;
}

static void bpmAveragesQueueListener(void* arg) {
	waveformRecord *wfr = (waveformRecord*)arg;
	DataHandler* dh = (DataHandler*)wfr->dpvt;

	syslog(LOG_INFO,"bpmAveragesQueueListener is online!\n");
	/* register with DataHandler for BPM avgs deliveries */
	dh->clientRegister(tid);
	for(;;) {
		/* getBPMAverages will block this thread */
		BPMData* avgs =  dh->getBPMAverages();
		if(avgs != 0) {
			/* good to go: copy data to record's buffer */
			memcpy(wfr->bptr, avgs->buf, avgs->numElements*sizeof(double));
			wfr->nord = avgs->numElements;
		}
		else {
			syslog(LOG_INFO, "bpmAveragesQueueListener: null BPMAverages object!!\n");
			break;
		}
		/* fire record processing */
		post_event(wfr->evnt);
		/* release mem */
		delete avgs;
	}
	dh->clientUnregister(tid);
	syslog(LOG_INFO,"bpmAveragesQueueListener is terminating!\n");
}

#ifdef __cplusplus
}
#endif
