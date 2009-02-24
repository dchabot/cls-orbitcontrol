/*
 * Assisted.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#include <Assisted.h>
#include <Ocm.h>
#include <OrbitControlException.h>



Assisted* Assisted::instance=0;

Assisted::Assisted(OrbitController* aCtlr)
	: State("Assisted", ASSISTED),oc(aCtlr) { }

Assisted* Assisted::getInstance(OrbitController* aCtlr) {
	if(instance==0) {
		instance = new Assisted(aCtlr);
	}
	return instance;
}

void Assisted::entryAction() {
	syslog(LOG_INFO, "OrbitController: entering state %s",toString().c_str());
	oc->mode = ASSISTED;
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	oc->startAdcAcquisition();
}

void Assisted::exitAction() {
	//state exit: silence the ADC's
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Assisted::stateAction() {
	//Wait for notification of ADC "fifo-half-full" event...
	oc->rendezvousWithIsr();
	oc->stopAdcAcquisition();
	oc->activateAdcReaders();
	//Wait (block) 'til AdcReaders have completed their block-reads: ~3 ms duration
	oc->rendezvousWithAdcReaders();
	oc->resetAdcFifos();
	oc->startAdcAcquisition();
	oc->enableAdcInterrupts();
	/* At this point, we have approx 50 ms to "do our thing" (at 10 kHz ADC framerate)
	 * before ADC FIFOs reach their 1/2-full point and trigger another interrupt.
	 */
	/* If we have new OCM setpoints to deliver, do it now
	 * We'll wait up to 20 ms for ALL the setpoints to enqueue
	 */
	int maxIter=4;
	uint32_t numMsgs=0;
	do {
		rtems_status_code rc = rtems_message_queue_get_number_pending(oc->spQueueId,&numMsgs);
		TestDirective(rc, "OrbitController: OCM msg_q_get_number_pending failure");
		if(numMsgs < NumOcm) { rtems_task_wake_after(5); }
		else { break; }
	} while(--maxIter);
	//deliver all the OCM setpoints we have.
	if(numMsgs > 0) {
		for(uint32_t i=0; i<numMsgs; i++) {
			OrbitController::SetpointMsg msg(NULL,0);
			size_t msgsz;
			rtems_status_code rc = rtems_message_queue_receive(oc->spQueueId,&msg,&msgsz,RTEMS_NO_WAIT,RTEMS_NO_TIMEOUT);
			TestDirective(rc,"OcmController: msq_q_rcv failure");
			msg.ocm->setSetpoint(msg.sp);
		}
		for(uint32_t i=0; i<oc->psbArray.size(); i++) {
			oc->psbArray[i]->updateSetpoints();
		}
#ifdef OC_DEBUG
		syslog(LOG_INFO, "OrbitController: updated %i OCM setpoints\n",numMsgs);
#endif
	}
	//hand raw ADC data off to processing thread
	oc->enqueueAdcData();
}
