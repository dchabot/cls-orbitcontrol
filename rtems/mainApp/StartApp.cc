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

extern "C" void setAssistedMode() {
	OrbitController *oc = OrbitController::getInstance();
	oc->setMode(ASSISTED);
}

extern "C" void setStandbyMode() {
	OrbitController *oc = OrbitController::getInstance();
	oc->setMode(STANDBY);
}

/* avoid c++ name-mangling, make this callable from "c"... (i.e. CEXP cmdline) */
extern "C" void startApp(char* epicsApp) {
	OrbitController* oc = OrbitController::getInstance();
	try {
		oc->start(rtems_task_self(),0);
		if(epicsApp != 0) {
			//parse and start the EPICS app, orbitcontrolUI:
			cexpsh(epicsApp);
		}
		/*rtems_task_wake_after(2000);
		oc->showAllBpms();
		oc->showAllOcms();*/
		rtems_event_set evOut = 0;
		rtems_event_receive(1,RTEMS_EVENT_ANY,RTEMS_NO_TIMEOUT,&evOut);
		syslog(LOG_INFO, "startApp(): Caught event !!\n");
	}
	catch(OrbitControlException& ex) {
		cout << "Caught Exception!!!" << endl;
		cout << ex.what();
	}
	catch(...) {
		syslog(LOG_INFO,"Caught an exception!!\n");
		oc->destroyInstance();
		//re-throw it up the stack; not much we can do here anyway... :-)
		throw;
	}

	if(oc) { oc->destroyInstance(); }
}
