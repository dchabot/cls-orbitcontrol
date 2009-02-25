/*
 * DataHandler.h
 *
 *  Created on: Dec 15, 2008
 *      Author: chabotd
 */

#ifndef DATAHANDLER_H_
#define DATAHANDLER_H_

#include <rtems.h>
#include <AdcData.h>
#include <AdcReader.h> /* for definition of RawDataSegment */
#include <syslog.h>
#include <vector>
using std::vector;

struct BPMData {
	uint32_t numElements;
	double* buf;
	BPMData(uint32_t numelem) : numElements(numelem) {
		buf = new double[numelem];//FIXME
	}
	~BPMData() {
		delete []buf;
		syslog(LOG_INFO, "~BPMAverages()\n");
	}
};

/**
 * A Singleton class, accessible via DataHandler::getInstance().
 */
class DataHandler {
public:
	static DataHandler* getInstance();
	void destroyInstance();
	void enqueRawData(AdcData* rdSegments) const;
	void start(rtems_task_argument);
	uint32_t sumBPMChannelData(double* sums, AdcData* rdSegments[]);
	void scaleBPMAverages(double* buf, uint32_t numSamples) const;
	void sortBPMData(double* sortedArray, double* rawArray) const;
	void clientRegister(rtems_id threadId);
	void clientUnregister(rtems_id threadId);
	BPMData* getBPMAverages();
	uint32_t getSamplesPerAvg() const;
	void setSamplesPerAvg(uint32_t n);

private:
	DataHandler();
	~DataHandler();
	DataHandler(const DataHandler&);
	DataHandler& operator=(const DataHandler&);

	static DataHandler *instance;
	static rtems_task threadStart(rtems_task_argument);
	rtems_task threadBody(rtems_task_argument);
	void deliverBPMAverages();
	rtems_task_argument arg;
	rtems_task_priority priority;
	rtems_id tid;
	rtems_name threadName;
	rtems_id inpQueueId;
	rtems_name inpQueueName;
	uint32_t adcChannelsPerFrame;
	AdcData** ds;
	vector<rtems_id> clients;
	static const rtems_event_set newDataEvent=1;
	double sums[108];//FIXME
	double sorted[108];//FIXME
	uint32_t samplesPerAvg;
};

/* here are the ADC channel mappings based on the current drawings*/
/* these channels may be moved around in the future */
/* structure is x,y,x,y,x,y... map positions indicate the X positions ONLY ! */
const int adc0ChMap_LENGTH=4;  /* 2404.1 */
const int adc0ChMap[adc0ChMap_LENGTH] = {9*2,10*2,11*2,12*2};

const int adc1ChMap_LENGTH=14; /* 2406.1 */
const int adc1ChMap[adc1ChMap_LENGTH] = {1*2,2*2,3*2,0*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,11*2,12*2,13*2};

const int adc2ChMap_LENGTH=14; /* 2406.3 */
const int adc2ChMap[adc2ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,6*2,7*2,8*2,9*2,5*2,10*2,11*2,12*2,13*2};

/* April 28/08 BPM1408-01 replaced with Libera unit */
/* May 1/08 BPM1410-01 replaced with Libera unit */
const int adc3ChMap_LENGTH=13; /* 2408.1 */
const int adc3ChMap[adc3ChMap_LENGTH] = {1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,11*2,12*2,13*2,14*2};

const int adc4ChMap_LENGTH=9;
const int adc4ChMap[adc4ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2};

const int TOTAL_BPMS=(adc0ChMap_LENGTH + adc1ChMap_LENGTH + adc2ChMap_LENGTH + adc3ChMap_LENGTH + adc4ChMap_LENGTH);
const int NumBpmChannels=2*TOTAL_BPMS;

#endif /* DATAHANDLER_H_ */
