/*
 * AdcReader.cc
 *
 *  Created on: Dec 12, 2008
 *      Author: chabotd
 */
#include <stdint.h>
#include <AdcReader.h>
#include <OrbitController.h>
#include <OrbitControlException.h>
#include <cstdio>
#include <rtems/error.h>
#include <syslog.h>

AdcReader::AdcReader(Ics110blModule* mod, rtems_id bid) :
	//ctor-initializer list
	priority(OrbitControllerPriority+1),
	tid(0),barrierId(bid),instance(0),
	adc(mod),data(NULL)
{
	static int i = 0;
	/* these threads are lower priority than their master (OrbitController)... */
	threadName = rtems_build_name('R','D', 'R',(char)(i+48));
	rtems_status_code rc = rtems_task_create(threadName,
							priority,
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&tid);
	instance = i;
	if(rc != RTEMS_SUCCESSFUL) {
		//Fatal...
		char msg[256];
		snprintf(msg,sizeof(msg),"AdcReader[%d]: create_task() failure--%s",
									instance,rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
	i++;
}

AdcReader::~AdcReader() {
	rtems_task_delete(tid);
}

/* Since threadStart() is a static method, it has no "this" ptr
 * hidden in its call stack. Therefore, its an acceptable thread-entry-point for the
 * native c-function, rtems_task_start().
 */
rtems_task AdcReader::threadStart(rtems_task_argument arg) {
	AdcReader *rdr = (AdcReader*)arg;
	rdr->threadBody(rdr->arg);
}

rtems_task AdcReader::threadBody(rtems_task_argument arg) {
	uint32_t wordsRequested,wordsRead;

	for(;;) {
		uint16_t adcStatus = 0;
		rtems_event_set eventsIn = 0;

		rtems_status_code rc = rtems_barrier_wait(barrierId, barrierTimeout);
		if(rc != RTEMS_SUCCESSFUL) {
			//Fatal: bail out
			char msg[256];//("AdcReader: barrier_wait()--");
			snprintf(msg,sizeof(msg),"AdcReader[%d]: barrier_wait() failure--%s",
										instance,rtems_status_text(rc));
			throw OrbitControlException(msg);
		}
		/* block for the controller's signal... */
		rc = rtems_event_receive(readEvent,
								RTEMS_EVENT_ANY|RTEMS_WAIT,
								RTEMS_NO_TIMEOUT,
								&eventsIn);
		if(rc != RTEMS_SUCCESSFUL) {
			//also Fatal: bail...
			char msg[256];
			snprintf(msg,sizeof(msg),"AdcReader[%d]: event_receive failure()--%s",
										instance,rtems_status_text(rc));
			throw OrbitControlException(msg);
		}
#ifndef OC_DEBUG
		/* chk for FIFO-FULL or FIFO-not-1/2-FULL conditions */
		adcStatus = adc->getStatus();
		if((adcStatus&ICS110B_FIFO_FULL) || !(adcStatus&ICS110B_FIFO_HALF_FULL)) {
			//this should "never happen": Fatal if it does...
			char msg[256];
			snprintf(msg,sizeof(msg),"AdcReader[%d] (pre-BLT) has abnormal status=%#hx",instance, adcStatus);
			throw OrbitControlException(msg);
		}
#endif
		/* get the data... */
		wordsRequested = data->getFrames()*adc->getChannelsPerFrame();
		int readStatus = adc->readFifo((uint32_t *)data->getBuffer(),wordsRequested,&wordsRead);
#ifndef OC_DEBUG
		/* chk for FIFO-1/2-FULL (still ?!?) or FIFO-empty conditions */
		adcStatus = adc->getStatus();
		if((adcStatus&ICS110B_FIFO_HALF_FULL) || (adcStatus&ICS110B_FIFO_EMPTY)) {
			//again, this should "never happen": Fatal if it does...
			char msg[256];
			snprintf(msg,sizeof(msg),"AdcReader[%d] (post-BLT) has abnormal status=%#hx",instance, adcStatus);
			throw OrbitControlException(msg);
		}
#endif
		if(readStatus) {
			//probably fatal...
			char msg[256];
			snprintf(msg,sizeof(msg),"AdcReader[%d] BLT problem: status = %d", instance, readStatus);
			throw OrbitControlException(msg);
		}
		if(wordsRead != wordsRequested) {
			//also probably fatal...
			char msg[256];
			snprintf(msg,sizeof(msg),"AdcReader[%d]: asked for %u words, but read only %u",
										instance,
										(unsigned int)wordsRequested,
										(unsigned int)wordsRead);
			throw OrbitControlException(msg);
		}
	}//end for(;;)
}

void AdcReader::start(rtems_task_argument arg) {
	rtems_status_code rc;
	this->arg = arg;
	rc = rtems_task_start(tid,threadStart,(rtems_task_argument)this);
	if(rc != RTEMS_SUCCESSFUL) {
		char msg[256];
		snprintf(msg,sizeof(msg),"AdcReader[%d]: task_start() failure--%s",
									instance,rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
}

/** read() will unblock this AdcReader's thread, triggering
 * 	a VME BLT op.
 *
 * @param data
 */
void AdcReader::read(AdcData *data) {
	this->data = data;
	rtems_event_send(tid,readEvent);
}

