/*
 * EpicsBpmController.h
 *
 *  Created on: Jan 9, 2009
 *      Author: chabotd
 */

#ifndef EPICSBPMCONTROLLER_H_
#define EPICSBPMCONTROLLER_H_

#include <stdint.h>
#include <OrbitController.h>
#include <AdcData.h>
#include <BpmController.h>
#include <epicsThread.h>
#include <epicsMessageQueue.h>
#include <string>
#include <map>
using std::string;
using std::map;
using std::iterator;
using std::pair;


typedef void (*BpmValueChangeCallback)(void*);

class EpicsBpmController: public BpmController,public epicsThreadRunable {
public:
	EpicsBpmController* getInstance();
	void destroyInstance();
	void registerBpm(Bpm* bpm);
	void unregisterBpm(Bpm* bpm);
	Bpm* getBpm(const string& id);
	void setBpmValueChangeCallback(BpmValueChangeCallback cb, void* cbArg);
	uint32_t getSamplesPerAvg() const { return samplesPerAvg; }
	void setSamplesPerAvg(uint32_t num) { samplesPerAvg = num; }
	void showAllBpms();
	void run();
	void enqueAdcData(AdcData** rdSegments);

private:
	EpicsBpmController();
	EpicsBpmController(const EpicsBpmController&);
	const EpicsBpmController& operator=(const EpicsBpmController&);
	virtual ~EpicsBpmController();

	static EpicsBpmController* instance;
	uint32_t samplesPerAvg;
	map<string,Bpm*> bpmMap;
	uint32_t priority;
	epicsThread thread;
	const uint32_t msgSize;
	const uint32_t maxMsgs;
	epicsMessageQueue inpQ;
	AdcData *ds[NumAdcModules];
	BpmValueChangeCallback bpmCb;
	void* bpmCbArg;
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
#endif /* EPICSBPMCONTROLLER_H_ */
