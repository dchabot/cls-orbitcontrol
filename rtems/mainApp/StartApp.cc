/*
 * StartApp.c
 *
 *  Created on: Dec 9, 2008
 *      Author: chabotd
 */

#include <VmeCrate.h>
#include <Ics110blModule.h>
#include <Vmic2536Module.h>
#include <DataHandler.h>
#include <AdcReader.h>
#include <AdcData.h>
#include <syslog.h>
#include <rtems.h>
#include <OrbitController.h>
#include <OrbitControlException.h>

#include <iostream>
using std::cout;
using std::endl;

/* avoid c++ name-mangling, make this callable from "c"... (i.e. CEXP cmdline) */
extern "C" void startApp() {
	OrbitController* oc = OrbitController::getInstance();
	try {
		//oc->initialize(1.0);
		oc->start(rtems_task_self());
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
		//re-throw to someone who gives a sh*t :-)
		throw;
	}

	if(oc) { oc->destroyInstance(); }
}
