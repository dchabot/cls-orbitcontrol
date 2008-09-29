#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <rtems.h>
#include <syslog.h>

#include <utils.h>
#include "dataDefs.h"
#include "DaqController.h"
#include "DataHandler.h"
#include "AdcDataServer.h"
#include <bpmDefs.h> /* contains x/y channel scaling factors */

#include <assert.h>
#define NDEBUG

/* lifted from devICS110BlSrBpms.c */
/* here are the ADC channel mappings based on the current drawings*/
/* these channels may be moved around in teh future */
/* structure is x,y,x,y,x,y... map positions indicate the X positions ONLY ! */
#define adc0ChMap_LENGTH 4  /* 2404.1 */
int adc0ChMap[adc0ChMap_LENGTH] = {9*2,10*2,11*2,12*2};

#define adc1ChMap_LENGTH 14 /* 2406.1 */
int adc1ChMap[adc1ChMap_LENGTH] = {1*2,2*2,3*2,0*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,11*2,12*2,13*2};

#define adc2ChMap_LENGTH 14 /* 2406.3 */
int adc2ChMap[adc2ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,6*2,7*2,8*2,9*2,5*2,10*2,11*2,12*2,13*2};

/* April 28/08 BPM1408-01 replaced with Libera unit */
/* May 1/08 BPM1410-01 replaced with Libera unit */
#define adc3ChMap_LENGTH 13 /* 2408.1 */
int adc3ChMap[adc3ChMap_LENGTH] = {1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,11*2,12*2,13*2,14*2};

#define adc4ChMap_LENGTH 9
int adc4ChMap[adc4ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2};

#define TOTAL_BPMS      (adc0ChMap_LENGTH + adc1ChMap_LENGTH + adc2ChMap_LENGTH + adc3ChMap_LENGTH + adc4ChMap_LENGTH)
#define NumBpmChannels  2*TOTAL_BPMS

/* accounts for the voltage-divider losses of an LPF aft of each Bergoz unit */
//#define LOWPASS_FILTER_FACTOR   1.015
//#define mmPerMeter              1000
#include <ics110bl.h>   /* need the definition of ADC_PER_VOLT */
//#define mmScaleFactor   LOWPASS_FILTER_FACTOR/(SamplesPerAvg*ShiftFactor*ADC_PER_VOLT*mmPerMeter)

static uint32_t maxMsgs = 0;

