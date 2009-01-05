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
		oc->start(0);
		for(int i=0; i<12; i++) {
			rtems_task_wake_after(5000);
		}
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
