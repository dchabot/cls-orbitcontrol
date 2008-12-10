/*
 * StartApp.c
 *
 *  Created on: Dec 9, 2008
 *      Author: chabotd
 */

#include <VmeCrate.h>
#include <Ics110blModule.h>
#include <iostream>
#include <rtems.h>

using namespace std;

/* to avoid c++ name-mangling, make this callable from "c"... */
extern "C" void startApp() {

	const double adcDefaultFrameRate = 10.1; //kHz
	try {
		VmeCrate c(0/*crateID*/);
		Ics110blModule adc(c,ICS110B_DEFAULT_BASEADDR);
		adc.initialize(adcDefaultFrameRate,INTERNAL_CLOCK,ICS110B_INTERNAL);
		//XXX -- interesting: must add (int) cast below.
		cout << adc.getType() << ": framerate=" << adc.getFrameRate()
				<< " kHz, ch/Frame = " << (int)adc.getChannelsPerFrame() << endl;
		rtems_task_wake_after(1000);
	}
	catch(...) {
		cout << "Caught an exception!!" << endl;
	}
}
