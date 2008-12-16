/*
 * AdcReader.cc
 *
 *  Created on: Dec 12, 2008
 *      Author: chabotd
 */

#include <AdcReader.h>
#include "OrbitController.h"
#include <rtems/error.h>
#include <syslog.h>
#include <utils.h>

AdcReader::AdcReader(Ics110blModule& mod) :
	//ctor-initializer list
	priority(OrbitControllerPriority+1),
	tid(0),barrierId(0),instance(0),
	adc(mod),ds(NULL)
{
	static int i = 0;
	rtems_status_code rc;
	/* these threads are lower priority than their master (OrbitController)... */
	threadName = rtems_build_name('R','D', 'R',(char)(i+48));
	rc = rtems_task_create(threadName,
							priority,
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&tid);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to create task %d: %s\n",i,rtems_status_text(rc));
		throw "Couldn't create AdcReader thread!!!\n";
	}
	instance = i;
	i++;
}

AdcReader::~AdcReader() {
	rtems_task_delete(tid);
	syslog(LOG_INFO, "Destroyed AdcReader %d\n",instance);
}

/* Since threadStart() is a static method, it has no "this" ptr
 * hidden in its call stack. Therefore, its an acceptable thread-entry-point for the
 * native c-function, rtems_task_start().
 *
 * WTF ?!?!... maybe some c++ guru can explain to me why threadBody() chokes (hangs sys)
 * if I use references(&) in threadStart() instead of good 'ol pointers...?
 *
 * Or, for that matter, how am I even able to call a non-static method (threadBody())
 * from within the static method threadStart() ? Shouldn't the compiler catch that ??
 *
 * Does the use of a pointer here (instead of reference) make all this magic possible ??
 */
rtems_task AdcReader::threadStart(rtems_task_argument arg) {
	AdcReader *rdr = (AdcReader*)arg;
	rdr->threadBody(rdr->arg);
}

rtems_task AdcReader::threadBody(rtems_task_argument arg) {
	rtems_status_code rc;
	uint32_t wordsRequested,wordsRead;

	syslog(LOG_INFO, "AdcReader#%d is alive!!!\n",instance);
	for(;;) {
		uint16_t adcStatus = 0;
		rtems_event_set eventsIn = 0;

		rc = rtems_barrier_wait(barrierId, 5000);
		if(TestDirective(rc, "rtems_barrier_wait()-AdcReaderThread")) {
			break;
		}
		/* block for the controller's signal... */
		rc = rtems_event_receive(RTEMS_EVENT_ANY,RTEMS_WAIT,RTEMS_NO_TIMEOUT,&eventsIn);
		if(TestDirective(rc, "AdcReader-->rtems_event_receive()")) {
			break;
		}
		/* chk for FIFO-FULL or FIFO-not-1/2-FULL conditions */
		adcStatus = adc.getStatus();
		if((adcStatus&ICS110B_FIFO_FULL) || !(adcStatus&ICS110B_FIFO_HALF_FULL)) {
			syslog(LOG_INFO, "AdcReader[%d] (pre-BLT) has abnormal status=%#hx",instance, adcStatus);
			break;
		}
		/* get the data... */
		wordsRequested = ds->numFrames*adc.getChannelsPerFrame();
		int readStatus = adc.readFifo((uint32_t *)ds->buf,wordsRequested,&wordsRead);
		/* chk for FIFO-1/2-FULL (still ?!?) or FIFO-empty conditions */
		adcStatus = adc.getStatus();
		if((adcStatus&ICS110B_FIFO_HALF_FULL) || (adcStatus&ICS110B_FIFO_EMPTY)) {
			syslog(LOG_INFO, "AdcReader[%d] (post-BLT) has abnormal status=%#hx",instance, adcStatus);
			//break;
		}
		if(readStatus) {
			syslog(LOG_INFO, "AdcReader[%d] BLT problem: status = %d", instance, readStatus);
			//FatalErrorHandler(0);
		}
		if(wordsRead != wordsRequested) {
			ds->numFrames = wordsRead/adc.getChannelsPerFrame();
			syslog(LOG_INFO, "AdcReader[%d]: asked for %u words, but read only %u\n",
					instance, wordsRequested, wordsRead);
		}
	}
	syslog(LOG_INFO, "AdcReader[%d]: deleting self", instance);
	rtems_task_delete(tid);
}

void AdcReader::start(rtems_task_argument arg) {
	rtems_status_code rc;
	this->arg = arg;
	rc = rtems_task_start(tid,threadStart,(rtems_task_argument)this);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to start AdcReader %d: %s\n",instance,rtems_status_text(rc));
		throw "Couldn't start AdcReader!!!\n";
	}
	syslog(LOG_INFO, "Started AdcReader %d with priority %d\n",instance,priority);
}

/** read() will unblock this AdcReader's thread, triggering
 * 	a VME BLT op.
 *
 * @param ds
 */
void AdcReader::read(RawDataSegment *ds) {
	this->ds = ds;
	rtems_event_send(tid,readEvent);
}

int AdcReader::getInstance() const {
	return instance;
}

rtems_id AdcReader::getThreadId() const {
	return tid;
}
