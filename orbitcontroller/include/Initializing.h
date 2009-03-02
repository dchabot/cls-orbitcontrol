/*
 * Initializing.h
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#ifndef INITIALIZING_H_
#define INITIALIZING_H_

#include <State.h>
#include <OrbitController.h>

class OrbitController;

class Initializing : public State {
public:
	virtual ~Initializing(){ }
	static Initializing* getInstance();

protected:
	Initializing();
	Initializing(const Initializing&);
	const Initializing& operator=(const Initializing&);

	void entryAction();
	void stateAction();
	void exitAction();

	static Initializing* instance;
	OrbitController* oc;
};

#endif /* INITIALIZING_H_ */
