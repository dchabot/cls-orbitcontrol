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


static uint64_t now,then,tmp,numIters,start,end,period;
static double sum,sumSqrs,avg,stdDev,maxTime;
static int once=1;
extern double tscTicksPerSecond;
static rtems_id periodId;
static rtems_interval periodTicks;
static uint32_t numFrames;

extern void fastAlgorithm(OrbitController*);

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
	rtems_status_code rc = rtems_rate_monotonic_create(rtems_build_name('P','E','R','1'),&periodId);
	TestDirective(rc, "OrbitController: failure creating RMS period");
	periodTicks = 1; //FIXME -- make this value an OrbitController attribute
	numFrames = ((((uint32_t)(oc->adcFrameRateFeedback)*1000)/oc->rtemsTicksPerSecond)*periodTicks)/*-1*/;
	syslog(LOG_INFO, "OrbitController: Timed mode numFrames=%d\n",numFrames);
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	oc->disableAdcInterrupts();
	oc->resetAdcFifos();
	oc->startAdcAcquisition();
	//initiate the RMS period
	rc = rtems_rate_monotonic_period(periodId,periodTicks);
	TestDirective(rc,"OrbitController: Timed->onEntry() RMS period failure");
}

void Timed::exitAction() {
	//state exit: silence the ADC's
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	rtems_status_code rc = rtems_rate_monotonic_delete(periodId);
	TestDirective(rc,"OrbitController: failure deleting RMS period");
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
	rdtscll(then);
	//block for remainder of periodTicks and re-initialize RMS period
	rtems_status_code rc = rtems_rate_monotonic_period(periodId,periodTicks);
	TestDirective(rc,"OrbitController: failure with RMS period");
	oc->stopAdcAcquisition();
	oc->activateAdcReaders(numFrames);
	//Wait (block) 'til AdcReaders have completed their block-reads
	oc->rendezvousWithAdcReaders();

	oc->resetAdcFifos();
	oc->startAdcAcquisition();

	oc->enqueueAdcData();

	//fastAlgorithm(oc);

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

