#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include <vmeDefs.h>
#include <utils.h>
#include "DaqServerUtils.h"
#include "DioWriteServer.h"
#include "DaqController.h"
#include "DataHandler.h"
#include "dataDefs.h"

void SetPwrSupply(int fd, uint32_t vmeAddr, uint32_t channel, uint32_t ampSetPoint);

static int dioWriteClientFD;
static rtems_id dioWriteServerTID;

static rtems_task
DioWriteServer(rtems_task_argument arg) {
	VmeCrate **crateArray = (VmeCrate **)arg;
	FILE *fp = NULL;
	char cbuf[512] = {0};
	int len = 0;
	DioWriteMsg dwMsg = {0};

	syslog(LOG_INFO, "DioWriteServer: waiting for client...\n");
	/* block-waiting for client to connect */
	dioWriteClientFD = ServerConnect("DioWriteServer", DioWriteServerPort, 0xFFFF0000);
	if(dioWriteClientFD < 0) {
		goto bailout;
	}

	/* wrap this socket in a stream... */
	fp = fdopen(dioWriteClientFD, "r+");
	if(fp==NULL) {
		syslog(LOG_INFO, "DioWriteServer: fdopen failure -- %s\n", strerror(errno));
		goto bailout;
	}

	syslog(LOG_INFO, "DioWriteServer: entering main loop...\n");
	for(;;) {
		/* get PS-controller info from client, parse into DioWriteMsg, send to DaqController */
		fflush(fp);
		if(fgets(cbuf,sizeof(cbuf),fp)==NULL) {
			syslog(LOG_INFO, "DioWriteServer: error reading -- %s\n",strerror(errno));
			fflush(fp);
			goto bailout;
		}
		len = strlen(cbuf);
		if(len<=1) { continue; }
		/* replace newline with null-terminator... */
		cbuf[len-1] = '\0';

		/* parse string into DioWriteMsg fields... */
		dwMsg.crateID = strtoul(strtok(cbuf," "),NULL,10);
		dwMsg.vmeBaseAddr = strtoul(strtok(NULL," "),NULL,16);
		dwMsg.modOffset = strtoul(strtok(NULL," "),NULL,10);
		dwMsg.pwrSupChan = strtoul(strtok(NULL," "),NULL,10);
		dwMsg.pwrSupData = strtol(strtok(NULL," "),NULL,10);

		/*syslog(LOG_INFO, "DioWriteServer: crateID=%d vmeBaseAddr=%#x modOffset=%d pwrSupChan=%d pwrSupData=%d",
							dwMsg.crateID,dwMsg.vmeBaseAddr,dwMsg.modOffset,dwMsg.pwrSupChan,dwMsg.pwrSupData);
		*/
		SetPwrSupply(crateArray[dwMsg.crateID]->fd,(dwMsg.vmeBaseAddr+dwMsg.modOffset),dwMsg.pwrSupChan,dwMsg.pwrSupData);
	} /* end for(;;) */

bailout:
	/*resource clean up... */
	syslog(LOG_INFO,"DioWriteServer: restarting...\n");
	if(fp != NULL) {
		fclose(fp);
	}
	if(dioWriteClientFD > 0) {
		close(dioWriteClientFD);
		dioWriteClientFD = -1;
	}
	rtems_task_wake_after(10);/*let socket drain...?*/
	rtems_task_restart(dioWriteServerTID, arg);
}

void StartDioWriteServer(void *arg) {
	rtems_status_code rc;

	rc = rtems_task_create(DioWriteServerName,
			DefaultPriority+4/*priority*/,
			RTEMS_MINIMUM_STACK_SIZE*4,
			RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
			RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
			&dioWriteServerTID);
	TestDirective(rc, "rtems_task_create()");

	rc = rtems_task_start(dioWriteServerTID, DioWriteServer, (rtems_task_argument)arg);
	TestDirective(rc, "rtems_task_start()");
}

void DestroyDioWriteServer(void) {

	if(dioWriteClientFD > 0) {
		close(dioWriteClientFD);
		dioWriteClientFD = -1;
	}
	rtems_task_delete(dioWriteServerTID);
}
