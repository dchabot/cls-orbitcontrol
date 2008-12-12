/*
 * StartApp.c
 *
 *  Created on: Dec 9, 2008
 *      Author: chabotd
 */

#include <VmeCrate.h>
#include <Ics110blModule.h>
#include <Vmic2536Module.h>
#include <AdcReader.h>
#include <iostream>
#include <rtems.h>

using namespace std;

/* to avoid c++ name-mangling, make this callable from "c"... */
extern "C" void startApp() {

	const double adcDefaultFrameRate = 6.0; //kHz
	try {
		VmeCrate c(0/*crateID*/);
		Ics110blModule adc(c,ICS110B_DEFAULT_BASEADDR);
		Vmic2536Module dio(c,VMIC_2536_DEFAULT_BASE_ADDR);
		adc.initialize(adcDefaultFrameRate,INTERNAL_CLOCK,ICS110B_INTERNAL);
		//XXX -- interesting: cout treats uint8_t as a char. So ch/frame=" " (a space)...
		cout << adc.getType() << ": framerate=" << adc.getFrameRate()
				<< " kHz, ch/Frame = " << dec << (int)adc.getChannelsPerFrame() << endl;
		dio.initialize();
		cout << dio.getType() << ": base address=0x" << hex << dio.getVmeBaseAddr() << endl;

		AdcReader r(adc);
		r.start(0);

		rtems_task_wake_after(1000);
		throw "Some exception!!!";
	}
	catch(...) {
		cout << "Caught an exception!!" << endl;
	}
}
