/*
 * PowerSupplyBulk.cc
 *
 *  Created on: Dec 22, 2008
 *      Author: chabotd
 */

#include <PowerSupplyBulk.h>


PowerSupplyBulk::PowerSupplyBulk(Ocm* ch) : psch(ch) { }

PowerSupplyBulk::~PowerSupplyBulk() { }

void PowerSupplyBulk::activateSetpoint() const {
	psch->activateSetpoint();
}
