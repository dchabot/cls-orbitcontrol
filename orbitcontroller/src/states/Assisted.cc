/*
 * Assisted.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#include <Assisted.h>
#include <Ocm.h>
#include <OrbitControlException.h>
#include <ics110bl.h>


Assisted* Assisted::instance=0;
static rtems_id bufId;


Assisted::Assisted()
	: State("Assisted",ASSISTED),oc(OrbitController::instance) { }

Assisted* Assisted::getInstance() {
	if(instance==0) {
		instance = new Assisted();
	}
	return instance;
}

void Assisted::entryAction() {
	syslog(LOG_INFO, "OrbitController: entering state %s",toString().c_str());
	oc->mode = ASSISTED;
	uint32_t numFrames = HALF_FIFO_LENGTH/oc->adcArray[0]->getChannelsPerFrame();
	uint32_t bufLength = NumAdcModules*oc->adcArray[0]->getChannelsPerFrame()*numFrames*(11)*sizeof(int32_t);
	uint32_t bufSize = oc->adcArray[0]->getChannelsPerFrame()*numFrames*sizeof(int32_t);
	int32_t *buf = new int32_t[bufLength/4];
	if(buf==0) { throw OrbitControlException("Can't allocate memory for Assisted State buffer"); }
	rtems_status_code rc = rtems_partition_create(rtems_build_name('B','U','F','1'),buf,
								bufLength,bufSize,RTEMS_LOCAL,&bufId);
	TestDirective(rc,"Assisted State: failure creating Partition");
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	oc->startAdcAcquisition();
	oc->enableAdcInterrupts();
}

void Assisted::exitAction() {
	//state exit: silence the ADC's
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	oc->disableAdcInterrupts();
	//nuke our ADC buffer Partition
	if(rtems_partition_delete(bufId) == RTEMS_RESOURCE_IN_USE) {
		while(rtems_partition_delete(bufId)==RTEMS_RESOURCE_IN_USE) {
			rtems_task_wake_after(10);
		}
	}
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Assisted::stateAction() {
	//Wait for notification of ADC "fifo-half-full" event...
	oc->rendezvousWithIsr();
	oc->stopAdcAcquisition();
	oc->activateAdcReaders(bufId,HALF_FIFO_LENGTH/oc->adcArray[0]->getChannelsPerFrame());
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
