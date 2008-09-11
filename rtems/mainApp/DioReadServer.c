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

static int dioReadClientFD;
static rtems_id dioReadServerTID;

static rtems_task
DioReadServer(rtems_task_argument arg) {
	VmeCrate **crateArray = (VmeCrate **)arg;
	rtems_status_code rc = 0;
	FILE *fp = NULL;
	char cbuf[512] = {0};
	int len = 0;
	DioRequestMsg reqMsg = {0};
	uint32_t data = 0;
	uint8_t numBytes = 0;

	
	syslog(LOG_INFO, "DioReadServer: waiting for client...\n");
	/* block-waiting for client to connect */
	dioReadClientFD = ServerConnect("DioReadServer", DioReadServerPort, 0xFFFF0000);
	if(dioReadClientFD < 0) {
		goto bailout;
	}

	/* wrap this socket in a stream... */
	fp = fdopen(dioReadClientFD, "r+");
	if(fp==NULL) {
		syslog(LOG_INFO, "DioReadServer: fdopen failure -- %s\n", strerror(errno));
		goto bailout;
	}
	
	syslog(LOG_INFO, "DioReadServer: entering main loop...\n");
	for(;;) {
		/* get DIO read-request from client... */
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
		
		/* parse string into DioRequestMsg... */
		reqMsg.crateID = strtoul(strtok(cbuf," "),NULL,10);
		reqMsg.vmeBaseAddr = strtoul(strtok(NULL," "),NULL,16);
		reqMsg.modOffset = strtoul(strtok(NULL," "),NULL,10);
		reqMsg.mask = strtoul(strtok(NULL," "),NULL,16);
		
		syslog(LOG_INFO, "DioReadServer request: crateID=%d vmeBaseAddr=%#x modOffset=%d mask=%3x\n",
							reqMsg.crateID,reqMsg.vmeBaseAddr,reqMsg.modOffset,reqMsg.mask);
		
		numBytes = (reqMsg.mask > (1<<16)) ? 4 : 2;
		
		if(numBytes==4) {
			/* it's a 32-bit read... */
			rc = vme_A24D32_read(crateArray[reqMsg.crateID]->fd,reqMsg.vmeBaseAddr+reqMsg.modOffset,&data);
			if(rc) {
		    	syslog(LOG_INFO, "DioReadServer: failed VME read--%#x\n",rc);
		    }
		}
		else {
			/* it's a 16-bit read... */
			rc = vme_A24D16_read(crateArray[reqMsg.crateID]->fd,reqMsg.vmeBaseAddr+reqMsg.modOffset,(uint16_t *)&data);
			if(rc) {
		    	syslog(LOG_INFO, "DioReadServer: failed VME read--%#x\n",rc);
		    }
		}
		
		/*len = fprintf(fp,"data=%#x",(reqMsg.mask>(1<<16))?data:(uint16_t)data);
		if(len<0) {
			syslog(LOG_INFO, "DioReadServer: fprintf failure--%s, len=%d\n",strerror(errno),len);
			goto bailout;
		}*/
		len = fwrite(&data,numBytes,1,fp);
		if(len != 1) {
			syslog(LOG_INFO, "DioReadServer: fwrite failure--%s, len=%d\n",strerror(errno),len);
			goto bailout;
		}
	} /* end for(;;) */
	
bailout:	
	/*resource clean up... */
	syslog(LOG_INFO,"DioReadServer: restarting...\n");
	if(fp != NULL) {
		fclose(fp);
	}
	if(dioReadClientFD > 0) {
		close(dioReadClientFD);
		dioReadClientFD = -1;
	}
	rtems_task_wake_after(10);/*let socket drain...?*/
	rtems_task_restart(dioReadServerTID, arg);
}

void StartDioReadServer(void *arg) {
	rtems_status_code rc;
	
	rc = rtems_task_create(DioReadServerName,
			DefaultPriority+4/*priority*/,
			RTEMS_MINIMUM_STACK_SIZE*4,
			RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
			RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
			&dioReadServerTID);
	TestDirective(rc, "rtems_task_create()");
	
	rc = rtems_task_start(dioReadServerTID, DioReadServer, (rtems_task_argument)arg);
	TestDirective(rc, "rtems_task_start()");
}

void DestroyDioReadServer(void) {
	
	if(dioReadClientFD > 0) { close(dioReadClientFD); dioReadClientFD = -1; }
	rtems_task_delete(dioReadServerTID);
}
