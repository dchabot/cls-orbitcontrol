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
#include <epicsMessageQueue.h>
#include "devSupBPMWaveform.h"
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


static long init_record(struct waveformRecord* wfr) {
	epicsThreadId tid;


	syslog(LOG_INFO,"Init BPM Waveform\n");
	/* chk INP type: */
	if (wfr->inp.type != INST_IO) {
		syslog(LOG_INFO,"%s: INP field type must be INST_IO\n", wfr->name);
		return (S_db_badField);
	}
	syslog(LOG_INFO,"wfr->inp.value.instio.string=%s\n",wfr->inp.value.instio.string);
	syslog(LOG_INFO,"event#=%hd\n", wfr->evnt);
	/* XXX -- Record Ref Manual says mem is allocated
	 * for buffer before we get here (pg 301)
	 */
	if(wfr->bptr == NULL) {
		/*allocate mem for record buffer using NELM and FTVL.*/
		syslog(LOG_INFO,"BPTR is NULL !!\n");
		return -1;
	}
	/*fire up msgQ-listener thread:*/
	tid = epicsThreadCreate("BPMAvgs",
							epicsThreadPriorityHigh,
							epicsThreadGetStackSize(epicsThreadStackBig),
							bpmAveragesQueueListener,
							(void*)wfr);
	if(tid == NULL) {
		syslog(LOG_INFO, "Problem creating EPICS thread (averaging)!!\n");
		return -1;
	}
	return 0;
}

/* nothing to do here: is this really needed ??? */
static long read_wf(struct waveformRecord* wfr) {
    return 0;
}

static void bpmAveragesQueueListener(void* arg) {
	waveformRecord *wfr = (waveformRecord*)arg;
	epicsMessageQueueId qid = NULL;
	ProcessedDataSegment pds = {0};
	extern rtems_id ProcessedDataQueueId;
	int rc;

	syslog(LOG_INFO,"bpmAveragesQueueListener is online!\n");
	/*epicsThreadSleep(5.0);*/

	/* initialize and block on msgQ */
	qid = epicsMessageQueueCreate(ProcessedDataQueueCapacity,
									sizeof(ProcessedDataSegment));
	if(qid == NULL) {
		syslog(LOG_INFO,"Couldn't create Averages MsgQ!!\n");
		exit(1);
	}
	ProcessedDataQueueId = qid->id;
	for(;;) {
		rc = epicsMessageQueueReceive(qid, &pds, sizeof(ProcessedDataSegment));
		if(rc == sizeof(ProcessedDataSegment)) {
			/* good to go: copy data to record's buffer */
			memcpy(wfr->bptr, pds.buf, pds.numElements*sizeof(double));
			wfr->nord = pds.numElements;
		}
		else {
			syslog(LOG_INFO, "bpmAveragesQueueListener: incorrect msg size=%d\n",rc);
		}
		/* fire record processing */
		post_event(wfr->evnt);
		/* release ProcessedDataSegment mem */
		free(pds.buf);
	}
	syslog(LOG_INFO,"bpmAveragesQueueListener is terminating!\n");
}
