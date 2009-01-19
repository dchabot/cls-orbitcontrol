/*
 * BpmController.h
 *
 *  Created on: Jan 7, 2009
 *      Author: chabotd
 */

#ifndef BPMCONTROLLER_H_
#define BPMCONTROLLER_H_

#include <stdint.h>
#include <Bpm.h>
#include <AdcData.h>

typedef void (*BpmValueChangeCallback)(void*);

class BpmController {
public:
	virtual ~BpmController(){}
	virtual void registerBpm(Bpm* bpm)=0;
	virtual void unregisterBpm(Bpm* bpm)=0;
	virtual Bpm* getBpm(const string& id)=0;
	virtual void enqueAdcData(AdcData** rdSegments)=0;
	virtual void showAllBpms()=0;
	virtual uint32_t getSamplesPerAvg() const=0;
	virtual void setSamplesPerAvg(uint32_t num)=0;
	virtual void setBpmValueChangeCallback(BpmValueChangeCallback cb, void* cbArg)=0;
};


#endif /* BPMCONTROLLER_H_ */
