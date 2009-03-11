/*
 * Ocm.cc
 *
 *  Created on: Dec 22, 2008
 *      Author: chabotd
 */

#include <Ocm.h>
#include <utils.h>


Ocm::Ocm(const string& str,Vmic2536Module* module,uint8_t chan) :
	//ctor-initializer list
	mod(module),channel(chan),
	id(str),enabled(true),
	setpoint(0),feedback(0),
	delay(35),//i.e. 30 usecs
	position(0)
{ }

/** setSetpoint(int sp) will change the value of a channel's setpoint,
 * 	but activateSetpoint() is then req'd to effect the change.
 *
 * @param sp new setpoint value
 */
void Ocm::setSetpoint(int32_t sp) {
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

/**
 * activateSetpoint() will toggle the UPDATE bit of a power-supply bulk,
 * causing all (current?) setpoints to take effect.
 */
void Ocm::activateSetpoint() {
	/* raise the UPDATE bit */
	mod->setOutput(UPDATE);
	/* wait some time */
	usecSpinDelay(delay);

	/* drop the UPDATE bit */
	mod->setOutput(0UL);
	usecSpinDelay(delay);
}
