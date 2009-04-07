/*
 * FastAlgorithm.cc
 *
 *  Created on: Mar 4, 2009
 *      Author: chabotd
 */
#include <OrbitController.h>
#include <cmath>

void fastAlgorithm(double* sums, OrbitController* oc) {
	static double sorted[TOTAL_BPMS*2];
	static double h[NumHOcm],v[NumVOcm];

	if (oc->hResponseInitialized && oc->vResponseInitialized) {
		memset(sorted, 0, sizeof(sorted));
		memset(h, 0, sizeof(h));
		memset(v, 0, sizeof(v));
		/* Calc and deliver new OCM setpoints:
		 * NOTE: testing shows calc takes ~1ms
		 * while OCM setpoint delivery req's ~5ms
		 * for 48 OCMs (scales linearly with # OCM)
		 */
		map<string, Bpm*>::iterator bit;
		uint32_t numSamples = oc->sumAdcSamples(sums, oc->rdSegments);
		oc->sortBPMData(sorted, sums, oc->rdSegments[0]->getChannelsPerFrame());
		double sf = oc->getBpmScaleFactor(numSamples);
		for (bit=oc->bpmMap.begin(); bit!=oc->bpmMap.end(); bit++) {
			Bpm *bpm = bit->second;
			uint32_t pos = bpm->getPosition();
			sorted[2*pos ] *= sf / bpm->getXVoltsPerMilli();
			sorted[2*pos+1] *= sf / bpm->getYVoltsPerMilli();
		}

		//First, calc BPM deltas
		for (bit=oc->bpmMap.begin(); bit!=oc->bpmMap.end(); bit++) {
			Bpm *bpm = bit->second;
			if (bpm->isEnabled()) {
				//subtract reference and DC orbit-components
				uint32_t pos = bpm->getPosition();
				sorted[2*pos ] -= (bpm->getXRef() + bpm->getXOffs());
				sorted[2*pos+1] -= (bpm->getYRef() + bpm->getYOffs());
			}
		}
		oc->lock();
		for (uint32_t i=0; i<NumHOcm; i++) {
			bit = oc->bpmMap.begin();
			for (uint32_t j=0; j<NumBpm && bit!=oc->bpmMap.end(); bit++) {
				Bpm *bpm = bit->second;
				if (bpm->isEnabled()) {
					uint32_t pos = bpm->getPosition();
					h[i] += oc->hmat[i][j] * sorted[2*pos];
					//syslog(LOG_INFO, "h[%i] += %.3e X %.3e = %.3e\n",i,hmat[i][j],sorted[2*pos],h[i]);
					++j;
				}
			}
			h[i] *= -1.0;
		}
		//calc vertical OCM setpoints
		for (uint32_t i=0; i<NumVOcm; i++) {
			bit = oc->bpmMap.begin();
			for (uint32_t j=0; j<NumBpm && bit!=oc->bpmMap.end(); bit++) {
				Bpm *bpm = bit->second;
				if (bpm->isEnabled()) {
					uint32_t pos = bpm->getPosition();
					v[i] += oc->vmat[i][j] * sorted[2*pos+1];
					//syslog(LOG_INFO, "v[%i] += %.3e X %.3e = %.3e\n",i,vmat[i][j],sorted[2*pos+1],v[i]);
					++j;
				}
			}
			v[i] *= -1.0;
		}
		//scale OCM setpoints (max step && %-age to apply)
		double max = 0;
		for (uint32_t i=0; i<NumHOcm; i++) {
			double habs = fabs(h[i]);
			if (habs > max) {
				max = habs;
			}
		}
		if (max > (double)oc->maxHStep) {
			double scaleFactor = ((double)oc->maxHStep)/max;
			for (uint32_t i=0; i<NumHOcm; i++) {
				h[i] *= scaleFactor;
			}
		}
		for (uint32_t i=0; i<NumHOcm; i++) {
			h[i] *= oc->maxHFrac;
		}
		max = 0;
		for (uint32_t i=0; i<NumVOcm; i++) {
			double vabs = fabs(v[i]);
			if (vabs > max) {
				max = vabs;
			}
		}
		if (max > (double)oc->maxVStep) {
			double scaleFactor = ((double)oc->maxVStep)/max;
			for (uint32_t i=0; i<NumVOcm; i++) {
				v[i] *= scaleFactor;
			}
		}
		for (uint32_t i=0; i<NumVOcm; i++) {
			v[i] *= oc->maxVFrac;
		}
		oc->unlock();
		//distribute new OCM setpoints
		set<Ocm*>::iterator hit, vit;
		uint32_t i = 0;
		for (hit=oc->hOcmSet.begin(); hit!=oc->hOcmSet.end(); hit++, i++) {
			Ocm *och = (*hit);
			if (och->isEnabled()) {
				och->setSetpoint((int32_t)h[i] + och->getSetpoint());
			}
		}
		i = 0;
		for (vit=oc->vOcmSet.begin(); vit!=oc->vOcmSet.end(); vit++, i++) {
			Ocm *ocv = (*vit);
			if (ocv->isEnabled()) {
				ocv->setSetpoint((int32_t)v[i] + ocv->getSetpoint());
			}
		}
		//distribute the UPDATE-signal to pwr-supply ctlrs
		for (i=0; i<oc->psbArray.size(); i++) {
			oc->psbArray[i]->updateSetpoints();
		}
	}
}
