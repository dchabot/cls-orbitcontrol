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
#include <Command.h>

class BpmController {
public:
	virtual ~BpmController(){}
	virtual void registerBpm(Bpm* bpm)=0;
	virtual void unregisterBpm(Bpm* bpm)=0;
	virtual Bpm* getBpmById(const string& id)=0;
	virtual void showAllBpms()=0;
	virtual uint32_t getSamplesPerAvg() const=0;
	virtual void setSamplesPerAvg(uint32_t num)=0;
	virtual void registerForBpmEvents(Command*)=0;
};


#endif /* BPMCONTROLLER_H_ */
