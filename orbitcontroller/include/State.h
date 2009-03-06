/*
 * State.h
 *
 *  Created on: Jan 27, 2009
 *      Author: chabotd
 */

#ifndef STATE_H_
#define STATE_H_

#include <string>
using std::string;

/**
 * A Template (GoF) class for State-pattern implementations.
 *
 * Clients must sub-class State and implement the entryAction(),
 * stateAction(), and exitAction() protected methods. It's recommended
 * to implement these sub-classes as Singletons.
 *
 * Clients MUST NOT override the changeState(State* s) method!
 *
 * XXX -- NOTE also that client implementations of the pure virtual methods
 * MUST NOT block for significant periods !!!!!!!!!!!!!!!
 */
class State {
public:
	State(const string& aName, int aType)
		: name(aName),type(aType),entered(false) {}
	virtual ~State() {}
	string toString() const { return name; }
	int getType() const { return type; }
	void changeState(State* s);

protected:
	State();
	State(const State&);
	const State& operator=(const State&);

	void onEntry();
	void onExit();
	virtual void entryAction()=0;
	virtual void exitAction()=0;
	virtual void stateAction()=0;

	string name;
	int type;
	bool entered;
};

#endif /* STATE_H_ */
