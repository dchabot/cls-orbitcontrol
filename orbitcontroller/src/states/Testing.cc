/*
 * Testing.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#include <Testing.h>
#include <cmath>

Testing* Testing::instance=0;

Testing::Testing()
	: State("Testing",TESTING),oc(OrbitController::instance) { }

Testing* Testing::getInstance() {
	if(instance==0) {
		instance = new Testing();
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
	static double sorted[TOTAL_BPMS*2];
	static double h[NumHOcm],v[NumVOcm];

	//TODO -- we're eventually going to want to incorporate Dispersion effects here
	if(oc->hResponseInitialized && oc->vResponseInitialized/* && oc->dispInitialized*/) {
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
		//FIXME -- extract oc->distributeOcmSetpoints() here and add debug logging!!!
		//distribute new OCM setpoints
		oc->distributeOcmSetpoints(h,v);
		//distribute the UPDATE-signal to pwr-supply ctlrs
		oc->updateOcmSetpoints();
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
}

