/*
 * Vmic2536Module.cpp
 *
 *  Created on: Dec 8, 2008
 *      Author: djc
 */

#include <Vmic2536Module.h>
#include <OrbitControlException.h>
#include <sis1100_api.h>
#include <syslog.h>


Vmic2536Module::Vmic2536Module(VmeCrate* c, uint32_t vmeAddr) :
	//constructor-initializer list
	VmeModule(c,vmeAddr)
{
	type = "VMIC-2536";
	initialized = false;
}

Vmic2536Module::~Vmic2536Module() {
	syslog(LOG_INFO, "Vmic2536Module dtor!!\n");
}

void Vmic2536Module::initialize() {
	uint16_t id = getId();
	if(id != VMIC_2536_BOARD_ID) {
		syslog(LOG_INFO, "VMIC-2536: crate %d, addr %#x, incorrect board ID: %#x\n",
						crate->getId(), vmeBaseAddr, id);
		throw OrbitControlException("Problem initializing VMIC-2536!!!");
	}
	/* turn off the test mode and enable the output register */
	setControl(VMIC_2536_INIT);
	/* toggling high */
	//setOutput(0xFFFFFFFF);
	/* toggling low */
	//setOutput(0);

	initialized = true;
}

uint16_t Vmic2536Module::getControl() const {
#ifndef VMIC2536MODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint16_t value;
	int cardFD = crate->getFd();
    uint32_t cardBase = vmeBaseAddr;

    vme_A24D16_read(cardFD,cardBase + VMIC_2536_CONTROL_REG_OFFSET,&value);
    return value;
#else
    return readA24D16(VMIC_2536_CONTROL_REG_OFFSET);
#endif
}

void Vmic2536Module::setControl(uint16_t val) const {
#ifndef VMIC2536MODULE_USE_SINGLE_CYCLE_VME_ACCESS
	int cardFD = crate->getFd();
    uint32_t cardBase = vmeBaseAddr;

	vme_A24D16_write(cardFD,cardBase + VMIC_2536_CONTROL_REG_OFFSET,val);
#else
	writeA24D16(VMIC_2536_CONTROL_REG_OFFSET,val);
#endif
}

uint32_t Vmic2536Module::getOutput() const {
#ifndef VMIC2536MODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t value;
	int cardFD = crate->getFd();
	uint32_t cardBase = vmeBaseAddr;

	vme_A24D32_read(cardFD,cardBase + VMIC_2536_OUTPUT_REG_OFFSET,&value);
	return value;
#else
    return readA24D32(VMIC_2536_OUTPUT_REG_OFFSET);
#endif
}

void Vmic2536Module::setOutput(uint32_t val) const {
#ifndef VMIC2536MODULE_USE_SINGLE_CYCLE_VME_ACCESS
	int cardFD = crate->getFd();
	uint32_t cardBase = vmeBaseAddr;

    vme_A24D32_write(cardFD,cardBase + VMIC_2536_OUTPUT_REG_OFFSET,val);
#else
    writeA24D32(VMIC_2536_OUTPUT_REG_OFFSET, val);
#endif
}

uint32_t Vmic2536Module::getInput() const {
#ifndef VMIC2536MODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t value;
	int cardFD = crate->getFd();
	uint32_t cardBase = vmeBaseAddr;

	vme_A24D32_read(cardFD,cardBase + VMIC_2536_INPUT_REG_OFFSET,&value);
    return value;
#else
    return readA24D32(VMIC_2536_INPUT_REG_OFFSET);
#endif
}

void Vmic2536Module::setInput(uint32_t val) const {
#ifndef VMIC2536MODULE_USE_SINGLE_CYCLE_VME_ACCESS
	int cardFD = crate->getFd();
	uint32_t cardBase = vmeBaseAddr;

    vme_A24D32_write(cardFD,cardBase + VMIC_2536_INPUT_REG_OFFSET,val);
#else
    writeA24D32(VMIC_2536_INPUT_REG_OFFSET, val);
#endif
}

uint16_t Vmic2536Module::getId() const {
#ifndef VMIC2536MODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint16_t value;
	int cardFD = crate->getFd();
	uint32_t cardBase = vmeBaseAddr;

	vme_A24D16_read(cardFD,cardBase + VMIC_2536_BOARDID_REG_OFFSET,&value);
    return value;
#else
    return readA24D16(VMIC_2536_BOARDID_REG_OFFSET);
#endif
}