/* This ugly: no easy way around that though... */
static void SortBpmData(double *sortedArray, double *sumArray) {
	int i,j;
	int nthAdc = 0;/*index into sumArray*/
	const int mmPerMeter = 1000;
	const int ShiftFactor = 256;
	const double LOWPASS_FILTER_FACTOR = 1.015;
	double mmScaleFactor = LOWPASS_FILTER_FACTOR/(SamplesPerAvg*ShiftFactor*ADC_PER_VOLT*mmPerMeter);

	/* Adc[0]-->chMaps 0*/
	for(i=0,j=0; j<adc0ChMap_LENGTH; i++,j++) {
		/* bpmX*/
		sortedArray[i] = sumArray[nthAdc+adc0ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = sumArray[nthAdc+adc0ChMap[j]+1];
	}
	assert(i=4);
	/* Adc[1]*/
	nthAdc += AdcChannelsPerFrame;
	for(j=0; j<adc1ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = sumArray[nthAdc+adc1ChMap[j]];
		/* bpmY */
		sortedArray[i+TOTAL_BPMS] = sumArray[nthAdc+adc1ChMap[j]+1];
	}
	assert(i==18);
	/* Adc[2] */
	nthAdc += AdcChannelsPerFrame;
	for(j=0; j<adc2ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = sumArray[nthAdc+adc2ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = sumArray[nthAdc+adc2ChMap[j]+1];
	}
	assert(i==32);
	/* Adc[3] */
	nthAdc += AdcChannelsPerFrame;
	for(j=0; j<adc3ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = sumArray[nthAdc+adc3ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = sumArray[nthAdc+adc3ChMap[j]+1];
	}
	assert(i==45);
	/* Adc[0]-->chMap 4...Again!! */
	nthAdc = 0;
	for(j=0; j<adc4ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = sumArray[nthAdc+adc4ChMap[j]];
		/* bpmY */
		sortedArray[i+TOTAL_BPMS] = sumArray[nthAdc+adc4ChMap[j]+1];
	}
	/* sanity chk... */
	assert(i==TOTAL_BPMS);
	/*XXX scale all the now-sorted values (final dimensions of [m]) */
	for(i=0; i<TOTAL_BPMS; i++) {
		/*sortedArray[i] *= mmScaleFactor;
		sortedArray[i+TOTAL_BPMS] *= mmScaleFactor;*/
		sortedArray[i] *= (mmScaleFactor/XBPM_convFactor[i]);
		sortedArray[i+TOTAL_BPMS] *= (mmScaleFactor/YBPM_convFactor[i]);
	}
}

static void TransmitAvgs(double *sumArray) {
	ProcessedDataSegment pds;
	static double sortedArray[NumBpmChannels];
	rtems_status_code rc;
	rtems_id qid;

	memset(sortedArray,0,sizeof(double)*NumBpmChannels);
	SortBpmData(sortedArray, sumArray);

	/* check if we've got a queue yet to dump data into... */
	rc = rtems_message_queue_ident(ProcessedDataQueueName, RTEMS_LOCAL, &qid);
	if(rc==RTEMS_SUCCESSFUL) {
		/* ship the avg'd ADC data off to AdcDataServer */
		pds.buf = (double *)calloc(1, sizeof(double)*NumBpmChannels);
		if(pds.buf==NULL) {
			syslog(LOG_INFO, "DataHandler: failed to allocate ProcessedDataSegment\n");
			FatalErrorHandler(0);
		}
		pds.numElements = NumBpmChannels;
		memcpy(pds.buf, sortedArray, pds.numElements*sizeof(double));

		rc = rtems_message_queue_send(qid, &pds, sizeof(ProcessedDataSegment));
		TestDirective(rc, "DataHandler: rtems_message_queue_send()");
	}
	/* just silently keep going if the queue is unavailable */
}


static rtems_task DataHandler(rtems_task_argument arg) {
	rtems_status_code rc;
	rtems_id rawDataQID;
	size_t rcvSize;
	int nthFrame,nthAdc,nthChannel;
	int32_t numSamplesSummed=0;
	static RawDataSegment rdSegments[NumAdcModules];
	static double sums[NumAdcModules*AdcChannelsPerFrame];
	int numSegs = 0;
	uint32_t localSamplesPerAvg = SamplesPerAvg;

	rc = rtems_message_queue_ident(RawDataQueueName, RTEMS_LOCAL, &rawDataQID);
	TestDirective(rc, "DataHandler: rtems_message_queue_ident()");

	syslog(LOG_INFO, "DataHandler: entering main loop...\n");
	for(;;) {
		uint32_t tmpMsgCount = 0;

		rc = rtems_message_queue_get_number_pending(rawDataQID, &tmpMsgCount);
		TestDirective(rc, "DataHandler: rtems_message_queue_get_number_pending()\n");
		if(tmpMsgCount > maxMsgs) {
			maxMsgs = tmpMsgCount;
		}
		/* grab some raw data */
		rc = rtems_message_queue_receive(rawDataQID, rdSegments, &rcvSize, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
		TestDirective(rc, "DataHandler: rtems_message_queue_receive()");
		if(rcvSize!=sizeof(rdSegments)) {
			syslog(LOG_INFO, "DataHandler: incorrect size (%d bytes) of rawDataQueue msg!!\n",rcvSize);
		}
		numSegs++;

		/* NOTE: assumes each rdSegment has an equal # of frames... */
		for(nthFrame=0; nthFrame<rdSegments[0].numFrames; nthFrame++) { /* for each ADC frame in rdSegment[].buf... */
			int nthFrameOffset = nthFrame*AdcChannelsPerFrame;

			for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
				int nthAdcOffset = nthAdc*AdcChannelsPerFrame;

				for(nthChannel=0; nthChannel<AdcChannelsPerFrame; nthChannel++) { /* for each channel of this frame... */
					sums[nthAdcOffset+nthChannel] += (double)(rdSegments[nthAdc].buf[nthFrameOffset+nthChannel]);
				}

			}
			numSamplesSummed++;

			if(numSamplesSummed==SamplesPerAvg) {
				/* pass the avg'd BPM data on to the CS-interface */
				TransmitAvgs(sums);
				/* zero the array of running-sums,reset counter, update num pts in avg */
				memset(sums, 0, sizeof(double)*sizeof(sums)/sizeof(sums[0]));
				numSamplesSummed=0;
				/* XXX - SamplesPerAvg is set via BpmSamplesPerAvgServer (UI) */
				localSamplesPerAvg = SamplesPerAvg;
			}

		}

		/* release allocated memory and go wait for more RawDataSegments... */
		for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) {
			free(rdSegments[nthAdc].buf);
		}

	} /* end for(;;) */
}

uint32_t getMaxMsgs(void) {
	return maxMsgs;
}

rtems_id StartDataHandler(uint32_t numReaderThreads) {
	rtems_status_code rc;
	rtems_id tid;
	rtems_id qid;

	rc = rtems_task_create(DataHandlerThreadName,
							DefaultPriority+3,
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_FLOATING_POINT|RTEMS_LOCAL,
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&tid);
	TestDirective(rc, "rtems_task_create()");
	/* raw-data queue: fed by DaqController */
	rc = rtems_message_queue_create(RawDataQueueName,
									10/*max queue size*/,
									sizeof(RawDataSegment)*numReaderThreads/*msg size*/,
									RTEMS_LOCAL|RTEMS_FIFO,
									&qid);
	TestDirective(rc, "rtems_message_queue_create()");

	rc = rtems_task_start(tid, DataHandler, numReaderThreads);
	TestDirective(rc, "rtems_task_start()");

	return tid;
}
