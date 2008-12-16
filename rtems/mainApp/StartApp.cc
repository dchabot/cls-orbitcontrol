/*
 * StartApp.c
 *
 *  Created on: Dec 9, 2008
 *      Author: chabotd
 */

#include <VmeCrate.h>
#include <Ics110blModule.h>
#include <Vmic2536Module.h>
#include "DataHandler.h"
#include <AdcReader.h>
#include <AdcDataSegment.h>
#include <syslog.h>
#include <rtems.h>

/* to avoid c++ name-mangling, make this callable from "c"... */
extern "C" void startApp() {

	const double adcDefaultFrameRate = 10.1; //kHz
	try {
		VmeCrate c(0/*crateID*/);
		Ics110blModule adc(c,ICS110B_DEFAULT_BASEADDR);
		Vmic2536Module dio(c,VMIC_2536_DEFAULT_BASE_ADDR);
		adc.initialize(adcDefaultFrameRate,INTERNAL_CLOCK,ICS110B_INTERNAL);
		//XXX -- interesting: cout treats uint8_t as a char.
		//So ch/frame=" " (a space, if #ch=32=0x20)...
		syslog(LOG_INFO,"%s: framerate=%g kHz, ch/Frame = %d\n",
							adc.getType(),
							adc.getFrameRate(),
							adc.getChannelsPerFrame());
		dio.initialize();
		syslog(LOG_INFO,"%s: base address=%#x\n",dio.getType(), dio.getVmeBaseAddr());
		rtems_task_wake_after(1000);

		DataHandler *dh = DataHandler::getInstance();

		AdcDataSegment *ads = new AdcDataSegment(adc,HALF_FIFO_LENGTH/adc.getChannelsPerFrame());
		syslog(LOG_INFO, "data segment: # of frames=%u\n",ads->getFrames());
		AdcReader r(adc);
		r.start(0);
		rtems_task_wake_after(5000);
		dh->destroyInstance();
		throw "Some exception!!!";
	}
	catch(...) {
		syslog(LOG_INFO,"Caught an exception!!\n");
	}
}
