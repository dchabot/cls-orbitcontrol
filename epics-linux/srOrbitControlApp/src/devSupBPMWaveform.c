/* FIXME -- change stdio outputs to use syslog() for use with RTEMS system */

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
/*#include "../../../rtems/mainApp/dataDefs.h"*/

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


typedef struct {
	waveformRecord* wfr;
	short event;
} BpmQueueListenerArg;

static void bpmAveragesQueueListener(void* arg);

static long init_record(struct waveformRecord* wfr) {
	epicsThreadId tid;


	printf("Init Waveform\n");
	/* chk INP type: */
	if (wfr->inp.type != INST_IO) {
		printf("%s: INP field type must be INST_IO\n", wfr->name);
		return (S_db_badField);
	}
	printf("wfr->inp.value.instio.string=%s\n",wfr->inp.value.instio.string);
	printf("event#=%hd\n", wfr->evnt);
	/*XXX -- mem-alloc may not be req'd: chk if BPTR==NULL */
	if(wfr->bptr == NULL) {
		/*allocate mem for record buffer using NELM and FTVL.*/
		printf("BPTR is NULL !!\n");
		return 0;
	}
	/*fire up msgQ-listener thread:*/
	tid = epicsThreadCreate("BPMAvgs",
							epicsThreadPriorityHigh,
							epicsThreadGetStackSize(epicsThreadStackBig),
							bpmAveragesQueueListener,
							(void*)wfr);
	if(tid == NULL) {

	}
	return 0;
}

static long read_wf(struct waveformRecord* wfr) {
	/*copy data to local buffer and free original buffer:*/
	printf("In read_wf()\n");
	/*set NORD*/
    return 0;
}

static void bpmAveragesQueueListener(void* arg) {
	waveformRecord *wfr = (waveformRecord*)arg;
	epicsMessageQueueId qid = NULL;

	printf("bpmAveragesQueueListener is online!\n");
	/*epicsThreadSleep(5.0);*/

	/* initialize and block on msgQ */
	qid = epicsMessageQueueCreate(10, sizeof(void*));
	if(qid == NULL) {
		printf("Couldn't create Averages MsgQ!!\n");
		exit(1);
	}
	for(;;) {
		epicsThreadSleep(1.0);
		/* fire record processing */
		printf("bpmAveragesQueueListener posting event#%hd\n",wfr->evnt);
		post_event(wfr->evnt);
	}
	printf("bpmAveragesQueueListener is terminating!\n");
}
