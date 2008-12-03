#include <stdlib.h>
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
#include "OrbitController.h"
#include "dataDefs.h"
#include "AdcReaderThread.h"
#include "DataHandler.h"
#include "AdcDataServer.h"

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




static int RendezvousPoint(rtems_event_set syncEvents) {
	rtems_event_set eventsIn=0;
	rtems_status_code rc;
	
	rc = rtems_event_receive(syncEvents, RTEMS_EVENT_ALL|RTEMS_WAIT, RTEMS_NO_TIMEOUT, &eventsIn);
	//syslog(LOG_INFO, "orbitControllerIrq: RendezvousPoint() syncEvents=%#x, eventsIn=%#x\n",syncEvents, eventsIn);
	return TestDirective(rc, "RendezvousPoint()-->rtems_event_receive()");
}

/* NOTE: *all* access to VME equipment goes through this thread */
rtems_task orbitControllerIrq(rtems_task_argument arg) {
	extern int errno;
	extern uint32_t maxMsgs;
	static rtems_id DaqControllerTID;
	VmeCrate *crateArray[NumVmeCrates];
	VmeModule *adcArray[NumAdcModules];
	int adcFramesPerTick;
	double adcFrequency = 0.0;
	double adcTrueFrequency = 0.0;
	VmeModule *dioArray[NumDioModules];
	static ReaderThreadArg *rdrArray[NumReaderThreads];
	RawDataSegment rdSegments[NumReaderThreads];
	rtems_id rawDataQID = 0;
	rtems_id dataHandlerTID = 0;
	/*rtems_id rawDataRID = 0;
	void *rawDataRegion = NULL;
	size_t rawDataRegionSize = 0;
	size_t rawDataRegionPageSize = 0;*/
	rtems_status_code rc;
	rtems_event_set rdrSyncEvents = 0;
	rtems_event_set isrSyncEvents = 0;
	rtems_interval rtemsTicksPerSecond;
	int i,loopIterations;
	int isAdcReady = 0;
	const int readSizeFrames = (HALF_FIFO_LENGTH/AdcChannelsPerFrame); /* @ 32 channels/frame, readSizeFrames==508 */
	
	/* Begin... */
	syslog(LOG_INFO, "orbitControllerIrq: initializing...\n");
	rc = rtems_task_ident(RTEMS_SELF,RTEMS_LOCAL,&DaqControllerTID);
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);
	
	/* setup software-to-hardware objects */
	InitializeVmeCrate(crateArray, NumVmeCrates);
	
	if(adcFrequency==0.0) {
		/* use default */
		adcFrequency = AdcDefaultFrequency;
	}

	for(i=0; i<NumAdcModules; i++) {
		adcArray[i] = InitializeAdcModule(crateArray[i], ICS110B_DEFAULT_BASEADDR, adcFrequency, AdcChannelsPerFrame, &adcTrueFrequency);
		syslog(LOG_INFO, "Adc[%d] rate is %.9f\n",i,adcTrueFrequency);
	}
	adcFramesPerTick = (int)ceil((adcTrueFrequency*HzPerkHz)/((double)rtemsTicksPerSecond));
	
	for(i=0; i<NumDioModules; i++) {
		dioArray[i] = InitializeDioModule(crateArray[dioConfig[i].vmeCrateID], dioConfig[i].baseAddr);
		syslog(LOG_INFO, "Initialized VMIC2536 DIO module[%d]\n",i);
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
	syslog(LOG_INFO, "orbitControllerIrq: synchronized with ReaderThreads...\n");
	
	/* fire up the DataHandler thread...*/
	dataHandlerTID = StartDataHandler(NumReaderThreads);
	rc = rtems_message_queue_ident(RawDataQueueName, RTEMS_LOCAL, &rawDataQID);
	TestDirective(rc, "rtems_message_queue_ident()");
	
	/* fire up the AdcDataServer interface */
	StartAdcDataServer();
	
	/* FIXME -- just using malloc/free for now... 
	 * use Regions instead of Partitions (dynamically sized vs. fixed) 
	 * set up the pool of raw adc-data buffers: roughly 5.2 MB
	 */ 
	/*rawDataRegionSize = 10*NumReaderThreads*HALF_FIFO_LENGTH*sizeof(uint32_t) + 4096fudge-space;
	rawDataRegion = (void *)calloc(1,rawDataRegionSize);
	 chk alignment too ! 
	if((rawDataRegion==NULL) || (((uint32_t)rawDataRegion%4)!=0)) {
		syslog(LOG_INFO, "Failed to allocate rawDataRegion\n");
		FatalErrorHandler(0);
	}
	rawDataRegionPageSize = HALF_FIFO_LENGTH*sizeof(uint32_t);
	rc = rtems_region_create(rtems_build_name('R','A','W','D'),
								rawDataRegionstart addr,
								rawDataRegionSize,
								rawDataRegionPageSizepage-size,
								RTEMS_DEFAULT_ATTRIBUTESFIFO wait ordering,
								&rawDataRID);
	TestDirective(rc, "rtems_region_create()");*/
	
	/* start ADC acquistion on the "edge" of a rtems_clock_tick()...*/
	rtems_task_wake_after(1);
	/* enable acquire at the ADCz */
	AdcStartAcquisition(adcArray, NumAdcModules);
	
	loopIterations=100000;
	
	/* main acquisition loop */
	while(loopIterations--) {
		/* FIXME -- need to POLL for successful Rendezvous condition,
		 * 			otherwise perform some DIO module interaction while
		 * 			we wait... 
		 * Get notification of ADC "fifo-half-full" event...
		 */ 
		while(ready != 0) {
			/* if we have DIO values to get/set, do that now...*/
			rtems_task_wake_after(1);
			for(i=0; i<NumAdcModules; i++) {
				if(ICS110BIsHalfFull(adcArray[i])) { ready--; }
			}
		}
		
		/* FIXME -- get raw-data buffers -- readSizeFrames*AdcChannelsPerFrame each...*/
		for(i=0; i<NumReaderThreads; i++) {
			/*rc = rtems_region_get_segment(rawDataRID, 
											rawDataRegionPageSizesize,
											RTEMS_NO_WAIToption set,
											RTEMS_NO_TIMEOUT,
											&rdSegments[i].buf);
			TestDirective(rc, "rtems_region_get_segment()");*/
			rdSegments[i].buf = (int32_t *)calloc(1, readSizeFrames*AdcChannelsPerFrame*sizeof(int32_t));
			if(rdSegments[i].buf==NULL) {
				syslog(LOG_INFO, "Failed to c'allocate rdSegment buffers: %s", strerror(errno));
				exit(1);
			}
			/* unleash the ReaderThreads... mmwwaahahaha... */
			rc = rtems_message_queue_send(rdrArray[i]->rawDataQID,&rdSegments[i],sizeof(RawDataSegment));
			TestDirective(rc, "orbitControllerIrq-->rtems_message_queue_send()-->ReaderThread queue");
		}
		/* block until the ReaderThreads are at their sync-point... */
		if(RendezvousPoint(rdrSyncEvents)) {
			break;
		}
		
		/* hand raw-data buffers off to DataHandling thread */
		rc = rtems_message_queue_send(rawDataQID, rdSegments, sizeof(rdSegments));
		if(TestDirective(rc, "orbitControllerIrq-->rtems_message_queue_send()-->RawDataQueue")<0) {
			syslog(LOG_INFO, "orbitControllerIrq: suspending self...\n");
			break;
		}
		
	} /* end main acquisition-loop */
	AdcStopAcquisition(adcArray, NumAdcModules);
	
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
	syslog(LOG_INFO, "RawDataQueue: max outstanding msgs=%u\n", maxMsgs);
	rtems_task_delete(dataHandlerTID);
	rtems_message_queue_delete(rawDataQID);
	/* kill AdcDataServer */
	DestroyAdcDataServer();
	/* clean up self */
	/* FIXME -- rtems_region_delete(rawDataRID);*/
	syslog(LOG_INFO, "orbitControllerIrq exiting... loop iterations=%d\n",loopIterations);
	rtems_task_restart(DaqControllerTID, 0);
}
