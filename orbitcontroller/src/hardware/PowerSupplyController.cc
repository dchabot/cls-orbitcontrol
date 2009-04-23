/*
 * PowerSupplyController.cc
 *
 *  Created on: Apr 21, 2009
 *      Author: chabotd
 */

#include <PowerSupplyController.h>

/**
 * Set channel (ocm) to ocm->setpoint+delta
 * @param ocm
 * @param delta
 */
void PowerSupplyController::setChannel(Ocm* ocm, int32_t delta) {
	uint32_t value = 0;
	int32_t dacSetPoint = 0;

	dacSetPoint = (ocm->setpoint+delta)*DAC_AMP_CONV_FACTOR;
	ocm->setpoint = dacSetPoint;
	value = (ocm->channel & PS_CHANNEL_MASK) << PS_CHANNEL_OFFSET;
	value = value | (dacSetPoint & 0xFFFFFF);

	/* write the 32 bits out to the power supply*/
	mod->setOutput(value);
}

void PowerSupplyController::raiseLatch(Ocm* ocm) {
	uint32_t value = (ocm->channel & PS_CHANNEL_MASK) << PS_CHANNEL_OFFSET;
	value = value | (ocm->setpoint & 0xFFFFFF);
	/* raise the PS_LATCH bit */
	mod->setOutput((value | PS_LATCH));
}

void PowerSupplyController::lowerLatch(Ocm* ocm) { mod->setOutput(0UL); }

void PowerSupplyController::raiseUpdate() { mod->setOutput(UPDATE);}

void PowerSupplyController::lowerUpdate() { mod->setOutput(0UL); }

// simple bubble-sort routine. sorts to ring-order
void PowerSupplyController::sortOcm(vector<Ocm*>& ocms) {
	uint32_t len = ocms.size();

	for(uint32_t i=0; i<len; i++) {
		uint32_t minIndex = i;
		uint32_t j;

		for(j=i+1; j<len; j++) {
			if(ocms[j]->position < ocms[i]->position) {
				minIndex = j;
			}
		}
		if(minIndex != i) {
			Ocm* tmp = ocms[i];
			ocms[i] = ocms[minIndex];
			ocms[minIndex] = tmp;
		}
	}
}
