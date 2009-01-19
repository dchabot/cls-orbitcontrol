/*
 * PowerSupplyBulk.cc
 *
 *  Created on: Jan 14, 2009
 *      Author: chabotd
 */

#include <PowerSupplyBulk.h>

void PowerSupplyBulk::updateSetpoints() const {
	/* raise the UPDATE bit */
	mod->setOutput((1<<31));
	/* wait some time */
	usecSpinDelay(delay);

	/* drop the UPDATE bit */
	mod->setOutput(0UL);
	usecSpinDelay(delay);
}
