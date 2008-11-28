#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <rtems.h>
#include <syslog.h>
#include <math.h>

#include <utils.h>
#include <dataDefs.h>
#include <OrbitController.h>
#include <DataHandler.h>
#include <bpmDefs.h> /* contains x/y channel scaling factors */



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


//#define LOWPASS_FILTER_FACTOR   1.015
//#define mmPerMeter              1000
#include <ics110bl.h>   /* need the definition of ADC_PER_VOLT */
//#define mmScaleFactor   LOWPASS_FILTER_FACTOR/(SamplesPerAvg*ShiftFactor*ADC_PER_VOLT*mmPerMeter)

static uint32_t maxMsgs = 0;

/* FIXME -- GLOBAL */
uint32_t SamplesPerAvg=5000; /* 0.5 seconds @ fs=10^4 Hz */
rtems_id ProcessedDataQueueId=0;

static void scaleBPMAverages(double* buf, uint32_t numSamples)
{
	int i;
	const int mmPerMeter = 1000;
	/* accounts for 24-bits of adc-data stuffed into the 3 MSB of a 32-bit word*/
	const int ShiftFactor = (1<<8);
	/* accounts for the voltage-divider losses of an LPF aft of each Bergoz unit */
	const double LPF_Factor = 1.015;
	double mmScaleFactor = LPF_Factor/(numSamples*ShiftFactor*ADC_PER_VOLT*mmPerMeter);

    for(i=0; i<TOTAL_BPMS; i++) {
		buf[i] *= (mmScaleFactor/XBPM_convFactor[i]);
		buf[i+TOTAL_BPMS] *= (mmScaleFactor/YBPM_convFactor[i]);
	}
}

/*
 * This routine will sort the BPM data from the internal representation
 * to a single array in (storage ring) cell-order, with the top-half of
 * the array occupied by horizontal (x) BPM values and the bottom-half
 * occupied by vertical (y) values.
 */
static void sortBPMData(double *sortedArray, double *rawArray) {
	int i,j;
	int nthAdc = 0;

	for(i=0,j=0; j<adc0ChMap_LENGTH; i++,j++) {
		/* bpmX*/
		sortedArray[i] = rawArray[nthAdc+adc0ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc0ChMap[j]+1];
	}
    nthAdc += AdcChannelsPerFrame;
    for(j=0; j<adc1ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc1ChMap[j]];
		/* bpmY */
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc1ChMap[j]+1];
	}
    nthAdc += AdcChannelsPerFrame;
    for(j=0; j<adc2ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc2ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc2ChMap[j]+1];
	}
    nthAdc += AdcChannelsPerFrame;
    for(j=0; j<adc3ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc3ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc3ChMap[j]+1];
	}
    nthAdc = 0;
    for(j=0; j<adc4ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc4ChMap[j]];
		/* bpmY */
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc4ChMap[j]+1];
	}
}

static void TransmitAvgs(double *sumArray) {
	ProcessedDataSegment pds;
	static double sortedArray[NumBpmChannels];
	rtems_status_code rc;
	rtems_id qid;
	extern uint32_t SamplesPerAvg;
	extern rtems_id ProcessedDataQueueId;
	rtems_name qname;

	memset(sortedArray,0,sizeof(double)*NumBpmChannels);
	sortBPMData(sortedArray, sumArray);
	scaleBPMAverages(sortedArray, SamplesPerAvg);

	/* check if we've got a queue yet to dump data into... */
	rc= rtems_object_get_classic_name(ProcessedDataQueueId, &qname);
	if(rc==RTEMS_SUCCESSFUL) {
		rc = rtems_message_queue_ident(qname, RTEMS_LOCAL, &qid);
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
	//static double sumsSqrd[NumAdcModules*AdcChannelsPerFrame];
	int numSegs = 0;
	extern uint32_t SamplesPerAvg;
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
					//sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
				}

			}
			numSamplesSummed++;

			if(numSamplesSummed==localSamplesPerAvg) {
				/* pass the avg'd BPM data on to the CS-interface */
				TransmitAvgs(sums);
				/* zero the array of running-sums,reset counter, update num pts in avg */
				memset(sums, 0, sizeof(double)*sizeof(sums)/sizeof(sums[0]));
				//memset(sumsSqrd, 0, sizeof(double)*sizeof(sumsSqrd)/sizeof(sumsSqrd[0]));
				numSamplesSummed=0;
				/* XXX - SamplesPerAvg can be set via orbitcontrolUI app */
				if(localSamplesPerAvg != SamplesPerAvg) {
					syslog(LOG_INFO, "DataHandler: changing localSamplesPerAvg=%d to\n\tSamplesPerAvg=%d\n",
							localSamplesPerAvg,SamplesPerAvg);
					localSamplesPerAvg = SamplesPerAvg;
				}
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
	/* raw-data queue: fed by OrbitController */
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
