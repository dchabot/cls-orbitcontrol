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


Autonomous::Autonomous(OrbitController* aCtlr)
	: State("Autonomous",AUTONOMOUS),oc(aCtlr) { }

Autonomous* Autonomous::getInstance(OrbitController* aCtlr) {
	if(instance==0) {
		instance = new Autonomous(aCtlr);
	}
	return instance;
}

void Autonomous::entryAction() {
	syslog(LOG_INFO, "OrbitController: entering state %s",toString().c_str());
	oc->mode = AUTONOMOUS;
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	oc->startAdcAcquisition();
}

void Autonomous::exitAction() {
	//state exit: silence the ADC's
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
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
	static double sums[NumAdcModules*32];
	static double sorted[TOTAL_BPMS*2];
	static double h[NumHOcm],v[NumVOcm];
	static int once=1;

	end=start;
	rdtscll(start);
	if(once) { once=0; }
	else { period += start-end; }
	//Wait for notification of ADC "fifo-half-full" event...
	oc->rendezvousWithIsr();
	oc->stopAdcAcquisition();
	oc->activateAdcReaders();
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
	//hand raw ADC data off to processing thread
	oc->enqueueAdcData();
}

