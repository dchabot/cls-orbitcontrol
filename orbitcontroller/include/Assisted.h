/*
 * Assisted.h
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#ifndef ASSISTED_H_
#define ASSISTED_H_

#include <State.h>
#include <OrbitController.h>

class OrbitController;

class Assisted : public State {
public:
	virtual ~Assisted() { }
	static Assisted* getInstance();

protected:
	Assisted();
	Assisted(const Assisted&);
	const Assisted& operator=(const Assisted&);

	void entryAction();
	void exitAction();
	void stateAction();

	static Assisted* instance;
	OrbitController* oc;
};

#endif /* ASSISTED_H_ */
