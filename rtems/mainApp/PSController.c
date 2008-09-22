/*
 * PSController.c
 *
 *  Created on: Sep 21, 2008
 *      Author: djc
 */

#include <PSController.h> /* #include <vmeDefs.h> */
#include <psDefs.h>
#include <utils.h>
#include <sis1100_api.h>

#include <string.h>


int getId(PSController *ctlr, char **id) {
	memcpy(id, ctlr->id, PSCONTROLLER_ID_SIZE);
	return 0;
}

int setId(PSController *ctlr, char **id) {
	memcpy(ctlr->id, id, PSCONTROLLER_ID_SIZE);
	return 0;
}

int setSetpoint(PSController *ctlr, int32_t sp) {

	return 0;
}

int getFeedback(PSController *ctlr, int32_t *fbk) {

	return 0;
}

int getChannel(PSController *ctlr, uint8_t *ch) {
	*ch = ctlr->channel;
	return 0;
}

int setChannel(PSController *ctlr, uint8_t ch) {
	ctlr->channel = ch;
	return 0;
}

int isInCorrection(PSController *ctlr, uint8_t *answer) {
	*answer = (ctlr->inCorrection != 0);
	return 0;
}

void UpdateSetPoint(VmeModule *mod, uint32_t vmeAddr, uint32_t channel, uint32_t setpoint) {
	int dacSetPoint;
    uint32_t value = 0;
    int rc;


    dacSetPoint = setpoint * DAC_AMP_CONV_FACTOR;

	value = (channel & PS_CHANNEL_MASK) << PS_CHANNEL_OFFSET;
    value = value | (dacSetPoint & 0xFFFFFF);

    /* write the 32 bits out to the power supply*/
#ifdef USE_MACRO_VME_ACCESSORS
    VmeWrite_32(mod,vmeAddr,value);
#else
    rc=vme_A24D32_write(mod->crate->fd,vmeAddr,value);
    if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
#endif
    /* added for Milan G IE Power 04/08/2002*/
    usecSpinDelay(30);

    /* toggle the PS_LATCH bit */
#ifdef USE_MACRO_VME_ACCESSORS
    VmeWrite_32(mod,vmeAddr,(value | PS_LATCH));
#else
    rc=vme_A24D32_write(mod->crate->fd,vmeAddr,(value | PS_LATCH));
    if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
#endif
    /* wait some time */
    usecSpinDelay(7);

    /* drop the PS_LATCH bit and data bits */
#ifdef USE_MACRO_VME_ACCESSORS
    VmeWrite_32(mod,vmeAddr,0x00000000);
#else
    rc=vme_A24D32_write(mod->crate->fd,vmeAddr,0x00000000);
    if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
#endif
	/* wait some time */
	usecSpinDelay(7);
}

void ToggleUpdateBit(VmeModule *mod, uint32_t vmeAddr, uint32_t channel, uint32_t setpoint) {
	int rc;

	/* raise the UPDATE bit */
#ifdef USE_MACRO_VME_ACCESSORS
    VmeWrite_32(mod,vmeAddr,UPDATE);
#else
	rc=vme_A24D32_write(mod->crate->fd,vmeAddr,UPDATE);
    if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
#endif
    /* wait some time */
    usecSpinDelay(7);

    /* drop the UPDATE bit */
#ifdef USE_MACRO_VME_ACCESSORS
	VmeWrite_32(mod,vmeAddr,0x00000000);
#else
    rc=vme_A24D32_write(mod->crate->fd,vmeAddr,0x00000000);
	if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
#endif
}
