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

AdcReader::AdcReader(Ics110blModule& mod) :
	//ctor-initializer list
	priority(OrbitControllerPriority),
	tid(0),qid(0),barrierId(0),instance(0),
	adc(mod)
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
	queueName = rtems_build_name('R','D','Q',(char)(i+48));
	rc = rtems_message_queue_create(queueName,
									1/*max queue size*/,
									sizeof(RawDataSegment)/*max msg size*/,
									RTEMS_LOCAL|RTEMS_FIFO/*attributes*/,
									&qid);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to create msg queue %d: %s\n",i,rtems_status_text(rc));
		throw "Couldn't create AdcReader queue!!!\n";
	}
	instance = i;
	i++;
}

AdcReader::~AdcReader() {
	rtems_message_queue_delete(qid);
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
	for(;;) {
		syslog(LOG_INFO, "AdcReader#%d is alive!!!\n",instance);
		rtems_task_wake_after(1000);
	}
}

void AdcReader::start(rtems_task_argument arg) {
	rtems_status_code rc;
	this->arg = arg;
	rc = rtems_task_start(tid,threadStart,(rtems_task_argument)this);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to start AdcReader %d: %s\n",instance,rtems_status_text(rc));
		throw "Couldn't start AdcReader!!!\n";
	}
	syslog(LOG_INFO, "Created ReaderThread %d with priority %d\n",instance,priority);
}

void AdcReader::read(RawDataSegment *ds) {
}

int AdcReader::getInstance() const {
	return instance;
}

rtems_id AdcReader::getThreadId() const {
	return tid;
}
