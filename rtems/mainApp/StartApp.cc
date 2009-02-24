/*
 * StartApp.c
 *
 *  Created on: Dec 9, 2008
 *      Author: chabotd
 */

#include <syslog.h>
#include <rtems.h>
#include <OrbitController.h>
#include <OrbitControlException.h>
#include <cexp.h>

#include <iostream>
using std::cout;
using std::endl;


extern "C" void setAutonomousMode() {
	OrbitController *oc = OrbitController::getInstance();
	oc->setMode(AUTONOMOUS);
}

extern "C" void setAssistedMode() {
	OrbitController *oc = OrbitController::getInstance();
	oc->setMode(ASSISTED);
}

extern "C" void setStandbyMode() {
	OrbitController *oc = OrbitController::getInstance();
	oc->setMode(STANDBY);
}

extern "C" void showAllBpms() {
	OrbitController *oc = OrbitController::getInstance();
	oc->showAllBpms();
}

extern "C" void showAllOcms() {
	OrbitController *oc = OrbitController::getInstance();
	oc->showAllOcms();
}

/* avoid c++ name-mangling, make this callable from "c"... (i.e. CEXP cmdline) */
extern "C" void startApp(char* epicsApp) {
	OrbitController* oc = OrbitController::getInstance();
	try {
		//oc->initialize();
		if(epicsApp != 0) {
			//parse and start the EPICS app, orbitcontrolUI:
			cexpsh(epicsApp);
		}
		oc->start(rtems_task_self(),0);
	}
	catch(OrbitControlException& ex) {
		cout << "Caught Exception!!!" << endl;
		cout << ex.what();
		oc->destroyInstance();
	}
	catch(...) {
		syslog(LOG_INFO,"Caught an exception!!\n");
		oc->destroyInstance();
		//re-throw it up the stack; not much we can do here anyway... :-)
		throw;
	}
}
