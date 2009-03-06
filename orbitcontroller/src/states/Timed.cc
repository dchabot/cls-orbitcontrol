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


Timed* Timed::instance=0;

static uint64_t now,then,tmp,numIters,start,end,period;
static double sum,sumSqrs,avg,stdDev,maxTime;
static int once=1;
extern double tscTicksPerSecond;
static rtems_id periodId;
static rtems_interval periodTicks;
static uint32_t numFrames;
static rtems_rate_monotonic_period_statistics rmsStats;
static rtems_id bufId;

extern void fastAlgorithm(OrbitController*);

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
	periodTicks = 10; //FIXME -- make this value an OrbitController attribute
	numFrames = ((((uint32_t)ceil(oc->adcFrameRateFeedback))*1000)/oc->rtemsTicksPerSecond)*periodTicks;
	syslog(LOG_INFO, "OrbitController: Timed State using %u ADC frames per RMS period\n",numFrames);

	uint32_t bufLength = NumAdcModules*oc->adcArray[0]->getChannelsPerFrame()*numFrames*(11)*sizeof(int32_t);
	uint32_t bufSize = oc->adcArray[0]->getChannelsPerFrame()*numFrames*sizeof(int32_t);
	int32_t *buf = new int32_t[bufLength/4];
	if(buf==0) { throw OrbitControlException("Can't allocate memory for Timed State buffer"); }
	rc = rtems_partition_create(rtems_build_name('B','U','F','3'),buf,
								bufLength,bufSize,RTEMS_LOCAL,&bufId);
	TestDirective(rc,"Timed State: failure creating Partition");
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	oc->disableAdcInterrupts();
	oc->resetAdcFifos();
	oc->startAdcAcquisition();
	//initiate the RMS period
	rc = rtems_rate_monotonic_period(periodId,periodTicks);
	TestDirective(rc,"OrbitController: Timed->onEntry() RMS period failure");
	//rtems_task_wake_after(periodTicks);
}

void Timed::exitAction() {
	//state exit: silence the ADC's
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	rtems_status_code rc = rtems_rate_monotonic_delete(periodId);
	TestDirective(rc,"OrbitController: failure deleting RMS period");
	//nuke our ADC buffer Partition
	if(rtems_partition_delete(bufId) == RTEMS_RESOURCE_IN_USE) {
		while(rtems_partition_delete(bufId)==RTEMS_RESOURCE_IN_USE) {
			rtems_task_wake_after(10);
		}
	}
#ifdef OC_DEBUG
	stdDev = (1.0/(double)(numIters))*sumSqrs - (1.0/(double)(numIters*numIters))*(sum*sum);
	stdDev = sqrt(stdDev);
	stdDev /= tscTicksPerSecond;
	avg = sum/((double)numIters);
	avg /= tscTicksPerSecond;
	maxTime /= tscTicksPerSecond;

	syslog(LOG_INFO, "OrbitController - Timed Mode stats:\n\tAvg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",avg,stdDev,maxTime);
	syslog(LOG_INFO, "OrbitController - Timed Mode: avgFreq=%.3g Hz\n",1.0/((double)(period/numIters)/tscTicksPerSecond));
	/* zero the parameters for the next iteration...*/
	sum=sumSqrs=avg=stdDev=maxTime=0.0;
	numIters=period=0;
	once=1;
	start=end=0;
#endif
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Timed::stateAction() {
	end=start;
	rdtscll(start);
	if(once) { once=0; }
	else { period += start-end; }

	//block for remainder of periodTicks and re-initialize RMS period
	rtems_status_code rc = rtems_rate_monotonic_period(periodId,periodTicks);
	TestDirective(rc,"OrbitController: failure with RMS period");
	//rtems_interval _then = rtems_clock_get_ticks_since_boot();

	oc->stopAdcAcquisition();
	rdtscll(then);
	oc->activateAdcReaders(bufId,numFrames);
	rdtscll(now);
	//Wait (block) 'til AdcReaders have completed their block-reads
	oc->rendezvousWithAdcReaders();

	oc->resetAdcFifos();
	oc->startAdcAcquisition();

	fastAlgorithm(oc);

#ifdef OC_DEBUG
	tmp = now-then;
	sum += (double)tmp;
	sumSqrs += (double)(tmp*tmp);
	if((double)tmp>maxTime) {
		maxTime = (double)tmp;
	}
	++numIters;
#endif
	//hand raw ADC data off to processing thread
	oc->enqueueAdcData();
	//rtems_interval _now = rtems_clock_get_ticks_since_boot();
	//rtems_task_wake_after(periodTicks-(_now-_then));
}

