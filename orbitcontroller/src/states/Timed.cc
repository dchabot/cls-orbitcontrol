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
extern double tscTicksPerSecond;
static rtems_id periodId;
static rtems_interval periodTicks;
static uint32_t numFrames;
static rtems_rate_monotonic_period_statistics rmsStats;


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
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	//initiate the RMS period
	rc = rtems_rate_monotonic_period(periodId,periodTicks);
	TestDirective(rc,"OrbitController: Timed->onEntry() RMS period failure");
	oc->disableAdcInterrupts();
	oc->startAdcAcquisition();
}

void Timed::exitAction() {
	rtems_status_code rc = rtems_rate_monotonic_cancel(periodId);
	TestDirective(rc, "OrbitController: failure cancelling RMS period");
	//state exit: silence the ADC's
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	rc = rtems_rate_monotonic_delete(periodId);
	TestDirective(rc,"OrbitController: failure deleting RMS period");
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
	numIters=0;
#endif
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Timed::stateAction() {
	static double sums[NumAdcModules*32];
	static double sorted[TOTAL_BPMS*2];
	static double h[NumHOcm],v[NumVOcm];

	//block for periodTicks and re-initialize RMS period
	rtems_status_code rc = rtems_rate_monotonic_period(periodId,periodTicks);
	TestDirective(rc,"OrbitController: failure with RMS period");
	oc->activateAdcReaders(numFrames);
	//Wait (block) 'til AdcReaders have completed their block-reads
	oc->rendezvousWithAdcReaders();
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	oc->startAdcAcquisition();
	//TODO -- we're eventually going to want to incorporate Dispersion effects here
	if(oc->hResponseInitialized && oc->vResponseInitialized/* && oc->dispInitialized*/) {
		memset(sums,0,sizeof(sums));
		memset(sorted,0,sizeof(sorted));
		memset(h,0,sizeof(h));
		memset(v,0,sizeof(v));
		/* Calc and deliver new OCM setpoints:
		 * NOTE: testing shows calc takes ~1ms
		 * while OCM setpoint delivery req's ~5ms
		 * for 48 OCMs (scales linearly with # OCM)
		 */
		map<string,Bpm*>::iterator bit;
		uint32_t numSamples = oc->sumAdcSamples(sums,oc->rdSegments);
		oc->sortBPMData(sorted,sums,oc->rdSegments[0]->getChannelsPerFrame());
		double sf = oc->getBpmScaleFactor(numSamples);
		for(bit=oc->bpmMap.begin(); bit!=oc->bpmMap.end(); bit++) {
			Bpm *bpm = bit->second;
			uint32_t pos = bpm->getPosition();
			sorted[2*pos] *= sf/bpm->getXVoltsPerMilli();
			sorted[2*pos+1] *= sf/bpm->getYVoltsPerMilli();
		}

		//FIXME -- refactor calcs to private methods
		//TODO: calc dispersion effect
		//First, calc BPM deltas
		for(bit=oc->bpmMap.begin(); bit!=oc->bpmMap.end(); bit++) {
			Bpm *bpm = bit->second;
			if(bpm->isEnabled()) {
				//subtract reference and DC orbit-components
				uint32_t pos = bpm->getPosition();
				sorted[2*pos] -= (bpm->getXRef() + bpm->getXOffs());
				sorted[2*pos+1] -= (bpm->getYRef() + bpm->getYOffs());
			}
		}
		oc->lock();
		for(uint32_t i=0; i<NumHOcm; i++) {
			bit=oc->bpmMap.begin();
			for(uint32_t j=0; j<NumBpm && bit!=oc->bpmMap.end(); bit++) {
				Bpm *bpm = bit->second;
				if(bpm->isEnabled()) {
					uint32_t pos = bpm->getPosition();
					h[i] += oc->hmat[i][j]*sorted[2*pos];
					//syslog(LOG_INFO, "h[%i] += %.3e X %.3e = %.3e\n",i,hmat[i][j],sorted[2*pos],h[i]);
					++j;
				}
			}
			h[i] *= -1.0;
		}
		//calc vertical OCM setpoints
		for(uint32_t i=0; i<NumVOcm; i++) {
			bit=oc->bpmMap.begin();
			for(uint32_t j=0; j<NumBpm && bit!=oc->bpmMap.end(); bit++) {
				Bpm *bpm = bit->second;
				if(bpm->isEnabled()) {
					uint32_t pos = bpm->getPosition();
					v[i] += oc->vmat[i][j]*sorted[2*pos+1];
					//syslog(LOG_INFO, "v[%i] += %.3e X %.3e = %.3e\n",i,vmat[i][j],sorted[2*pos+1],v[i]);
					++j;
				}
			}
			v[i] *= -1.0;
		}
		//scale OCM setpoints (max step && %-age to apply)
		double max=0;
		for(uint32_t i=0; i<NumHOcm; i++) {
			double habs = fabs(h[i]);
			if(habs > max) { max = habs; }
		}
		if(max > (double)oc->maxHStep) {
			double scaleFactor = ((double)oc->maxHStep)/max;
			for(uint32_t i=0; i<NumHOcm; i++) {
				h[i] *= scaleFactor;
			}
		}
		for(uint32_t i=0; i<NumHOcm; i++) {
			h[i] *= oc->maxHFrac;
		}
		max=0;
		for(uint32_t i=0; i<NumVOcm; i++) {
			double vabs = fabs(v[i]);
			if(vabs > max) { max = vabs; }
		}
		if(max > (double)oc->maxVStep) {
			double scaleFactor = ((double)oc->maxVStep)/max;
			for(uint32_t i=0; i<NumVOcm; i++) {
				v[i] *= scaleFactor;
			}
		}
		for(uint32_t i=0; i<NumVOcm; i++) {
			v[i] *= oc->maxVFrac;
		}
		oc->unlock();
		//distribute new OCM setpoints
		set<Ocm*>::iterator hit,vit;
		uint32_t i = 0;
		for(hit=oc->hOcmSet.begin(); hit!=oc->hOcmSet.end(); hit++) {
			Ocm *och = (*hit);
			if(och->isEnabled()) {
				och->setSetpoint((int32_t)h[i]+och->getSetpoint());
				i++;
			}
		}
		i=0;
		for(vit=oc->vOcmSet.begin(); vit!=oc->vOcmSet.end(); vit++) {
			Ocm *ocv = (*vit);
			if(ocv->isEnabled()) {
				ocv->setSetpoint((int32_t)v[i]+ocv->getSetpoint());
				i++;
			}
		}
		//distribute the UPDATE-signal to pwr-supply ctlrs
		for(uint32_t i=0; i<oc->psbArray.size(); i++) {
			oc->psbArray[i]->updateSetpoints();
		}
#if 0
		tmp = now-then;
		sum += (double)tmp;
		sumSqrs += (double)(tmp*tmp);
		if((double)tmp>maxTime) {
			maxTime = (double)tmp;
		}
		++numIters;
#endif
	}
	//hand raw ADC data off to processing thread
	oc->enqueueAdcData();
}

