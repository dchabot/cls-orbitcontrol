#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include <ics110bl.h>
#include <utils.h> /*TestDirective()*/

//#include <rtems-gdb-stub.h>
#include "DaqController.h"
#include "dataDefs.h"
#include "AdcReaderThread.h"
#include "DataHandler.h"

static void WaitAtRendezvousPoint(rtems_event_set syncEvents) {
	rtems_event_set eventsIn=0;
	rtems_status_code rc;
	
	rc = rtems_event_receive(syncEvents, RTEMS_EVENT_ALL|RTEMS_WAIT, 10000, &eventsIn);
	TestDirective(rc, "rtems_event_receive()");
}

rtems_task daqControllerPeriodic(rtems_task_argument arg) {
	extern int errno;
	rtems_id DaqControllerTID;
	VmeCrate *crateArray[NumVmeCrates];
	VmeModule *adcArray[NumAdcModules];
	int adcFramesPerTick;
	double adcFrequency = 0.0;
	double adcTrueFrequency = 0.0;
	VmeModule *dioArray[NumDioModules];
	ReaderThreadArg *rdrArray[NumReaderThreads];
	RawDataSegment rdSegments[NumReaderThreads];
	rtems_id rawDataQID = 0;
	/*rtems_id rawDataRID = 0;
	void *rawDataRegion = NULL;
	size_t rawDataRegionSize = 0;
	size_t rawDataRegionPageSize = 0;*/
	rtems_name rtmonPeriodName;
	rtems_id rtmonPeriodID;
	rtems_status_code rc;
	static rtems_event_set rdrSyncEvents;
	rtems_interval rtmonTicksPerPeriod = 1;
	rtems_interval adcFramesPerRtmonPeriod;
	rtems_interval rtemsTicksPerSecond;
	int i,x;
	uint32_t numAcquires = 2;
	
	
	/* Begin... */
	syslog(LOG_INFO, "DaqControllerPeriodic: initializing...\n");
	rc = rtems_task_ident(RTEMS_SELF,RTEMS_LOCAL,&DaqControllerTID);
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);
	
	/* setup software-to-hardware objects */
	InitializeVmeCrates(crateArray, NumVmeCrates);
	
	if(adcFrequency==0.0) {
		/* use default */
		adcFrequency = AdcDefaultFrequency;
	}
	InitializeAdcModules(adcArray, crateArray, NumAdcModules, adcFrequency, AdcChannelsPerFrame, &adcTrueFrequency);
	adcFramesPerTick = (int)floor((adcTrueFrequency*HzPerkHz)/((double)rtemsTicksPerSecond));
	adcFramesPerRtmonPeriod = adcFramesPerTick*rtmonTicksPerPeriod;
	syslog(LOG_INFO, "adcFramesPerRtmonPeriod=%d\n",adcFramesPerRtmonPeriod);
	
	InitializeDioModules(dioArray, crateArray, NumDioModules);
	
	/* set up AdcReaderThreads and synchronize with them (Rendezvous pattern) */
	for(i=0; i<NumReaderThreads; i++) {
		rtems_event_set event = 0;
		
		event = (1<<(16+i));
		rdrSyncEvents |= event;
		rdrArray[i] = startReaderThread(adcArray[i], event);
		/* initialize the invariants of the raw-data segment array...*/ 
		rdSegments[i].adc = adcArray[i];
		rdSegments[i].numChannelsPerFrame = AdcChannelsPerFrame;
		rdSegments[i].numFrames = HALF_FIFO_LENGTH/(2*AdcChannelsPerFrame);
	}
	WaitAtRendezvousPoint(rdrSyncEvents);
	syslog(LOG_INFO, "Synchronized with ReaderThreads...\n");
	
	/* fire up the DataHandler thread...*/
	StartDataHandler(NumReaderThreads);
	rc = rtems_message_queue_ident(RawDataQueueName, RTEMS_LOCAL, &rawDataQID);
	TestDirective(rc, "rtems_message_queue_ident()");
	
	/* set up our periodic invocation mechanism... */
	rtmonPeriodName = rtems_build_name('P','E','R',(char)1);
	rc = rtems_rate_monotonic_create(rtmonPeriodName, &rtmonPeriodID);
	TestDirective(rc, "rtems_rate_monotonic_create()");
	
	/* FIXME -- use Regions instead of Partitions (dynamically sized vs. fixed) 
	 * set up the pool of raw adc-data buffers: roughly 5.25 MB
	 */ 
	/*rawDataRegionSize = 1024*NumReaderThreads*(adcFramesPerRtmonPeriod+1)*AdcChannelsPerFrame*sizeof(uint32_t) + 4096fudge-space;
	rawDataRegion = (void *)calloc(1,rawDataRegionSize);
	 chk alignment too ! 
	if((rawDataRegion==NULL) || (((uint32_t)rawDataRegion%4)!=0)) {
		syslog(LOG_INFO, "Failed to allocate rawDataRegion\n");
		FatalErrorHandler(0);
	}
	rawDataRegionPageSize = AdcChannelsPerFrame*sizeof(uint32_t);
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
	rc = rtems_rate_monotonic_period(rtmonPeriodID, rtmonTicksPerPeriod);
	TestDirective(rc, "rtems_rate_monotonic_period()");
	x=10000;
	
	/* main acquisition loop */
	while(--x) {
		uint32_t numFrames = 0;
		
		/* re-arm rate-monotonic period && block until the current period expires... */
		rc = rtems_rate_monotonic_period(rtmonPeriodID, rtmonTicksPerPeriod);
		TestDirective(rc, "rtems_rate_monotonic_period()");

		/* At 10 frames per rtmonPeriod, read 9 frames, then 10, then 9, etc. */
		if((numAcquires%2)==0) {
			numFrames = adcFramesPerRtmonPeriod; /* 9 */
		}
		else {
			numFrames = adcFramesPerRtmonPeriod+1;
		}
		numAcquires++;

		/* FIXME -- get raw-data buffers -- 1/4 FIFO each...*/
		for(i=0; i<NumReaderThreads; i++) {
			/*rc = rtems_region_get_segment(rawDataRID, 
											rawDataRegionPageSizesize,
											RTEMS_NO_WAIToption set,
											RTEMS_NO_TIMEOUT,
											&rdSegments[i].buf);
			TestDirective(rc, "rtems_region_get_segment()");*/
			rdSegments[i].buf = (int32_t *)calloc(1, 0.5*HALF_FIFO_LENGTH*sizeof(int32_t));
			if(rdSegments[i].buf==NULL) {
				syslog(LOG_INFO, "Failed to c'allocate rdSegment buffers: %s", strerror(errno));
				exit(1);
			}
			/* unleash the ReaderThreads... mmwwaahahaha... */
			rc = rtems_message_queue_send(rdrArray[i]->rawDataQID,&rdSegments[i],sizeof(RawDataSegment));
			TestDirective(rc, "rtems_message_queue_send()");
		}
		
		/* block until the ReaderThreads are at their sync-point... */
		WaitAtRendezvousPoint(rdrSyncEvents);
		
		/* hand raw-data buffers off to DataHandling thread */
		rc = rtems_message_queue_send(rawDataQID, rdSegments, sizeof(rdSegments));
		TestDirective(rc, "rtems_message_queue_send()");
		
		/* FIXME -- if we have DIO values to get/set, do that now...*/
	}
	AdcStopAcquisition(adcArray, NumAdcModules);
	
	/* check out how much data is left in each ADC FIFO:
	 * 	this is a measure of how well we can keep up...
	 */
	for(i=0; i<NumAdcModules; i++) {
		for(x=0; !ICS110BIsEmpty(adcArray[i]); x++) {
			VmeRead_32(adcArray[i], ICS110B_FIFO_OFFSET);
		}
		syslog(LOG_INFO, "adc[%d]: fifo has %d frames, %d channels remaining\n",i, x/AdcChannelsPerFrame, x);
	}
	
	/* clean up resources */
	syslog(LOG_INFO, "daqControllerIrq: nuking self...\n");
	ShutdownAdcModules(adcArray, NumAdcModules);
	ShutdownDioModules(dioArray, NumDioModules);
	ShutdownVmeCrates(crateArray, NumVmeCrates);
	for(i=0; i<NumReaderThreads; i++) {
		rtems_message_queue_delete(rdrArray[i]->rawDataQID);
		rtems_task_delete(rdrArray[i]->readerTID);
		free(rdrArray[i]);
	}
	/* FIXME -- rtems_region_delete(rawDataRID);*/
	rtems_task_delete(RTEMS_SELF);
}
