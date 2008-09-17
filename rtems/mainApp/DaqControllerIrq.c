#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include <sis1100_api.h>
#include <ics110bl.h>
#include <vmic2536.h>
#include <utils.h> /*TestDirective()*/

//#include <rtems-gdb-stub.h>
#include "DaqController.h"
#include "dataDefs.h"
#include "AdcReaderThread.h"
#include "DataHandler.h"
#include "AdcDataServer.h"
#include "DioWriteServer.h"
#include "DioReadServer.h"


/* number of DIO modules will vary:
 * 	experimental config uses 4,
 * 	production config uses 5
 */
typedef struct {
	uint32_t baseAddr;
	uint32_t vmeCrateID;
} DioConfig;

DioConfig dioConfig[] = {
		{VMIC_2536_DEFAULT_BASE_ADDR,0},
		{VMIC_2536_DEFAULT_BASE_ADDR,1},
		{VMIC_2536_DEFAULT_BASE_ADDR,2},
		{VMIC_2536_DEFAULT_BASE_ADDR,3}
#if NumDioModules==5
		,{VMIC_2536_DEFAULT_BASE_ADDR+0x10,3}
#endif
};



typedef struct {
	VmeModule *adc;
	rtems_id controllerTID;
	rtems_event_set irqEvent;
} AdcIsrArg;

static void NotifyDaqController(AdcIsrArg *argp) {
	rtems_status_code rc;

	rc = rtems_event_send(argp->controllerTID, argp->irqEvent);
	TestDirective(rc, "NotifyDaqController()-->rtems_event_send()");
}

static void AdcIsr(void *arg, uint8_t vector) {
	AdcIsrArg *parg = (AdcIsrArg *)arg;

	/* disable irq on the board */
	ICS110BInterruptControl(parg->adc, ICS110B_IRQ_DISABLE);
	/* inform the DaqController of this event */
	NotifyDaqController(parg);
}

static int RendezvousPoint(rtems_event_set syncEvents) {
	rtems_event_set eventsIn=0;
	rtems_status_code rc;

	rc = rtems_event_receive(syncEvents, RTEMS_EVENT_ALL|RTEMS_WAIT, RTEMS_NO_TIMEOUT, &eventsIn);
	if(syncEvents != eventsIn) {
		syslog(LOG_INFO, "RendezvousPoint() error: syncEvents=0x%08x\teventsIn=0x%08x\n",syncEvents, eventsIn);
	}
	return TestDirective(rc, "RendezvousPoint()-->rtems_event_receive()");
}

/*static FILE* getOutputFile(const char* filename) {
	char basepath[64] = "/home/tmp/adcData/";
	FILE *f;
	char *outfile;

	 form output filename
	outfile = strncat(basepath, filename, sizeof(basepath));
	syslog(LOG_INFO, "outfile: %s\n", outfile);

	f = fopen(outfile, "w");
	if(f == NULL) {
	    syslog(LOG_CRIT, "fopen %s -- %s",outfile, strerror(errno));
	    //FatalErrorHandler(0);
	}

	return f;
}

static void
writeToFile(FILE* f, void* buf, size_t size) {
	int n=0;

	n = fwrite(buf, 1, size, f);
	fflush(f);
	if(n != size) {
	    syslog(LOG_INFO, "Error: wrote only %d words, not %lu\n",n,size);
	}
}*/

