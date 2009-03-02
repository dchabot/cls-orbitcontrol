/*
 * Autonomous.h
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#ifndef TIMED_H_
#define TIMED_H_

#include <State.h>
#include <OrbitController.h>

class OrbitController;

class Timed : public State {
public:
	virtual ~Timed() { }
	static Timed* getInstance();

protected:
	Timed();
	Timed(const Timed&);
	const Timed& operator=(const Timed&);

	void entryAction();
	void exitAction();
	void stateAction();

	static Timed* instance;
	OrbitController* oc;
};

#endif /* TIMED_H_ */
