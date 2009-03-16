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

static int once=1;
static uint64_t now,then,tmp,numIters,start,end,period;
static double sum,sumSqrs,avg,stdDev,maxTime;
extern double tscTicksPerSecond;

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
	numIters=start=end=period=0;
	once=1;
#endif
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Autonomous::stateAction() {
	end=start;
	rdtscll(start);
	if(once) { once=0; }
	else { period += start-end; }
	//Wait for notification of ADC "fifo-half-full" event...
	oc->rendezvousWithIsr();
	rdtscll(then);
	oc->stopAdcAcquisition();
	oc->activateAdcReaders(HALF_FIFO_LENGTH/oc->adcArray[0]->getChannelsPerFrame());
	//Wait (block) 'til AdcReaders have completed their block-reads: ~3 ms duration for 1/2-FIFO
	oc->rendezvousWithAdcReaders();
	oc->resetAdcFifos();
	oc->startAdcAcquisition();
	oc->enableAdcInterrupts();
	/* At this point, we have approx 50 ms to "do our thing" (at 10 kHz ADC framerate)
	 * before ADC FIFOs reach their 1/2-full point and trigger another interrupt.
	 */
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

