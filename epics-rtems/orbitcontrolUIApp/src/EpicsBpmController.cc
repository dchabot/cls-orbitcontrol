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
	pair<map<string,Bpm*>::iterator,bool> ret;
	ret = bpmMap.insert(pair<string,Bpm*>(bpm->getId(),bpm));
	if(ret.second == false) {
		syslog(LOG_INFO, "Failed to insert %s in bpmMap; it already exists!\n",
							bpm->getId().c_str());
	}
}

void EpicsBpmController::unregisterBpm(Bpm *bpm) {
	map<string,Bpm*>::iterator it;
	it = bpmMap.find(bpm->getId());
	if(it != bpmMap.end()) {
		bpmMap.erase(it);
	}
	else {
		syslog(LOG_INFO, "Failed to remove %s in bpmMap; it doesn't exist!\n");
	}
}

Bpm* EpicsBpmController::getBpm(const string & id) {

	return NULL;
}

void EpicsBpmController::showAllBpms() {
	map<string,Bpm*>::iterator it;
	for(it=bpmMap.begin(); it!=bpmMap.end(); it++) {
		syslog(LOG_INFO, "%s: x=%.9g\ty=%.9g\n",it->second->getId(),
												it->second->getX(),
												it->second->getY());
	}
}

void EpicsBpmController::enqueAdcData(AdcData** data) {
	if(inpQ.send((void*)data,msgSize)) {
		//FIXME -- not much we can do here...
		inpQ.show(3);
	}
}

double getBpmScaleFactor(uint32_t numSamples) {
	const int mmPerMeter = 1000;
	// accounts for 24-bits of adc-data stuffed into the 3 MSB of a 32-bit word
	const int ShiftFactor = (1<<8);
	// accounts for the voltage-divider losses of an LPF aft of each Bergoz unit
	const double LPF_Factor = 1.015;
	// AdcPerVolt==(1<<23)/(20 Volts)
	const double AdcPerVolt = 419430.4;

	return LPF_Factor/(numSamples*ShiftFactor*AdcPerVolt*mmPerMeter);
}

/**
 * Returns BPM data in sortedArray in x,y sequences, in "ring-order"
 *
 * @param sortedArray
 * @param rawArray
 * @param adcChannelsPerFrame
 */
void sortBPMData(double *sortedArray,
					double *rawArray,
					uint32_t adcChannelsPerFrame) {
	int i,j;
	int nthAdc = 0;

	for(i=0,j=0; j<adc0ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc0ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc0ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc1ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc1ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc1ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc2ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc2ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc2ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc3ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc3ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc3ChMap[j]+1];
	}
	nthAdc = 0;
	for(j=0; j<adc4ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc4ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc4ChMap[j]+1];
	}
}

/**
 * @param sums ptr to array of doubles; storage for running sums
 * @param ds input array of AdcData ptrs
 *
 * @return the number of samples summed
 */
uint32_t sumAdcSamples(double* sums, AdcData** data) {
	uint32_t nthFrame,nthAdc,nthChannel;
	uint32_t numSamplesSummed=0;

	/* XXX -- assumes each rdSegment has an equal # of frames... */
	for(nthFrame=0; nthFrame<data[nthFrame]->getFrames(); nthFrame++) { /* for each ADC frame in rdSegment[].buf... */
		int nthFrameOffset = nthFrame*data[nthFrame]->getChannelsPerFrame();

		for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
			int nthAdcOffset = nthAdc*data[nthAdc]->getChannelsPerFrame();

			for(nthChannel=0; nthChannel<data[nthAdc]->getChannelsPerFrame(); nthChannel++) { /* for each channel of this frame... */
				sums[nthAdcOffset+nthChannel] += (double)(data[nthAdc]->getBuffer()[nthFrameOffset+nthChannel]);
				//sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
			}

		}
		numSamplesSummed++;
	}
	return numSamplesSummed;
}

//the thread-body:
void EpicsBpmController::run() {
	uint32_t localSamplesPerAvg = samplesPerAvg;
	uint32_t numSamplesSummed=0;
	static double sums[NumBpmChannels];
	static double sorted[NumBpmChannels];

	syslog(LOG_INFO, "BpmController: entering main loop...\n");
	for(;;) {
		uint32_t bytes = inpQ.receive((void*)ds,msgSize);
		if(bytes%msgSize) {
			syslog(LOG_INFO, "BpmController: received %u bytes in msg: was expecting %u\n",
								bytes,msgSize);
			//FIXME -- handle this error!!!
		}
		numSamplesSummed += sumAdcSamples(sums, ds);
		if(numSamplesSummed==localSamplesPerAvg) {
			sortBPMData(sorted,sums,ds[0]->getChannelsPerFrame());
			/* scale & update each BPM object, then execute client's BpmValueChangeCallback */
			map<string,Bpm*>::iterator it;
			int i;
			double cf = getBpmScaleFactor(numSamplesSummed);
			for(it=bpmMap.begin(),i=0; it!=bpmMap.end(); it++,i++) {
				double x = sorted[i]*cf/it->second->getXVoltsPerMilli();
				it->second->setX(x);
				double y = sorted[i+1]*cf/it->second->getYVoltsPerMilli();
				it->second->setY(y);
			}
			if(bpmCb != 0) {
				/* fire record processing */
				this->bpmCb(bpmCbArg);
			}
			/* zero the array of running-sums,reset counter, update num pts in avg */
			memset(sums, 0, sizeof(double)*NumBpmChannels);
			//memset(sumsSqrd, 0, sizeof(double)*NumBpmChannels);
			numSamplesSummed=0;
			/* XXX - SamplesPerAvg can be set via orbitcontrolUI app */
			if(localSamplesPerAvg != samplesPerAvg) {
				syslog(LOG_INFO, "BpmController: changing SamplesPerAvg=%d to %d\n",
								localSamplesPerAvg,samplesPerAvg);
				localSamplesPerAvg = samplesPerAvg;
			}
		}
		/* release allocated memory */
		for(uint32_t i=0; i<NumAdcModules; i++) {
			delete ds[i];
		}
	}
}
