#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include <utils.h>
#include "DaqServerUtils.h"
#include "AdcDataServer.h"
#include "DaqController.h"
#include "DataHandler.h"
#include "dataDefs.h"

static int adcClientFD;
static rtems_id adcServerTID;
static rtems_id adcServerQID;

static rtems_task
AdcDataServer(rtems_task_argument arg) {
	rtems_status_code rc = 0;
	ProcessedDataSegment pds = {0};

	syslog(LOG_INFO, "AdcDataServer: waiting for client...\n");
	/* block-waiting for client to connect */
	adcClientFD = ServerConnect("AdcDataServer", (uint16_t)arg, 0xFFFF0000);
	if(adcClientFD < 0) {
		goto bailout;
	}

	/* processed-data queue: fed by DataHandler, read by control system interface */
	rc = rtems_message_queue_create(ProcessedDataQueueName,
									10/*max queue size*/,
									sizeof(ProcessedDataSegment)/*msg size*/,
									RTEMS_LOCAL|RTEMS_FIFO,
									&adcServerQID);
	TestDirective(rc, "rtems_message_queue_create()");

	syslog(LOG_INFO, "AdcDataServer: entering main loop...\n");
	/* push data to client */
	for(;;) {
		ssize_t bytesTx = 0;
		size_t txSize = 0;
		uint32_t msgSize = 0;
		
		rc = rtems_message_queue_receive(adcServerQID, &pds, &msgSize, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
		if(rc != RTEMS_SUCCESSFUL) {
			/* someone likely destroyed the ProcessedDataQueue; restart this thread */
			syslog(LOG_INFO, "AdcDataServer: rtems_message_queue_receive()==%s",rtems_status_text(rc));
			goto bailout; 
		}
		TestDirective(rc, "AdcDataServer: rtems_message_queue_receive()");
		if(msgSize != sizeof(pds)) {
			syslog(LOG_INFO, "AdcDataServer: was expecting %u bytes but got %u\n", sizeof(pds), msgSize);
			goto bailout; /* FIXME */
		}
		
		txSize = pds.numElements*sizeof(double);
		bytesTx = write(adcClientFD, pds.buf, txSize);
		if(bytesTx < 0) {
			syslog(LOG_INFO, "AdcDataServer: write() error -- %s\n", strerror(errno));
			goto bailout;
		}
		if((bytesTx > 0) && (bytesTx != txSize)) {
			syslog(LOG_INFO, "AdcDataServer: transmitted only %u bytes, not %u\n", bytesTx, txSize);
			/* try again */
			txSize -= bytesTx;
			bytesTx = write(adcClientFD, (void *)(pds.buf+bytesTx), txSize);
			if(bytesTx != txSize) {
				syslog(LOG_INFO, "AdcDataServer: giving up on net transmission...\n");
				goto bailout;
			}
		}
		/* done: free the buffer at pds.buf */
		free(pds.buf);
	} /* end for(;;) */
	
bailout:	
	/*resource clean up... */
	syslog(LOG_INFO,"AdcDataServer: restarting...\n");
	if(adcClientFD > 0) {
		close(adcClientFD);
		adcClientFD = -1;
	}
	rtems_message_queue_delete(adcServerQID);
	rtems_task_wake_after(10);/*let socket drain...?*/
	rtems_task_restart(adcServerTID, AdcDataServerPort);
}

void StartAdcDataServer(void) {
	rtems_status_code rc;
	
	rc = rtems_task_create(AdcDataServerName,
			DefaultPriority+4/*priority*/,
			RTEMS_MINIMUM_STACK_SIZE*4,
			RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
			RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
			&adcServerTID);
	TestDirective(rc, "rtems_task_create()");
	
	rc = rtems_task_start(adcServerTID, AdcDataServer, AdcDataServerPort);
	TestDirective(rc, "rtems_task_start()");
}

void DestroyAdcDataServer(void) {
	
	if(adcClientFD > 0) { close(adcClientFD); adcClientFD = -1; }
	rtems_message_queue_delete(adcServerQID);
	rtems_task_delete(adcServerTID);
}
