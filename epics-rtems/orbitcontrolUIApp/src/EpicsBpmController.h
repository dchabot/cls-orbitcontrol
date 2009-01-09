/*
 * EpicsBpmController.h
 *
 *  Created on: Jan 9, 2009
 *      Author: chabotd
 */

#ifndef EPICSBPMCONTROLLER_H_
#define EPICSBPMCONTROLLER_H_

#include <BpmController.h>

class EpicsBpmController: public BpmController {
public:
	EpicsBpmController();
	virtual ~EpicsBpmController();
	void registerBpm(Bpm* bpm);
	void unregisterBpm(Bpm* bpm);
	Bpm* getBpm(const string& id);

private:

};

#endif /* EPICSBPMCONTROLLER_H_ */
