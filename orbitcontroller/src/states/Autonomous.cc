/*
 * Autonomous.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#include <Autonomous.h>
#include <cmath> //fabs(double)
#include <tscDefs.h>

Autonomous* Autonomous::instance=0;

static uint64_t now,then,tmp,numIters,start,end,period;
static double sum,sumSqrs,avg,stdDev,maxTime;
extern double tscTicksPerSecond;
static rtems_id bufId;

extern void fastAlgorithm(OrbitController*);

Autonomous::Autonomous()
	: State("Autonomous",AUTONOMOUS),oc(OrbitController::instance) { }

Autonomous* Autonomous::getInstance() {
	if(instance==0) {
		instance = new Autonomous();
	}
	return instance;
}

void Autonomous::entryAction() {
	syslog(LOG_INFO, "OrbitController: entering state %s",toString().c_str());
	oc->mode = AUTONOMOUS;
	uint32_t numFrames = HALF_FIFO_LENGTH/oc->adcArray[0]->getChannelsPerFrame();
	uint32_t bufLength = NumAdcModules*oc->adcArray[0]->getChannelsPerFrame()*numFrames*(11)*sizeof(int32_t);
	uint32_t bufSize = oc->adcArray[0]->getChannelsPerFrame()*numFrames*sizeof(int32_t);
	int32_t *buf = new int32_t[bufLength/4];
	if(buf==0) { throw OrbitControlException("Can't allocate memory for Autonomous State buffer"); }
	rtems_status_code rc = rtems_partition_create(rtems_build_name('B','U','F','2'),buf,
								bufLength,bufSize,RTEMS_LOCAL,&bufId);
	TestDirective(rc,"Autonomous State: failure creating Partition");
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	oc->startAdcAcquisition();
	oc->enableAdcInterrupts();
}

void Autonomous::exitAction() {
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
#ifdef OC_DEBUG
	stdDev = (1.0/(double)(numIters))*sumSqrs - (1.0/(double)(numIters*numIters))*(sum*sum);
	stdDev = sqrt(stdDev);
	stdDev /= tscTicksPerSecond;
	avg = sum/((double)numIters);
	avg /= tscTicksPerSecond;
	maxTime /= tscTicksPerSecond;

	syslog(LOG_INFO, "OrbitController - Autonomous Mode stats:\n\tAvg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",avg,stdDev,maxTime);
	syslog(LOG_INFO, "OrbitController - Autonomous Mode: avgFreq=%.3g Hz\n",1.0/((double)(period/numIters)/tscTicksPerSecond));
	/* zero the parameters for the next iteration...*/
	sum=sumSqrs=avg=stdDev=maxTime=0.0;
	numIters=0;
#endif
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Autonomous::stateAction() {
	static int once=1;

	end=start;
	rdtscll(start);
	if(once) { once=0; }
	else { period += start-end; }
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
	//TODO -- we're eventually going to want to incorporate Dispersion effects here
	rdtscll(then);
	fastAlgorithm(oc);
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
	//hand raw ADC data off to processing thread
	oc->enqueueAdcData();
}

