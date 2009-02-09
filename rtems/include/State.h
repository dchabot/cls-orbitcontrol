/*
 * State.h
 *
 *  Created on: Jan 27, 2009
 *      Author: chabotd
 */

#ifndef STATE_H_
#define STATE_H_

class State {
public:
	State();
	virtual ~State(){}
	virtual void changeState(State* s){}

protected:
	virtual void onEntry() {}
	virtual void onExit(){}
	virtual void doAction()=0;
private:
	State(const State&);
	const State& operator=(const State&);
};

#endif /* STATE_H_ */
