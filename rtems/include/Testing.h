/*
 * Testing.h
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#ifndef TESTING_H_
#define TESTING_H_

#include <State.h>
#include <OrbitController.h>

class OrbitController;

class Testing : public State {
public:
	virtual ~Testing() { }
	static Testing* getInstance(OrbitController*);

protected:
	Testing(OrbitController*);
	Testing();
	Testing(const Testing&);
	const Testing& operator=(const Testing&);

	void entryAction();
	void exitAction();
	void stateAction();

	static Testing* instance;
	OrbitController* oc;
};

#endif /* TESTING_H_ */
