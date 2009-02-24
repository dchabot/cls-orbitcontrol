/*
 * Standby.cc
 *
 *  Created on: Feb 17, 2009
 *      Author: chabotd
 */

#include <Standby.h>


Standby* Standby::instance=0;

Standby::Standby(OrbitController* aCtlr)
	: State("Standby",STANDBY),oc(aCtlr) { }

Standby* Standby::getInstance(OrbitController* aCtlr) {
	if(instance==0) {
		instance = new Standby(aCtlr);
	}
	return instance;
}

void Standby::entryAction() {
	syslog(LOG_INFO, "OrbitController: entering state %s",toString().c_str());
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	oc->mode = STANDBY;
}

void Standby::exitAction() {
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Standby::stateAction() {
	rtems_task_wake_after(oc->rtemsTicksPerSecond/2);
}
