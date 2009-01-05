/*
 * PowerSupplyChannel.cc
 *
 *  Created on: Dec 22, 2008
 *      Author: chabotd
 */

#include <PowerSupplyChannel.h>
#include <utils.h>

PowerSupplyChannel::PowerSupplyChannel(const char* pvId,
										Vmic2536Module* module,
										uint8_t chan) :
	//ctor-initializer list
	mod(module),channel(chan),
	id(pvId),inCorrection(true),
	setpoint(0),feedback(0),delay(30)//i.e. 30 usecs
{ }

PowerSupplyChannel::~PowerSupplyChannel() { }

void PowerSupplyChannel::setSetpoint(int32_t sp) {
	uint32_t value = 0;
	int32_t dacSetPoint = 0;

	dacSetPoint = sp * DAC_AMP_CONV_FACTOR;
	setpoint = dacSetPoint;
	value = (channel & PS_CHANNEL_MASK) << PS_CHANNEL_OFFSET;
	value = value | (dacSetPoint & 0xFFFFFF);

	/* write the 32 bits out to the power supply*/
	mod->setOutput(value);
	/* added for Milan G IE Power 04/08/2002*/
	usecSpinDelay(delay);

	/* toggle the PS_LATCH bit */
	mod->setOutput((value | PS_LATCH));
	usecSpinDelay(delay);

	/* drop the PS_LATCH bit and data bits */
	mod->setOutput(0UL);
	usecSpinDelay(delay);
}

void PowerSupplyChannel::activateSetpoint() {
	/* raise the UPDATE bit */
	mod->setOutput(UPDATE);
	/* wait some time */
	usecSpinDelay(delay);

	/* drop the UPDATE bit */
	mod->setOutput(0UL);
	usecSpinDelay(delay);
}
