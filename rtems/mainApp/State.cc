/*
 * State.cc
 *
 *  Created on: Feb 24, 2009
 *      Author: chabotd
 */

#include <State.h>

void State::changeState(State* s) {
	if(s->getType()==type) {//transition to self
		onEntry();
		stateAction();
	}
	else {//transition to other state
		onExit();
		s->onEntry();
		s->stateAction();
	}
}

void State::onEntry() {
	if(!entered) {
		entered=true;
		entryAction();
	}
}

void State::onExit() {
	exitAction();
	entered=false;
}
