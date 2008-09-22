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
#include <OcmSetpointServer.h>
#include "DaqController.h"
#include "DataHandler.h"
#include "dataDefs.h"
#include <ocmDefs.h>

static int ocmSetpointClientFD;
static rtems_id ocmSetpointServerTID;
static rtems_id ocmSetpointQID;

static rtems_task
OcmSetpointServer(rtems_task_argument arg) {
	PSController **psp = (PSController **)arg;
	rtems_status_code rc = 0;
	ssize_t n = 0;
	static int32_t data[NumOCM], *ip;

	rc = rtems_message_queue_create(OcmSetpointQueueName,
			1,sizeof(int32_t*), RTEMS_LOCAL|RTEMS_FIFO, &ocmSetpointQID);
	TestDirective(rc, "rtems_message_queue_create()");

	syslog(LOG_INFO, "OcmSetpointServer: waiting for client...\n");
	/* block-waiting for client to connect */
	ocmSetpointClientFD = ServerConnect("OcmSetpointServer", OcmSetpointServerPort, 0xFFFF0000);
	if(ocmSetpointClientFD < 0) {
		goto bailout;
	}

	syslog(LOG_INFO, "OcmSetpointServer: entering main loop...\n");
	for(;;) {
		/* block waiting for update of PS setpoint-array from client... */
		if((n=read(ocmSetpointClientFD, data, sizeof(int32_t)*NumOCM)) <= 0) {
			syslog(LOG_INFO, "OcmSetpointServer: error reading -- %s\n",strerror(errno));
			goto bailout;
		}
		if(n != NumOCM) {
			syslog(LOG_INFO, "OcmSetpointServer: error only read %d bytes!!\n",n);
			goto bailout;
		}

		/* NOTE: DaqController will free() this calloc'd memory */
		ip = (int32_t*)calloc(1,sizeof(int32_t)*NumOCM);
		if(ip != NULL) {
			memcpy(ip, data, sizeof(int32_t)*NumOCM);
		}
		else {
			syslog(LOG_INFO, "OcmSetpointServer: error calloc'ing -- %s\n",strerror(errno));
			goto bailout;
		}

		/* parse string into DioRequestMsg... */
		/*reqMsg.crateID = strtoul(strtok(cbuf," "),NULL,10);
		reqMsg.vmeBaseAddr = strtoul(strtok(NULL," "),NULL,16);
		reqMsg.modOffset = strtoul(strtok(NULL," "),NULL,10);
		reqMsg.mask = strtoul(strtok(NULL," "),NULL,16);*/
/*
		syslog(LOG_INFO, "OcmSetpointServer request: crateID=%d vmeBaseAddr=%#x modOffset=%d mask=%3x\n",
							reqMsg.crateID,reqMsg.vmeBaseAddr,reqMsg.modOffset,reqMsg.mask);
*/

		/* Let the DaqController know new OCM setpoints need to be applied. */
		rc = rtems_message_queue_send(ocmSetpointQID, ip, sizeof(int32_t*));
		TestDirective(rc, "rtems_message_queue_send");
	} /* end for(;;) */

bailout:
	/*resource clean up... */
	syslog(LOG_INFO,"OcmSetpointServer: restarting...\n");
	if(ocmSetpointClientFD > 0) {
		close(ocmSetpointClientFD);
		ocmSetpointClientFD = -1;
	}
	rtems_message_queue_delete(ocmSetpointQID);
	rtems_task_wake_after(10);/*let socket drain...?*/
	rtems_task_restart(ocmSetpointServerTID, arg);
}

void StartOcmSetpointServer(void *arg) {
	rtems_status_code rc;

	rc = rtems_task_create(OcmSetpointServerName,
			DefaultPriority+4/*priority*/,
			RTEMS_MINIMUM_STACK_SIZE*4,
			RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
			RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
			&ocmSetpointServerTID);
	TestDirective(rc, "rtems_task_create()");

	rc = rtems_task_start(ocmSetpointServerTID, OcmSetpointServer, (rtems_task_argument)arg);
	TestDirective(rc, "rtems_task_start()");
}

void DestroyOcmSetpointServer(void) {

	if(ocmSetpointClientFD > 0) { close(ocmSetpointClientFD); ocmSetpointClientFD = -1; }
	rtems_message_queue_delete(ocmSetpointQID);
	rtems_task_delete(ocmSetpointServerTID);
}
