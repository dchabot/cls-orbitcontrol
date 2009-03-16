/*
 * Autonomous.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#include <Timed.h>
#include <cmath> //fabs(double)
#include <tscDefs.h>
#include <OrbitControlException.h>


static uint64_t now,then,tmp,numIters;
static double sum,sumSqrs,avg,stdDev,maxTime;
static rtems_interval periodTicks;
extern double tscTicksPerSecond;
static rtems_id timerId;
static uint32_t numFrames;

extern void fastAlgorithm(OrbitController*);

static void timerCallback(rtems_id timeId, void* arg) {
	rtems_id tid = (rtems_id)arg;

	rtems_event_send(tid,RTEMS_EVENT_1);
	rtems_timer_reset(timerId);
}

Timed* Timed::instance=0;

Timed::Timed()
	: State("Timed",TIMED),oc(OrbitController::instance) { }

Timed* Timed::getInstance() {
	if(instance==0) {
		instance = new Timed();
	}
	return instance;
}

void Timed::entryAction() {
	syslog(LOG_INFO, "OrbitController: entering state %s",toString().c_str());
	oc->mode = TIMED;
	periodTicks = 10; //FIXME -- make this value an OrbitController attribute
	numFrames = ((((uint32_t)ceil(oc->adcFrameRateFeedback))*1000)/oc->rtemsTicksPerSecond)*periodTicks;
	rtems_status_code rc = rtems_timer_create(rtems_build_name('T','I','M','R'), &timerId);
	TestDirective(rc,"OrbitController: failure creating Timed-State timer");
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	oc->disableAdcInterrupts();
	oc->resetAdcFifos();
	oc->startAdcAcquisition();
	rc = rtems_timer_fire_after(timerId,periodTicks,timerCallback,(void*)oc->ocTID);
}

void Timed::exitAction() {
	//state exit: silence the ADC's
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	rtems_timer_cancel(timerId);
	rtems_timer_delete(timerId);
#ifdef OC_DEBUG
	stdDev = (1.0/(double)(numIters))*sumSqrs - (1.0/(double)(numIters*numIters))*(sum*sum);
	stdDev = sqrt(stdDev);
	stdDev /= tscTicksPerSecond;
	avg = sum/((double)numIters);
	avg /= tscTicksPerSecond;
	maxTime /= tscTicksPerSecond;

	syslog(LOG_INFO, "OrbitController - Timed Mode stats:\n\tAvg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",avg,stdDev,maxTime);
	/* zero the parameters for the next iteration...*/
	sum=sumSqrs=avg=stdDev=maxTime=0.0;
	numIters=0;
#endif
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Timed::stateAction() {
	rtems_event_set eventsIn = 0;

	rtems_status_code rc = rtems_event_receive(RTEMS_EVENT_1,
												RTEMS_WAIT|RTEMS_EVENT_ANY,
												periodTicks+1,&eventsIn);
	TestDirective(rc,"OrbitController: failure receiving Timed-State event");
	rdtscll(then);
	oc->stopAdcAcquisition();
	oc->activateAdcReaders(numFrames);
	//Wait (block) 'til AdcReaders have completed their block-reads
	oc->rendezvousWithAdcReaders();
	oc->resetAdcFifos();
	oc->startAdcAcquisition();

	//fastAlgorithm(oc);

	//hand raw ADC data off to processing thread
	oc->enqueueAdcData();
	rdtscll(now);
#ifdef OC_DEBUG
	tmp = now-then;
	sum += (double)tmp;
	sumSqrs += (double)(tmp*tmp);
	if((double)tmp>maxTime) {
		maxTime = (double)tmp;
	}
	++numIters;
#endif
}

