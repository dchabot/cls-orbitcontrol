/*
 * OrbitController.cpp
 *
 *  Created on: Dec 11, 2008
 *      Author: chabotd
 */

#include <OrbitController.h>
#include <OrbitControlException.h>
#include <cstdio>
#include <rtems/error.h>
#include <sis1100_api.h>
#include <syslog.h>
#include <utils.h>
#include <ics110bl.h>
#include <vmic2536.h>

OrbitController* OrbitController::instance = 0;

OrbitController::OrbitController() :
	//ctor-initializer list
	tid(0),threadName(0),arg(0),
	priority(OrbitControllerPriority),
	rtemsTicksPerSecond(0),adcFramesPerTick(0),
	isrBarrierId(0),isrBarrierName(0),
	rdrBarrierId(0),rdrBarrierName(0),
	initialized(false)
{ }

OrbitController::~OrbitController() {
	stopAdcAcquisition();
	resetAdcFifos();
	if(tid) { rtems_task_delete(tid); }
	if(isrBarrierId) { rtems_barrier_delete(isrBarrierId); }
	if(rdrBarrierId) { rtems_barrier_delete(rdrBarrierId); }
	isrArray.clear();
	rdrArray.clear();
	adcArray.clear();
	crateArray.clear();
	instance = 0;
}

OrbitController* OrbitController::getInstance() {
	//FIXME -- not thread-safe!!
	if(instance==0) {
		instance = new OrbitController();
	}
	return instance;
}

void OrbitController::destroyInstance() {
	syslog(LOG_INFO, "Destroying OrbitController instance!!\n");
	delete this;
}

void OrbitController::initialize(const double adcSampleRate) {
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
	if(rc != RTEMS_SUCCESSFUL) {
		//fatal
		char msg[256];
		snprintf(msg,sizeof(msg),"OrbitController: task_create failure--%s",
									rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
	isrBarrierName = rtems_build_name('i','s','r','B');
	rc = rtems_barrier_create(isrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&isrBarrierId);
	if(rc != RTEMS_SUCCESSFUL) {
		//fatal
		char msg[256];
		snprintf(msg,sizeof(msg),"OrbitController: ISR barrier_create() failure--%s",
									rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
	rdrBarrierName = rtems_build_name('a','d','c','B');
	rc = rtems_barrier_create(rdrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&rdrBarrierId);
	if(rc != RTEMS_SUCCESSFUL) {
		//fatal
		char msg[256];
		snprintf(msg,sizeof(msg),"OrbitController: RDR barrier_create() failure--%s",
									rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);
	//initialize hardware control...
	for(uint32_t i=0; i<NumVmeCrates; i++) {
		crateArray.push_back(new VmeCrate(i));
	}
	for(uint32_t i=0; i<NumAdcModules; i++) {
		adcArray.push_back(new Ics110blModule(crateArray[i],ICS110B_DEFAULT_BASEADDR));
		adcArray[i]->initialize(adcSampleRate,INTERNAL_CLOCK,ICS110B_INTERNAL);
		syslog(LOG_INFO,"%s[%u]: framerate=%g kHz, ch/Frame = %d\n",
									adcArray[i]->getType(),i,
									adcArray[i]->getFrameRate(),
									adcArray[i]->getChannelsPerFrame());
		dioArray.push_back(new Vmic2536Module(crateArray[i],VMIC_2536_DEFAULT_BASE_ADDR));
		dioArray[i]->initialize();
	}
	for(uint32_t i=0; i<NumAdcModules; i++) {
		isrArray.push_back(new AdcIsr(adcArray[i],isrBarrierId));
		rdrArray.push_back(new AdcReader(adcArray[i], rdrBarrierId));
		rdrArray[i]->start(0);
	}
	rendezvousWithAdcReaders();
	initialized = true;
	syslog(LOG_INFO, "OrbitController: initialized and synchronized with AdcReaders...\n");
}

void OrbitController::start(rtems_task_argument arg) {
	if(initialized==false) {
		initialize(10.1);
	}
	//fire up the OrbitController thread:
	this->arg = arg;
	rtems_status_code rc = rtems_task_start(tid,threadStart,(rtems_task_argument)this);
	if(rc != RTEMS_SUCCESSFUL) {
		//fatal
		char msg[256];
		snprintf(msg,sizeof(msg),"Failed to start OrbitController thread: %s",
									rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
}

/*********************** private interface *****************************************/

rtems_task OrbitController::threadStart(rtems_task_argument arg) {
	OrbitController *oc = (OrbitController*)arg;
	oc->threadBody(oc->arg);
}

rtems_task OrbitController::threadBody(rtems_task_argument arg) {
	syslog(LOG_INFO, "Started OrbitController with priority %d\n",priority);
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	startAdcAcquisition();
	for(int j=0; j<1000; j++) {
		//Wait for notification of ADC "fifo-half-full" event...
		rendezvousWithIsr();
		stopAdcAcquisition();
		activateAdcReaders();
		//Wait (block) 'til AdcReaders have completed their block-reads:
		rendezvousWithAdcReaders();
		resetAdcFifos();
		startAdcAcquisition();
		enableAdcInterrupts();
		//FIXME -- temporary debugging!!
		for(uint32_t i=0; i<NumAdcModules; i++) {
			delete rdSegments[i];
		}
	}
	stopAdcAcquisition();
	resetAdcFifos();
	rtems_task_delete(tid);
}

void OrbitController::startAdcAcquisition() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		adcArray[i]->startAcquisition();
	}
}

void OrbitController::stopAdcAcquisition() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		adcArray[i]->stopAcquisition();
	}
}

void OrbitController::resetAdcFifos() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		adcArray[i]->resetFifo();
	}
}

void OrbitController::enableAdcInterrupts() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		int rc = vme_enable_irq_level(crateArray[i]->getFd(),adcArray[i]->getIrqLevel());
		if(rc) {
			throw OrbitControlException("OrbitController: vme_enable_irq_level() failure!!",rc);
		}
		adcArray[i]->enableInterrupt();
	}
}

void OrbitController::rendezvousWithIsr() {
	/* Wait for notification of ADC "fifo-half-full" event... */
	rtems_status_code rc = rtems_barrier_wait(isrBarrierId,5000);/*FIXME--debugging timeouts*/
	if(rc != RTEMS_SUCCESSFUL) {
		//Fatal
		char msg[256];
		snprintf(msg,sizeof(msg),"OrbitController: ISR barrier_wait() failure--%s",
									rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
}

void OrbitController::rendezvousWithAdcReaders() {
	/* block until the ReaderThreads are at their sync-point... */
	rtems_status_code rc = rtems_barrier_wait(rdrBarrierId, 5000);/*FIXME--debugging timeouts*/
	if(rc != RTEMS_SUCCESSFUL) {
		//Fatal
		char msg[256];
		snprintf(msg,sizeof(msg),"OrbitController: RDR barrier_wait() failure--%s",
									rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
}

void OrbitController::activateAdcReaders() {
	for(uint32_t i=0; i<NumAdcModules; i++) {
		rdSegments[i] = new AdcData(adcArray[i],
							HALF_FIFO_LENGTH/adcArray[i]->getChannelsPerFrame());
		//this'll unblock the associated AdcReader thread:
		rdrArray[i]->read(rdSegments[i]);
	}
}
