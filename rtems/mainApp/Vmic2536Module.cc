/*
 * Vmic2536Module.cpp
 *
 *  Created on: Dec 8, 2008
 *      Author: djc
 */

#include <Vmic2536Module.h>
#include <sis1100_api.h>
#include <vmic2536.h>
#include <syslog.h>


Vmic2536Module::Vmic2536Module(VmeCrate& c, uint32_t vmeAddr) :
	//constructor-initializer list
	VmeModule(c,vmeAddr)
{
	VmeModule::setType("VMIC-2536");
}

Vmic2536Module::~Vmic2536Module() {
	// TODO Auto-generated destructor stub
}

void Vmic2536Module::initialize() const {
	uint16_t id = getId();
	if(id != VMIC_2536_BOARD_ID) {
		syslog(LOG_INFO, "VMIC-2536: crate %d, addr %#x, incorrect board ID: %#x\n",
						VmeModule::getCrate().getId(), VmeModule::getVmeBaseAddr(), id);
		throw "Problem initializing VMIC-2536!!!\n";
	}
	/* turn off the test mode and enable the output register */
	setControl(VMIC_2536_INIT);
	/* toggling high */
	//setOutput(0xFFFFFFFF);
	/* toggling low */
	//setOutput(0);

}

uint16_t Vmic2536Module::getControl() const {
#ifndef USE_MACRO_VME_ACCESSORS
	uint16_t value;
	int cardFD = VmeModule::getCrate().getFd();
    uint32_t cardBase = VmeModule::getVmeBaseAddr();

    vme_A24D16_read(cardFD,cardBase + VMIC_2536_CONTROL_REG_OFFSET,&value);
    return value;
#else
    return VmeModule::readA24D16(VMIC_2536_CONTROL_REG_OFFSET);
#endif
}

void Vmic2536Module::setControl(uint16_t val) const {
#ifndef USE_MACRO_VME_ACCESSORS
	int cardFD = VmeModule::getCrate().getFd();
    uint32_t cardBase = VmeModule::getVmeBaseAddr();

	vme_A24D16_write(cardFD,cardBase + VMIC_2536_CONTROL_REG_OFFSET,val);
#else
	VmeModule::writeA24D16(VMIC_2536_CONTROL_REG_OFFSET,val);
#endif
}

uint32_t Vmic2536Module::getOutput() const {
#ifndef USE_MACRO_VME_ACCESSORS
	uint32_t value;
	int cardFD = VmeModule::getCrate().getFd();
	uint32_t cardBase = VmeModule::getVmeBaseAddr();

	vme_A24D32_read(cardFD,cardBase + VMIC_2536_OUTPUT_REG_OFFSET,&value);
	return value;
#else
    return VmeModule::readA24D32(VMIC_2536_OUTPUT_REG_OFFSET);
#endif
}

void Vmic2536Module::setOutput(uint32_t val) const {
#ifndef USE_MACRO_VME_ACCESSORS
	int cardFD = VmeModule::getCrate().getFd();
	uint32_t cardBase = VmeModule::getVmeBaseAddr();

    vme_A24D32_write(cardFD,cardBase + VMIC_2536_OUTPUT_REG_OFFSET,val);
#else
    VmeModule::writeA24D32(VMIC_2536_OUTPUT_REG_OFFSET, val);
#endif
}

uint32_t Vmic2536Module::getInput() const {
#ifndef USE_MACRO_VME_ACCESSORS
	uint32_t value;
	int cardFD = VmeModule::getCrate().getFd();
	uint32_t cardBase = VmeModule::getVmeBaseAddr();

	vme_A24D32_read(cardFD,cardBase + VMIC_2536_INPUT_REG_OFFSET,&value);
    return value;
#else
    return VmeModule::readA24D32(VMIC_2536_INPUT_REG_OFFSET);
#endif
}

void Vmic2536Module::setInput(uint32_t val) const {
#ifndef USE_MACRO_VME_ACCESSORS
	int cardFD = VmeModule::getCrate().getFd();
	uint32_t cardBase = VmeModule::getVmeBaseAddr();

    vme_A24D32_write(cardFD,cardBase + VMIC_2536_INPUT_REG_OFFSET,val);
#else
    VmeModule::writeA24D32(VMIC_2536_INPUT_REG_OFFSET, val);
#endif
}

uint16_t Vmic2536Module::getId() const {
#ifndef USE_MACRO_VME_ACCESSORS
	uint16_t value;
	int cardFD = VmeModule::getCrate().getFd();
	uint32_t cardBase = VmeModule::getVmeBaseAddr();

	vme_A24D16_read(cardFD,cardBase + VMIC_2536_BOARDID_REG_OFFSET,&value);
    return value;
#else
    return VmeModule::readA24D16(VMIC_2536_BOARDID_REG_OFFSET);
#endif
}
