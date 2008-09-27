/*
 * PSController.c
 *
 *  Created on: Sep 21, 2008
 *      Author: djc
 */

/* TODO -- replace sis1100 API calls with libvmic2536 API */

#include <psDefs.h>
#include <utils.h>
#include <sis1100_api.h>
#include <vmic2536.h>

#include <string.h>

#include "PSController.h" /* #include <vmeDefs.h> */

static PSController psCtlrArray[] = {
	/* id, setpoint, feedback, channel, inCorrection, crateId, VmeModule*   */
	/* first, the horizontal controllers (OCH14xx-xx): */
	{"OCH1401-01",0,0,9,1,0,NULL},
	{"OCH1401-02",0,0,12,1,0,NULL},
	{"OCH1402-01",0,0,13,1,0,NULL},
	{"OCH1402-02",0,0,14,1,0,NULL},
	{"OCH1403-01",0,0,1,1,1,NULL},
	{"OCH1403-02",0,0,2,1,1,NULL},
	{"OCH1404-01",0,0,3,1,1,NULL},
	{"OCH1404-02",0,0,4,1,1,NULL},
	{"OCH1405-01",0,0,5,1,1,NULL},
	{"OCH1405-02",0,0,6,1,1,NULL},
	{"OCH1406-01",0,0,7,1,2,NULL},
	{"OCH1406-02",0,0,8,1,2,NULL},
	{"OCH1407-01",0,0,9,1,2,NULL},
	{"OCH1407-02",0,0,12,1,2,NULL},
	{"OCH1408-01",0,0,13,1,2,NULL},
	{"OCH1408-02",0,0,14,1,2,NULL},
	{"OCH1409-01",0,0,7,1,3,NULL},
	{"OCH1409-02",0,0,8,1,3,NULL},
	{"OCH1410-01",0,0,9,1,3,NULL},
	{"OCH1410-02",0,0,12,1,3,NULL},
	{"OCH1411-01",0,0,13,1,3,NULL},
	{"OCH1411-02",0,0,14,1,3,NULL},
	{"OCH1412-01",0,0,7,1,0,NULL},
	{"OCH1412-02",0,0,8,1,0,NULL},

	/* next, the vertical correctors (OCVxx-xx): */
	{"OCV1401-01",0,0,3,1,0,NULL},
	{"OCV1401-02",0,0,4,1,0,NULL},
	{"OCV1402-01",0,0,5,1,0,NULL},
	{"OCV1402-02",0,0,6,1,0,NULL},
	{"OCV1403-01",0,0,1,1,1,NULL},
	{"OCV1403-02",0,0,2,1,1,NULL},
	{"OCV1404-01",0,0,3,1,1,NULL},
	{"OCV1404-02",0,0,4,1,1,NULL},
	{"OCV1405-01",0,0,5,1,1,NULL},
	{"OCV1405-02",0,0,6,1,1,NULL},
	{"OCV1406-01",0,0,1,1,2,NULL},
	{"OCV1406-02",0,0,2,1,2,NULL},
	{"OCV1407-01",0,0,3,1,2,NULL},
	{"OCV1407-02",0,0,4,1,2,NULL},
	{"OCV1408-01",0,0,5,1,2,NULL},
	{"OCV1408-02",0,0,6,1,2,NULL},
	{"OCV1409-01",0,0,1,1,3,NULL},
	{"OCV1409-02",0,0,2,1,3,NULL},
	{"OCV1410-01",0,0,3,1,3,NULL},
	{"OCV1410-02",0,0,4,1,3,NULL},
	{"OCV1411-01",0,0,5,1,3,NULL},
	{"OCV1411-02",0,0,6,1,3,NULL},
	{"OCV1412-01",0,0,1,1,0,NULL},
	{"OCV1412-02",0,0,2,1,0,NULL}
};

/*int getId(PSController *ctlr, char **id) {
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
}*/

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

