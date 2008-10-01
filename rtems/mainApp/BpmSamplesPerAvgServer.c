#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include <sis1100_api.h>
#include <vmeDefs.h>
#include <utils.h>
#include "DaqServerUtils.h"
#include "DioReadServer.h"
#include "DaqController.h"
#include "DataHandler.h"
#include "dataDefs.h"
#include "BpmSamplesPerAvgServer.h"


static int bpmSamplesPerAvgFD;
static rtems_id bpmSamplesPerAvgServerTID;

static rtems_task
BpmSamplesPerAvgServer(rtems_task_argument arg) {
	extern uint32_t SamplesPerAvg;
	rtems_status_code rc = 0;
	FILE *fp = NULL;
	const int bufsize=512;
	char cbuf[bufsize];
	int len = 0;
	uint32_t data = 0;
	uint8_t numBytes = 0;


	syslog(LOG_INFO, "BpmSamplesPerAvgServer: waiting for client...\n");
	/* block-waiting for client to connect */
	bpmSamplesPerAvgFD = ServerConnect("BpmSamplesPerAvgServer", BpmSamplesPerAvgServerPort, 0xFFFF0000);
	if(bpmSamplesPerAvgFD < 0) {
		goto bailout;
	}

	/* wrap this socket in a stream... */
	fp = fdopen(bpmSamplesPerAvgFD, "r+");
	if(fp==NULL) {
		syslog(LOG_INFO, "BpmSamplesPerAvgServer: fdopen failure -- %s\n", strerror(errno));
		goto bailout;
	}

	syslog(LOG_INFO, "BpmSamplesPerAvgServer: entering main loop...\n");
	for(;;) {
		/* get request from client... */
		fflush(fp);
		if(fgets(cbuf,sizeof(cbuf),fp)==NULL) {
			syslog(LOG_INFO, "BpmSamplesPerAvgServer: error reading -- %s\n",strerror(errno));
			fflush(fp);
			goto bailout;
		}
		len = strlen(cbuf);
		if(len<=1) { continue; }
		/* replace newline with null-terminator... */
		cbuf[len-1] = '\0';

		uint32_t tmp = strtoul(cbuf,NULL,0);
		if(1000 <= tmp && tmp <= 10000) { /* i.e. between 0.1 and 1.0 seconds per BPM avg */
			SamplesPerAvg = tmp;
			memset(cbuf,'\0',len);
			syslog(LOG_INFO, "BpmSamplesPerAvgServer: setting SamplesPerAvg=%d\n", tmp);
		}
		else { /* error!! */
			syslog(LOG_INFO, "BpmSamplesPerAvgServer: won't set SamplesPerAvg=%d\n", tmp);
		}

	} /* end for(;;) */

bailout:
	/*resource clean up... */
	syslog(LOG_INFO,"DioReadServer: restarting...\n");
	if(fp != NULL) {
		fclose(fp);
	}
	if(bpmSamplesPerAvgFD > 0) {
		close(bpmSamplesPerAvgFD);
		bpmSamplesPerAvgFD = -1;
	}
	rtems_task_wake_after(10);/*let socket drain...?*/
	rtems_task_restart(bpmSamplesPerAvgServerTID, arg);
}

void StartBpmSamplesPerAvgServer(void *arg) {
	rtems_status_code rc;

	rc = rtems_task_create(BpmSamplesPerAvgServerName,
			DefaultPriority+4/*priority*/,
			RTEMS_MINIMUM_STACK_SIZE*4,
			RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
			RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
			&bpmSamplesPerAvgServerTID);
	TestDirective(rc, "rtems_task_create()");

	rc = rtems_task_start(bpmSamplesPerAvgServerTID, BpmSamplesPerAvgServer, (rtems_task_argument)arg);
	TestDirective(rc, "rtems_task_start()");
}

void DestroyBpmSamplesPerAvgServer(void) {

	if(bpmSamplesPerAvgFD > 0) { close(bpmSamplesPerAvgFD); bpmSamplesPerAvgFD = -1; }
	rtems_task_delete(bpmSamplesPerAvgServerTID);
}