rtems_task DaqControllerIrq(rtems_task_argument arg) {
	extern int errno;
	static rtems_id DaqControllerTID;
	VmeCrate *crateArray[NumVmeCrates];
	VmeModule *adcArray[NumAdcModules];
	int adcFramesPerTick;
	double adcFrequency = 0.0;
	double adcTrueFrequency = 0.0;
	VmeModule *dioArray[NumDioModules];
	AdcIsrArg *IsrArgList[NumAdcModules];
	static ReaderThreadArg *rdrArray[NumReaderThreads];
	RawDataSegment rdSegments[NumReaderThreads];
	rtems_id rawDataQID = 0;
	rtems_id dataHandlerTID = 0;
	rtems_status_code rc;
	rtems_event_set rdrSyncEvents = 0;
	rtems_event_set isrSyncEvents = 0;
	rtems_interval rtemsTicksPerSecond;
	int i,x;
	const int readSizeFrames = (HALF_FIFO_LENGTH/(/*4**/AdcChannelsPerFrame)); /* @ 32 channels/frame, readSizeFrames==508 */
	//FILE *fp = NULL;

	/* Begin... */
	syslog(LOG_INFO, "DaqControllerIrq: initializing...\n");
	rc = rtems_task_ident(RTEMS_SELF,RTEMS_LOCAL,&DaqControllerTID);
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);

	/* setup software-to-hardware objects */
	InitializeVmeCrates(crateArray, NumVmeCrates);

	if(adcFrequency==0.0) {
		/* use default */
		adcFrequency = AdcDefaultFrequency;
	}

	for(i=0; i<NumAdcModules; i++) {
		adcArray[i] = InitializeAdcModule(crateArray[i], ICS110B_DEFAULT_BASEADDR, adcFrequency, AdcChannelsPerFrame, &adcTrueFrequency);
		syslog(LOG_INFO, "Adc[%d] rate is %.6f kHz\n",i,adcTrueFrequency);
	}
	adcFramesPerTick = (int)ceil((adcTrueFrequency*HzPerkHz)/((double)rtemsTicksPerSecond));

	for(i=0; i<NumDioModules; i++) {
		dioArray[i] = InitializeDioModule(crateArray[dioConfig[i].vmeCrateID], dioConfig[i].baseAddr);
		syslog(LOG_INFO, "Initialized VMIC2536 DIO module[%d]\n",i);
	}

	/* register ADC interrupt service routines with sis1100 driver... */
	for(i=0; i<NumAdcModules; i++) {
		rtems_event_set event = 0;
		AdcIsrArg *parg = NULL;

		event = (1<<(24+i));
		isrSyncEvents |= event;
		parg = (AdcIsrArg *)calloc(1,sizeof(AdcIsrArg));
		if(parg==NULL) {
			syslog(LOG_INFO, "Failed to allocated AdcIsrArg[%d]\n",i);
			FatalErrorHandler(0);
		}
		parg->adc = adcArray[i];
		parg->controllerTID = DaqControllerTID;
		parg->irqEvent = event;
		//AdcInstallIsr(adcArray[i], AdcIsr, parg);
		rc = vme_set_isr(crateArray[i]->fd,
							adcArray[i]->irqVector/*vector*/,
							AdcIsr/*handler*/,
							parg/*handler arg*/);
		if(rc) {
			syslog(LOG_INFO, "Failed to set ADC Isr\n");
			FatalErrorHandler(0);
		}
		ICS110BSetIrqVector(adcArray[i], adcArray[i]->irqVector);
		/* arm the appropriate VME interrupt level at each sis3100 & ADC... */
		rc = vme_enable_irq_level(crateArray[i]->fd, adcArray[i]->irqLevel);
		TestDirective(rc, "vme_enable_irq_level()");
		ICS110BInterruptControl(adcArray[i],ICS110B_IRQ_ENABLE);
	}


	/* set up AdcReaderThreads and synchronize with them (Rendezvous pattern) */
	for(i=0; i<NumReaderThreads; i++) {
		rtems_event_set event = 0;

		event = (1<<(16+i));
		rdrSyncEvents |= event;
		rdrArray[i] = startReaderThread(adcArray[i], event);
		/* initialize the invariants of the raw-data segment array...*/
		rdSegments[i].adc = adcArray[i];
		rdSegments[i].numChannelsPerFrame = AdcChannelsPerFrame;
		rdSegments[i].numFrames = readSizeFrames;
	}
	RendezvousPoint(rdrSyncEvents);
	syslog(LOG_INFO, "DaqControllerIrq: synchronized with ReaderThreads...\n");

	/* fire up the AdcDataHandler thread...*/
	dataHandlerTID = StartDataHandler(NumReaderThreads);
	rc = rtems_message_queue_ident(RawDataQueueName, RTEMS_LOCAL, &rawDataQID);
	TestDirective(rc, "rtems_message_queue_ident()");

	/* fire up the AdcDataServer,DioWriteServer,DioReadServer interfaces */
	StartAdcDataServer();
	StartDioWriteServer(crateArray);
	/* FIXME -- unused! OCM feedback channel is via serial links... */
	StartDioReadServer(crateArray);

	/* FIXME -- testing: offload data to host over NFS */
	/*fp = getOutputFile("adc_1.dat");
	if(fp==NULL) {
		syslog(LOG_INFO, "DaqController: can't open %s -- %s\n","adc_1.dat",strerror(errno));
	}*/

	/* start ADC acquistion on the "edge" of a rtems_clock_tick()...*/
	rtems_task_wake_after(1);
	/* enable acquire at the ADCz */
	AdcStartAcquisition(adcArray, NumAdcModules);

	/* main acquisition loop */
	for(;;) {
		/* Wait for notification of ADC "fifo-half-full" event... */
		if(RendezvousPoint(isrSyncEvents)) {
			break; /* restart */
		}

		/* disable acquisition at the ADCs... */
		AdcStopAcquisition(adcArray, NumAdcModules);

		/* get raw-data buffers -- readSizeFrames*AdcChannelsPerFrame each...*/
		for(i=0; i<NumReaderThreads; i++) {
			rdSegments[i].buf = (int32_t *)calloc(1, readSizeFrames*AdcChannelsPerFrame*sizeof(int32_t));
			if(rdSegments[i].buf==NULL) {
				syslog(LOG_INFO, "Failed to c'allocate rdSegment buffers: %s", strerror(errno));
				exit(1);/* FIXME */
			}
			/* unleash the ReaderThreads... mmwwaahahaha... */
			rc = rtems_message_queue_send(rdrArray[i]->rawDataQID,&rdSegments[i],sizeof(RawDataSegment));
			TestDirective(rc, "DaqControllerIrq-->rtems_message_queue_send()-->ReaderThread queue");
		}
		/* block until the ReaderThreads are at their sync-point... */
		if(RendezvousPoint(rdrSyncEvents)) {
			break;
		}

		//rawData[x] = rdSegments[1].buf;

		/* reset (clear) each ADC FIFO, then re-enable acquisition... */
		for(i=0; i<NumReaderThreads; i++) {
			ICS110BFIFOReset(adcArray[i]);
		}
		AdcStartAcquisition(adcArray, NumAdcModules);

		/* reEnable irqs at each ADC and sis3100... */
		for(i=0; i<NumReaderThreads; i++) {
			rc = vme_enable_irq_level(crateArray[i]->fd, adcArray[i]->irqLevel);
			TestDirective(rc, "vme_enable_irq_level()");
			ICS110BInterruptControl(adcArray[i], ICS110B_IRQ_ENABLE);
		}
		/* hand raw-data buffers off to DataHandling thread */
		rc = rtems_message_queue_send(rawDataQID, rdSegments, sizeof(rdSegments));
		if(TestDirective(rc, "DaqControllerIrq-->rtems_message_queue_send()-->RawDataQueue")<0) {
			/*syslog(LOG_INFO, "DaqControllerIrq: suspending self...\n");
			rtems_task_wait(10);*/
			break;
		}

		/* If we have PowerSupply setpoints to demux, do that now.
		 *
		 * First, demux setpoints and toggle the LATCH of each ps-channel:
		 */

		/* Finally, toggle the UPDATE for each ps-controller (4 total) */

	} /* end main acquisition-loop */
	AdcStopAcquisition(adcArray, NumAdcModules);

	/* send data to host (opi2043-001) via NFS */
	/*if(fp != NULL) {
		syslog(LOG_INFO, "DaqController: sending data to host...\n");
		for(i=0; i<100; i++) {
			writeToFile(fp,rawData[i],readSizeFrames*AdcChannelsPerFrame*sizeof(int32_t));
		}
		syslog(LOG_INFO, "DaqController: done sending data...\n");
	}*/

	/* shut down interrupt generation/handling */
	for(i=0; i<NumAdcModules; i++) {
		rc = vme_disable_irq_level(crateArray[i]->fd, (1<<adcArray[i]->irqLevel));
		if(rc) {
			syslog(LOG_INFO, "DaqControllerIrq: failed to IRQ-mask for Adc[%d], rc=%d\n",crateArray[i]->fd,rc);
		}
		rc = vme_clr_isr(crateArray[i]->fd, adcArray[i]->irqVector);
		if(rc) {
			syslog(LOG_INFO, "DaqControllerIrq: failed to remove ISR for Adc[%d], rc=%d\n",crateArray[i]->fd,rc);
			//FatalErrorHandler(0);
		}
		free(IsrArgList[i]);
	}
	//rtems_task_suspend(DaqControllerTID);
	/* clean up resources */
	ShutdownAdcModules(adcArray, NumAdcModules);
	ShutdownDioModules(dioArray, NumDioModules);
	ShutdownVmeCrates(crateArray, NumVmeCrates);
	for(i=0; i<NumReaderThreads; i++) {
		rtems_message_queue_delete(rdrArray[i]->rawDataQID);
		rtems_task_delete(rdrArray[i]->readerTID);
		free(rdrArray[i]);
	}
	/* clean up DataHandler resources */
	syslog(LOG_INFO, "RawDataQueue: max outstanding msgs=%u\n", getMaxMsgs());
	rtems_task_delete(dataHandlerTID);
	rtems_message_queue_delete(rawDataQID);
	/* kill servers */
	DestroyAdcDataServer();
	DestroyDioWriteServer();
	DestroyDioReadServer();
	/* clean up self */
	/* FIXME -- rtems_region_delete(rawDataRID);*/
	syslog(LOG_INFO, "daqControllerIrq exiting... loop iterations=%d\n",x);
	rtems_task_restart(DaqControllerTID, 0);
	/*if(fp) {
		fflush(fp);
		fclose(fp);
	}
	rtems_task_suspend(RTEMS_SELF);*/
}
