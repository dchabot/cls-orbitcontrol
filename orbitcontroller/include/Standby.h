/*
 * Standby.h
 *
 *  Created on: Feb 17, 2009
 *      Author: chabotd
 */

#ifndef STANDBY_H_
#define STANDBY_H_

#include <State.h>
#include <OrbitController.h>

class OrbitController;

class Standby : public State {
public:
	virtual ~Standby(){}
	static Standby* getInstance();

protected:
	Standby();
	Standby(const Standby&);
	const Standby& operator=(const Standby&);

	void entryAction();
	void stateAction();
	void exitAction();

	static Standby* instance;
	OrbitController* oc;
};

#endif /* STANDBY_H_ */
