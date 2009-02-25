/*
 * Testing.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#include <Testing.h>
#include <cmath>

Testing* Testing::instance=0;

Testing::Testing(OrbitController* aCtlr)
	: State("Testing",TESTING),oc(aCtlr) { }

Testing* Testing::getInstance(OrbitController* aCtlr) {
	if(instance==0) {
		instance = new Testing(aCtlr);
	}
	return instance;
}

void Testing::entryAction() {
	syslog(LOG_INFO, "OrbitController: entering state %s",toString().c_str());
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	oc->startAdcAcquisition();
}

void Testing::exitAction() {
	//state exit: silence the ADC's
	oc->stopAdcAcquisition();
	oc->resetAdcFifos();
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Testing::stateAction() {
	static double sums[NumAdcModules*32];
	static double sorted[TOTAL_BPMS*2];
	static double h[NumHOcm],v[NumVOcm];

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
		//TESTING mode: assumes BPM objects have been populated with known data
		for(bit=oc->bpmMap.begin(); bit!=oc->bpmMap.end(); bit++) {
			Bpm *bpm = bit->second;
			uint32_t pos = bpm->getPosition();
			sorted[2*pos] = bpm->getX();
			sorted[2*pos+1] = bpm->getY();
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
			//h[i] *= -1.0;
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
			//v[i] *= -1.0;
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
		for(hit=oc->hOcmSet.begin(),vit=oc->vOcmSet.begin();
			hit!=oc->hOcmSet.end() && vit!=oc->vOcmSet.end();
			hit++,vit++) {
			//foreach Ocm:ocm->setSetpoint(val)
			Ocm *och = (*hit);
			if(och->isEnabled()) {
				och->setSetpoint((int32_t)h[i]+och->getSetpoint());
			}
			Ocm *ocv = (*vit);
			if(ocv->isEnabled()) {
				ocv->setSetpoint((int32_t)v[i]+ocv->getSetpoint());
			}
		}
		//distribute the UPDATE-signal to pwr-supply ctlrs
		for(uint32_t i=0; i<oc->psbArray.size(); i++) {
			oc->psbArray[i]->updateSetpoints();
		}
		hit=oc->hOcmSet.begin();
		for(uint32_t i=0; hit!=oc->hOcmSet.end(); hit++,i++) {
			Ocm *och = (*hit);
			if(och->isEnabled()) {
				syslog(LOG_INFO, "%s=%i + %.3e\n",och->getId().c_str(),
						och->getSetpoint(),h[i]);
			}
		}
		syslog(LOG_INFO, "\n\n\n");
		vit=oc->vOcmSet.begin();
		for(uint32_t i=0; vit!=oc->vOcmSet.end(); vit++,i++) {
			Ocm *ocv = (*vit);
			if(ocv->isEnabled()) {
				syslog(LOG_INFO, "%s=%i + %.3e\n",ocv->getId().c_str(),
						ocv->getSetpoint(),v[i]);
			}
		}
		syslog(LOG_INFO, "\n\n\n");
		for(bit=oc->bpmMap.begin(); bit!=oc->bpmMap.end(); bit++) {
			Bpm *bpm = bit->second;
			if(bpm->isEnabled()) {
				uint32_t pos = bpm->getPosition();
				syslog(LOG_INFO, "%s: x=%.3e\ty=%.3e\n",
						bpm->getId().c_str(),
						sorted[2*pos]+(bpm->getXRef() + bpm->getXOffs()),
						sorted[2*pos+1]+(bpm->getYRef() + bpm->getYOffs()));
			}
		}
	}
	//hand raw ADC data off to processing thread
	oc->enqueueAdcData();
}

