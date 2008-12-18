/*
 * OrbitController.cpp
 *
 *  Created on: Dec 11, 2008
 *      Author: chabotd
 */

#include "OrbitController.h"
#include <syslog.h>
#include <utils.h>
#include <ics110bl.h>

OrbitController::OrbitController() :
	//ctor-initializer list
	tid(0),threadName(0),arg(0),
	priority(OrbitControllerPriority),
	rtemsTicksPerSecond(0),adcFramesPerTick(0),
	isrBarrierId(0),isrBarrierName(0),
	rdrBarrierId(0),rdrBarrierName(0)

{
	rtems_status_code rc;
	syslog(LOG_INFO, "OrbitController: initializing...\n");
	//create thread and barriers for Rendezvous Pattern
	threadName = rtems_build_name('O','R','B','C');
	rc = rtems_task_create(threadName,
							priority,
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_FLOATING_POINT|RTEMS_LOCAL,
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&tid);
	if(TestDirective(rc, "OrbitController -- rtems_task_create()")) {
		throw "OrbitController: problem creating task!!\n";
	}
	isrBarrierName = rtems_build_name('i','s','r','B');
	rc = rtems_barrier_create(isrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&isrBarrierId);
	if(TestDirective(rc, "OrbitController -- rtems_barrier_create()-isr barrier")) {
		throw "OrbitController: problem creating isr-barrier!!\n";
	}
	rdrBarrierName = rtems_build_name('a','d','c','B');
	rc = rtems_barrier_create(rdrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&rdrBarrierId);
	if(TestDirective(rc, "OrbitController -- rtems_barrier_create()-adc barrier")) {
		throw "OrbitController: problem creating AdcReader barrier!!\n";
	}
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);
	//initialize hardware control...
	for(uint32_t i=0; i<NumVmeCrates; i++) {
		crateArray.push_back(new VmeCrate(i));
	}
	for(uint32_t i=0; i<NumAdcModules; i++) {
		adcArray.push_back(new Ics110blModule(crateArray[i],ICS110B_DEFAULT_BASEADDR));
		adcArray[i]->initialize(10.1,INTERNAL_CLOCK,ICS110B_INTERNAL);
		syslog(LOG_INFO,"%s[%u]: framerate=%g kHz, ch/Frame = %d\n",
									adcArray[i]->getType(),i,
									adcArray[i]->getFrameRate(),
									adcArray[i]->getChannelsPerFrame());
		isrArray.push_back(new AdcIsr(adcArray[i],isrBarrierId));
		rdrArray.push_back(new AdcReader(adcArray[i], rdrBarrierId));
		rdrArray[i]->start(0);
	}
	rc = rtems_barrier_wait(rdrBarrierId, 50000);/*FIXME--debugging timeouts*/
	if(TestDirective(rc,"rtems_barrier_wait-OrbitController rdr barrier")) {
		throw "OrbitController: problem waiting at rdrBarrier!!\n";
	}
	syslog(LOG_INFO, "OrbitController: synchronized with AdcReaders...\n");
}

OrbitController::~OrbitController() {
	syslog(LOG_INFO, "OrbitController dtor!!\n");
	if(tid) { rtems_task_delete(tid); }
	if(isrBarrierId) { rtems_barrier_delete(isrBarrierId); }
	if(rdrBarrierId) { rtems_barrier_delete(rdrBarrierId); }
	/*for(uint32_t i=0; i<adcArray.size(); i++) { delete adcArray[i]; }
	for(uint32_t i=0; i<isrArray.size(); i++) { delete isrArray[i]; }
	for(uint32_t i=0; i<rdrArray.size(); i++) { delete rdrArray[i]; }
	for(uint32_t i=0; i<crateArray.size(); i++) { delete crateArray[i]; }*/
}

void OrbitController::start() {
	rtems_status_code rc;
	this->arg = arg;
	rc = rtems_task_start(tid,threadStart,(rtems_task_argument)this);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to start OrbitController thread: %s\n",rtems_status_text(rc));
		throw "Couldn't start OrbitController thread!!!\n";
	}
	syslog(LOG_INFO, "Started OrbitController with priority %d\n",priority);

}

rtems_task OrbitController::threadStart(rtems_task_argument arg) {
	OrbitController *oc = (OrbitController*)arg;
	oc->threadBody(oc->arg);
}

rtems_task OrbitController::threadBody(rtems_task_argument arg) {

}

