/*
 * OrbitController.cpp
 *
 *  Created on: Dec 11, 2008
 *      Author: chabotd
 */

#include <OrbitController.h>
#include <OrbitControlException.h>
#include <rtems/error.h>
#include <sis1100_api.h>
#include <syslog.h>
#include <ics110bl.h>
#include <vmic2536.h>

/* THE singleton instance of this class */
OrbitController* OrbitController::instance = 0;

OrbitController::OrbitController() :
	//ctor-initializer list
	tid(0),threadName(0),arg(0),
	priority(OrbitControllerPriority),
	rtemsTicksPerSecond(0),adcFramesPerTick(0),
	adcFrameRateSetpoint(0),adcFrameRateFeedback(0),
	isrBarrierId(0),isrBarrierName(0),
	rdrBarrierId(0),rdrBarrierName(0),
	initialized(false),mode(ASSISTED),
	spQueueId(0),spQueueName(0)
{ }

OrbitController::~OrbitController() { }

OrbitController* OrbitController::getInstance() {
	//FIXME -- not thread-safe!!
	if(instance==0) {
		instance = new OrbitController();
	}
	return instance;
}

void OrbitController::destroyInstance() {
	syslog(LOG_INFO, "Destroying OrbitController instance!!\n");
	stopAdcAcquisition();
	resetAdcFifos();
	if(tid) { rtems_task_delete(tid); }
	if(spQueueId) { rtems_message_queue_delete(spQueueId); }
	if(isrBarrierId) { rtems_barrier_delete(isrBarrierId); }
	if(rdrBarrierId) { rtems_barrier_delete(rdrBarrierId); }
	/* XXX -- vector.clear() will NOT call item dtors if items are pointers!! */
	for(uint32_t i=0; i<isrArray.size(); i++) { delete isrArray[i]; }
	isrArray.clear();
	for(uint32_t i=0; i<rdrArray.size(); i++) { delete rdrArray[i]; }
	rdrArray.clear();
	for(uint32_t i=0; i<adcArray.size(); i++) { delete adcArray[i]; }
	adcArray.clear();
	for(uint32_t i=0; i<dioArray.size(); i++) { delete dioArray[i]; }
	dioArray.clear();
	for(uint32_t i=0; i<crateArray.size(); i++) { delete crateArray[i]; }
	crateArray.clear();
	instance = 0;
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
	TestDirective(rc,"OrbitController: task_create failure");
	isrBarrierName = rtems_build_name('i','s','r','B');
	rc = rtems_barrier_create(isrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&isrBarrierId);
	TestDirective(rc,"OrbitController: ISR barrier_create() failure");
	rdrBarrierName = rtems_build_name('a','d','c','B');
	rc = rtems_barrier_create(rdrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&rdrBarrierId);
	TestDirective(rc,"OrbitController: RDR barrier_create() failure");
	spQueueName = rtems_build_name('S','P','Q','1');
	rc = rtems_message_queue_create(spQueueName,
									48+1/*FIXME -- max msgs in queue*/,
									sizeof(SetpointMsg)/*max msg size (bytes)*/,
									RTEMS_LOCAL|RTEMS_FIFO,
									&spQueueId);
	TestDirective(rc, "OrbitController: power-supply msg_queue_create() failure");

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
	adcFrameRateSetpoint = adcSampleRate;
	adcFrameRateFeedback = adcArray[0]->getFrameRate();
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
		initialize();
	}
	//fire up the OrbitController thread:
	this->arg = arg;
	rtems_status_code rc = rtems_task_start(tid,threadStart,(rtems_task_argument)this);
	TestDirective(rc,"Failed to start OrbitController thread");
}

void OrbitController::setOcmSetpoint(Ocm* ocm, int32_t val) {
	SetpointMsg msg(ocm, val);
	rtems_status_code rc = rtems_message_queue_send(spQueueId,(void*)&msg,sizeof(msg));
	TestDirective(rc,"OrbitController: msg_q_send failure");
}

int32_t OrbitController::getOcmSetpoint(Ocm *ch) {
	return 0;
}

void OrbitController::updateAllOcmSetpoints() {

}

void OrbitController::registerOcm(Ocm* ch) {

}

void OrbitController::unregisterOcm(Ocm* ch) {

}

void OrbitController::setVerticalResponseMatrix(double v[NumOcm][NumOcm]) {

}

void OrbitController::setHorizontalResponseMatrix(double h[NumOcm][NumOcm]) {

}

/*********************** private interface *****************************************/

rtems_task OrbitController::threadStart(rtems_task_argument arg) {
	OrbitController *oc = (OrbitController*)arg;
	oc->threadBody(oc->arg);
}

rtems_task OrbitController::threadBody(rtems_task_argument arg) {
	syslog(LOG_INFO, "OrbitController: entering main processing loop\n");
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	startAdcAcquisition();
	for(int j=0; j<100; j++) {
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
	//FIXME -- temporary!!!
	rtems_event_send((rtems_id)arg,1);
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
	TestDirective(rc, "OrbitController: ISR barrier_wait() failure");
}

void OrbitController::rendezvousWithAdcReaders() {
	/* block until the ReaderThreads are at their sync-point... */
	rtems_status_code rc = rtems_barrier_wait(rdrBarrierId, 5000);/*FIXME--debugging timeouts*/
	TestDirective(rc,"OrbitController: RDR barrier_wait() failure");
}

void OrbitController::activateAdcReaders() {
	for(uint32_t i=0; i<NumAdcModules; i++) {
		rdSegments[i] = new AdcData(adcArray[i],
							HALF_FIFO_LENGTH/adcArray[i]->getChannelsPerFrame());
		//this'll unblock the associated AdcReader thread:
		rdrArray[i]->read(rdSegments[i]);
	}
}
