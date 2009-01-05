/*
 * PowerSupplyBulk.cc
 *
 *  Created on: Dec 22, 2008
 *      Author: chabotd
 */

#include <PowerSupplyBulk.h>
#include <PowerSupplyChannel.h>
#include <utils.h>


PowerSupplyBulk::PowerSupplyBulk(PowerSupplyChannel* ch) : psch(ch) { }

PowerSupplyBulk::~PowerSupplyBulk() { }

void PowerSupplyBulk::activateSetpoint() const {
	psch->activateSetpoint();
}
