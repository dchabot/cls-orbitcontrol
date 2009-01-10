/*
 * BpmController.h
 *
 *  Created on: Jan 7, 2009
 *      Author: chabotd
 */

#ifndef BPMCONTROLLER_H_
#define BPMCONTROLLER_H_

#include <Bpm.h>
#include <AdcData.h>


class BpmController {
public:
	BpmController();
	virtual ~BpmController();
	virtual void registerBpm(Bpm* bpm)=0;
	virtual void unregisterBpm(Bpm* bpm)=0;
	virtual Bpm* getBpm(const string& id)=0;
	virtual void enqueAdcData(AdcData** rdSegments)=0;
};


#endif /* BPMCONTROLLER_H_ */
