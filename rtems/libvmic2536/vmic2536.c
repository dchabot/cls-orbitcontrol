#include "vmic2536.h"
#include <syslog.h>
#include <sis1100_api.h>

int VMIC2536_Init(VmeModule *module) {
	int status;
    int cardFD;
    uint32_t cardBase;
    uint16_t boardID = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

	status = VMIC2536_getBoardID(module, &boardID);
	if(status) {
		syslog(LOG_INFO, "VMIC-2536: crate %d, addr %#x, problem getting board ID: rc=%#x\n",
				module->crate->id, module->vmeBaseAddr, status);
		return -1;
	}
	if(boardID != VMIC_2536_BOARD_ID) {
		syslog(LOG_INFO, "VMIC-2536: crate %d, addr %#x reporting board ID as %#x\n",module->crate->id, module->vmeBaseAddr);
		return -1;
	}

	/* turn off the test mode and enable the output resgister */
	status = VMIC2536_setControl(module, VMIC_2536_INIT);
	if(status) {
		syslog(LOG_INFO, "Initialization failed at VMIC2536_setControl with error code %#x\n", status);
		return -1;
    }

	/* toggling high */
	status = VMIC2536_setOutput(module,  0xFFFFFFFF);
	if(status != 0) {
		syslog(LOG_INFO, "Initialization failed at VMIC2536_setOutput with error code 0x%x\n", status);
		return -1;
	}

	/* toggling low */
	status = VMIC2536_setOutput(module,  0x00000000);
	if(status != 0) {
		syslog(LOG_INFO, "Initialization failed at VMIC2536_setOutput with error code %#x\n", status);
		return -1;
	}

	return 0;
}

int VMIC2536_setControl(VmeModule *module, uint16_t value) {
#ifndef USE_MACRO_VME_ACCESSORS
	int cardFD;
    uint32_t cardBase;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

	return vme_A24D16_write(cardFD,cardBase + VMIC_2536_CONTROL_REG_OFFSET,value);
#else
	writeD16(module,VMIC_2536_CONTROL_REG_OFFSET,value);
	return 0;
#endif
}

/*
	int VMIC2536_getControl(int crate_id, int board_id, u_int16_t* value)
	---------------------------------------------------------------------------
	This function allows you to get the control register of a board and places
	it in the variable value.
*/
int VMIC2536_getControl(VmeModule *module, uint16_t* value) {
#ifndef USE_MACRO_VME_ACCESSORS
	int cardFD;
    uint32_t cardBase;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

    return vme_A24D16_read(cardFD,cardBase + VMIC_2536_CONTROL_REG_OFFSET,value);
#else
    *value = readD16(module, VMIC_2536_CONTROL_REG_OFFSET);
    return 0;
#endif
}

/*
	int VMIC2536_setOutput(int crate_id, int board_id, u_int32_t value)
	---------------------------------------------------------------------------
	This function allows you to set the Output register of a board with value.
	It is the resonsibility of the calling code to create this value and make
	sure that is will have the expected affect.
*/
int VMIC2536_setOutput(VmeModule *module, uint32_t value) {
#ifndef USE_MACRO_VME_ACCESSORS
	int cardFD;
    uint32_t cardBase;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

    return vme_A24D32_write(cardFD,cardBase + VMIC_2536_OUTPUT_REG_OFFSET,value);
#else
    writeD32(module, VMIC_2536_OUTPUT_REG_OFFSET, value);
    return 0;
#endif
}

/*
	int VMIC2536_getOutput(int crate_id, int board_id, u_int32_t* value)
	---------------------------------------------------------------------------
	This function allows you to get the output register of a board and places
	it in the variable value.
*/
int VMIC2536_getOutput(VmeModule *module, uint32_t* value) {
#ifndef USE_MACRO_VME_ACCESSORS
	int cardFD;
    uint32_t cardBase;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

    return vme_A24D32_read(cardFD,cardBase + VMIC_2536_OUTPUT_REG_OFFSET,value);
#else
    *value = readD32(module, VMIC_2536_OUTPUT_REG_OFFSET);
    return 0;
#endif
}

int VMIC2536_getBoardID(VmeModule *module, uint16_t* value) {
#ifndef USE_MACRO_VME_ACCESSORS
	int cardFD;
    uint32_t cardBase;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

    return vme_A24D16_read(cardFD,cardBase + VMIC_2536_BOARDID_REG_OFFSET,value);
#else
    *value = readD16(module, VMIC_2536_BOARDID_REG_OFFSET);
    return 0;
#endif
}
