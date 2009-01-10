/*
 * EpicsBpmController.cpp
 *
 *  Created on: Jan 9, 2009
 *      Author: chabotd
 */

#include "EpicsBpmController.h"
#include <syslog.h>


EpicsBpmController* EpicsBpmController::instance = 0;

EpicsBpmController::EpicsBpmController() :
	//ctor-initializer list
	samplesPerAvg(0),
	priority(199-OrbitControllerPriority+1),//see epicsThreadGetOssPriorityValue
	thread(*this,"BPMCtlr",epicsThreadGetStackSize(epicsThreadStackBig),priority),
	msgSize(sizeof(AdcData*)*NumAdcModules),
	maxMsgs(10),
	inpQ(maxMsgs, msgSize)
{ }

EpicsBpmController::~EpicsBpmController() { }

EpicsBpmController* EpicsBpmController::getInstance() {
	if(instance==0) {
		instance = new EpicsBpmController();
		thread.start();
	}
	return instance;
}

void EpicsBpmController::destroyInstance() {
	thread.exitWait(1.0);
	syslog(LOG_INFO, "BpmController ~dtor!!\n");
}

void EpicsBpmController::registerBpm(Bpm *bpm) {
}

void EpicsBpmController::unregisterBpm(Bpm *bpm) {
}

Bpm *EpicsBpmController::getBpm(const string & id) {

	return NULL;
}

void EpicsBpmController::enqueAdcData(AdcData** data) {
	if(inpQ.send((void*)data,msgSize)) {
		//FIXME -- not much we can do here...
		inpQ.show(3);
	}
}

/*void scaleBPMAverages(double *buf, uint32_t numSamples) const {
	int i;
	const int mmPerMeter = 1000;
	 accounts for 24-bits of adc-data stuffed into the 3 MSB of a 32-bit word
	const int ShiftFactor = (1<<8);
	 accounts for the voltage-divider losses of an LPF aft of each Bergoz unit
	const double LPF_Factor = 1.015;
	double mmScaleFactor = LPF_Factor/(numSamples*ShiftFactor*ADC_PER_VOLT*mmPerMeter);

	for(i=0; i<TOTAL_BPMS; i++) {
		buf[i] *= (mmScaleFactor/XBPM_convFactor[i]);
		buf[i+TOTAL_BPMS] *= (mmScaleFactor/YBPM_convFactor[i]);
	}
}

void sortBPMData(double *sortedArray, double *rawArray) const {
	int i,j;
	int nthAdc = 0;

	for(i=0,j=0; j<adc0ChMap_LENGTH; i++,j++) {
		 bpmX
		sortedArray[i] = rawArray[nthAdc+adc0ChMap[j]];
		 bpmY
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc0ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc1ChMap_LENGTH; i++,j++) {
		 bpmX
		sortedArray[i] = rawArray[nthAdc+adc1ChMap[j]];
		 bpmY
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc1ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc2ChMap_LENGTH; i++,j++) {
		 bpmX
		sortedArray[i] = rawArray[nthAdc+adc2ChMap[j]];
		 bpmY
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc2ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc3ChMap_LENGTH; i++,j++) {
		 bpmX
		sortedArray[i] = rawArray[nthAdc+adc3ChMap[j]];
		 bpmY
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc3ChMap[j]+1];
	}
	nthAdc = 0;
	for(j=0; j<adc4ChMap_LENGTH; i++,j++) {
		 bpmX
		sortedArray[i] = rawArray[nthAdc+adc4ChMap[j]];
		 bpmY
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc4ChMap[j]+1];
	}
}*/

//the thread-body:
void EpicsBpmController::run() {
	uint32_t localSamplesPerAvg = samplesPerAvg;
	uint32_t nthFrame,nthAdc,nthChannel;
	uint32_t numSamplesSummed=0;

	syslog(LOG_INFO, "BpmController: entering main loop...\n");
	for(;;) {
		uint32_t bytes = inpQ.receive((void*)ds,msgSize);
		if(bytes%msgSize) {
			syslog(LOG_INFO, "BpmController: received %u bytes in msg: was expecting %u\n",
								bytes,msgSize);
			//FIXME -- handle this error!!!
		}
		/* XXX -- assumes each rdSegment has an equal # of frames... */
		for(nthFrame=0; nthFrame<ds[nthFrame]->getFrames(); nthFrame++) { /* for each ADC frame in rdSegment[].buf... */
			int nthFrameOffset = nthFrame*ds[nthFrame]->getChannelsPerFrame();

			for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
				int nthAdcOffset = nthAdc*ds[nthAdc]->getChannelsPerFrame();

				for(nthChannel=0; nthChannel<ds[nthAdc]->getChannelsPerFrame(); nthChannel++) { /* for each channel of this frame... */
					//sums[nthAdcOffset+nthChannel] += (double)(ds[nthAdc]->getBuffer()[nthFrameOffset+nthChannel]);
					//sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
				}

			}
			numSamplesSummed++;
		}
		if(numSamplesSummed==localSamplesPerAvg) {
			//sortBPMData(sorted,sums);
			//scaleBPMAverages(sorted,localSamplesPerAvg);
			/* pass the avg'd BPM data on to the CS-interface */
			//deliverBPMAverages();
			/* zero the array of running-sums,reset counter, update num pts in avg */
			//memset(sums, 0, sizeof(double)*sizeof(sums)/sizeof(sums[0]));
			//memset(sumsSqrd, 0, sizeof(double)*sizeof(sumsSqrd)/sizeof(sumsSqrd[0]));
			numSamplesSummed=0;
			/* XXX - SamplesPerAvg can be set via orbitcontrolUI app */
			/*if(localSamplesPerAvg != samplesPerAvg) {
				syslog(LOG_INFO, "DataHandler: changing localSamplesPerAvg=%d to\n\tSamplesPerAvg=%d\n",
						localSamplesPerAvg,samplesPerAvg);
				localSamplesPerAvg = samplesPerAvg;
			}*/
		}
		/* release allocated memory */
		for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) {
			delete ds[nthAdc];
		}
	}
}