void UpdateSetPoints(int32_t *spArray) {
	int dacSetPoint;
    uint32_t value = 0;
    int i,rc;

    for(i=0; i<NumOCM; i++) {
    	psCtlrArray[i].setpoint = spArray[i];
    	PSController *ctlr = &psCtlrArray[i];
		dacSetPoint = ctlr->setpoint * DAC_AMP_CONV_FACTOR;

		value = (ctlr->channel & PS_CHANNEL_MASK) << PS_CHANNEL_OFFSET;
		value = value | (dacSetPoint & 0xFFFFFF);

		/* write the 32 bits out to the power supply*/
	#ifdef USE_MACRO_VME_ACCESSORS
		VmeWrite_32(ctlr->mod,VMIC_2536_OUTPUT_REG_OFFSET,value);
	#else
		rc=vme_A24D32_write(ctlr->mod->crate->fd,
							ctlr->mod->vmeBaseAddr+VMIC_2536_OUTPUT_REG_OFFSET,
							value);
		if(rc) { goto bailout; }
	#endif
		/* added for Milan G IE Power 04/08/2002*/
		usecSpinDelay(30);

		/* toggle the PS_LATCH bit */
	#ifdef USE_MACRO_VME_ACCESSORS
		VmeWrite_32(ctlr->mod,VMIC_2536_OUTPUT_REG_OFFSET,(value | PS_LATCH));
	#else
		rc=vme_A24D32_write(ctlr->mod->crate->fd,
							ctlr->mod->vmeBaseAddr+VMIC_2536_OUTPUT_REG_OFFSET,
							(value | PS_LATCH));
		if(rc) { goto bailout; }
	#endif
		/* wait some time */
		usecSpinDelay(7);

		/* drop the PS_LATCH bit and data bits */
	#ifdef USE_MACRO_VME_ACCESSORS
		VmeWrite_32(ctlr->mod,VMIC_2536_OUTPUT_REG_OFFSET,0UL);
	#else
		rc=vme_A24D32_write(ctlr->mod->crate->fd,
							ctlr->mod->vmeBaseAddr+VMIC_2536_OUTPUT_REG_OFFSET,
							0UL);
		if(rc) { goto bailout; }
	#endif
		/* wait some time */
		usecSpinDelay(7);
		continue;
bailout:
		syslog(LOG_INFO, "UpdateSetPoint: failed VME write--%#d\n\tid=%s\n",rc,ctlr->id);
		return;
    }

}

void ToggleUpdateBit(VmeModule* mod) {
	int rc;

	/* raise the UPDATE bit */
#ifdef USE_MACRO_VME_ACCESSORS
    VmeWrite_32(mod,VMIC_2536_OUTPUT_REG_OFFSET,UPDATE);
#else
	rc=vme_A24D32_write(mod->crate->fd,
						mod->vmeBaseAddr+VMIC_2536_OUTPUT_REG_OFFSET,
						UPDATE);
    if(rc) {
    	syslog(LOG_INFO, "ToggleUpdateBit: failed VME write--%#x\n",rc);
    	return;
    }
#endif
    /* wait some time */
    usecSpinDelay(7);

    /* drop the UPDATE bit */
#ifdef USE_MACRO_VME_ACCESSORS
	VmeWrite_32(mod,VMIC_2536_OUTPUT_REG_OFFSET,0UL);
#else
	rc=vme_A24D32_write(mod->crate->fd,
						mod->vmeBaseAddr+VMIC_2536_OUTPUT_REG_OFFSET,
						0UL);
	if(rc) {
    	syslog(LOG_INFO, "ToggleUpdateBit: failed VME write--%#x\n",rc);
    	return;
    }
#endif
}

void InitializePSControllers(VmeModule **modArray) {
	int i,j;
	const int numDioMods = 4;

	for(i=0; i<numDioMods; i++) {
		for(j=0; j<NumOCM; j++) {
			if(modArray[i]->crate->id == psCtlrArray[j].crateId) {
				psCtlrArray[j].mod = modArray[i];
				/*syslog(LOG_INFO, "psCtlrArray[%d]: id=%s, crateId=%d\n",
						j,psCtlrArray[j].id,psCtlrArray[j].crateId);*/
			}
		}
	}
}
