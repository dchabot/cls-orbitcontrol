/*
 * DataHandler.cc
 *
 *  Created on: Dec 15, 2008
 *      Author: chabotd
 */

#include "DataHandler.h"
#include "OrbitController.h"
#include <OrbitControlException.h>
#include <iostream>
#include <syslog.h>
#include <utils.h>
#include <bpmDefs.h> /* contains x/y channel scaling factors */
#include <ics110bl.h> /* need the definition of ADC_PER_VOLT */
#include <stdlib.h>

DataHandler* DataHandler::instance = 0;

DataHandler::DataHandler() :
	//ctor-initializer list
	arg(0),priority(OrbitControllerPriority+3),
	tid(0),inpQueueId(0)
{ 	syslog(LOG_INFO, "DataHandler instance ctor!!\n"); }

DataHandler* DataHandler::getInstance() {
	if(instance == 0) {
		instance = new DataHandler();
	}
	return instance;
}

void DataHandler::destroyInstance() {
	delete instance;
}

DataHandler::~DataHandler() {
	if(tid) { rtems_task_delete(tid); }
	if(inpQueueId) { rtems_message_queue_delete(inpQueueId); }
	syslog(LOG_INFO, "DataHandler dtor!!\n");
}

rtems_task DataHandler::threadStart(rtems_task_argument arg) {
	DataHandler *dh = (DataHandler*)arg;
	dh->threadBody(dh->arg);
}

rtems_task DataHandler::threadBody(rtems_task_argument arg) {

}

void DataHandler::enqueRawData(RawDataSegment *ds) const {
	rtems_status_code rc;
	rc = rtems_message_queue_send(inpQueueId, ds, sizeof(RawDataSegment)*NumAdcModules);
	if(TestDirective(rc, "DataHandler--rtems_message_queue_send()-->RawDataQueue")<0) {
		throw OrbitControlException("DataHandler: enque failure!!",rc);
	}
}

/** Create thread and input message queue, and start the thread */
void DataHandler::start(rtems_task_argument arg) {
	rtems_status_code rc;
	threadName = rtems_build_name('D','H','T','N');
	rc = rtems_task_create(threadName,
							priority,
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_FLOATING_POINT|RTEMS_LOCAL,
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&tid);
	if(TestDirective(rc, "Data Handler -- rtems_task_create()")) {
		throw OrbitControlException("DataHandler: problem creating task!!",rc);
	}
	/* raw-data queue: fed by OrbitController */
	inpQueueName = rtems_build_name('D','H','I','q');
	rc = rtems_message_queue_create(inpQueueName,
									10/*max queue size*/,
									sizeof(RawDataSegment)*NumAdcModules/*msg size*/,
									RTEMS_LOCAL|RTEMS_FIFO,
									&inpQueueId);
	if(TestDirective(rc, "DataHandler -- rtems_message_queue_create()")) {
		throw OrbitControlException("DataHandler: problem creating msq queue!!",rc);
	}

	this->arg = arg;
	rc = rtems_task_start(tid,threadStart,(rtems_task_argument)this);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to start DataHandler thread: %s\n",rtems_status_text(rc));
		throw OrbitControlException("Couldn't start DataHandler thread!!!",rc);
	}
	syslog(LOG_INFO, "Started DataHandler with priority %d\n",priority);

}

void DataHandler::scaleBPMAverages(double *buf, uint32_t numSamples) const {
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

void DataHandler::sortBPMData(double *sortedArray, double *rawArray) const {
	int i,j;
	int nthAdc = 0;

	for(i=0,j=0; j<adc0ChMap_LENGTH; i++,j++) {
		/* bpmX*/
		sortedArray[i] = rawArray[nthAdc+adc0ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc0ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc1ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc1ChMap[j]];
		/* bpmY */
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc1ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc2ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc2ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc2ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
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

/**
 * Produces a running-sum of the BPM data in rdSegments.
 *
 * @param sums
 * @param rdSegments Pointer to an array of (4) RawDataSegments.
 */
void DataHandler::sumBPMChannelData(double* sums, RawDataSegment* rdSegments) {
	uint32_t nthFrame,nthAdc,nthChannel;
	int32_t numSamplesSummed=0;

	/* XXX -- assumes each rdSegment has an equal # of frames... */
	for(nthFrame=0; nthFrame<rdSegments[0].numFrames; nthFrame++) { /* for each ADC frame in rdSegment[].buf... */
		int nthFrameOffset = nthFrame*adcChannelsPerFrame;

		for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
			int nthAdcOffset = nthAdc*adcChannelsPerFrame;

			for(nthChannel=0; nthChannel<adcChannelsPerFrame; nthChannel++) { /* for each channel of this frame... */
				sums[nthAdcOffset+nthChannel] += (double)(rdSegments[nthAdc].buf[nthFrameOffset+nthChannel]);
				//sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
			}

		}
		numSamplesSummed++;

		/*if(numSamplesSummed==localSamplesPerAvg) {
			 pass the avg'd BPM data on to the CS-interface
			TransmitAvgs(sums);
			 zero the array of running-sums,reset counter, update num pts in avg
			memset(sums, 0, sizeof(double)*sizeof(sums)/sizeof(sums[0]));
			//memset(sumsSqrd, 0, sizeof(double)*sizeof(sumsSqrd)/sizeof(sumsSqrd[0]));
			numSamplesSummed=0;
			 XXX - SamplesPerAvg can be set via orbitcontrolUI app
			if(localSamplesPerAvg != SamplesPerAvg) {
				syslog(LOG_INFO, "DataHandler: changing localSamplesPerAvg=%d to\n\tSamplesPerAvg=%d\n",
						localSamplesPerAvg,SamplesPerAvg);
				localSamplesPerAvg = SamplesPerAvg;
			}
		}*/

	}

	/* release allocated memory */
	for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) {
		free(rdSegments[nthAdc].buf);
	}
}
