/*
 * Autonomous.h
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#ifndef AUTONOMOUS_H_
#define AUTONOMOUS_H_

#include <State.h>
#include <OrbitController.h>

class OrbitController;

class Autonomous : public State {
public:
	virtual ~Autonomous() { }
	static Autonomous* getInstance(OrbitController*);

protected:
	Autonomous(OrbitController*);
	Autonomous();
	Autonomous(const Autonomous&);
	const Autonomous& operator=(const Autonomous&);

	void entryAction();
	void exitAction();
	void stateAction();

	static Autonomous* instance;
	OrbitController* oc;
};

#endif /* AUTONOMOUS_H_ */
