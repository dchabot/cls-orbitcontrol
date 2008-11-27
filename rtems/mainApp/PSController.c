/*
 * PSController.c
 *
 *  Created on: Sep 21, 2008
 *      Author: djc
 */
#include <syslog.h>

#include <utils.h>
#include <vmic2536.h>

#include <string.h>

#include <PSController.h> /* #include <vmeDefs.h> */
#include <DaqController.h> /* InitializeDioModule declaration */

static DioConfig dioConfig[] = {
		{VMIC_2536_DEFAULT_BASE_ADDR,0},
		{VMIC_2536_DEFAULT_BASE_ADDR,1},
		{VMIC_2536_DEFAULT_BASE_ADDR,2},
		{VMIC_2536_DEFAULT_BASE_ADDR,3}
#if NumDioModules==5
		,{VMIC_2536_DEFAULT_BASE_ADDR+0x10,3}
#endif
};

static VmeModule *dioArray[NumDioModules];

static PSController psCtlrArray[] = {
	/* id, setpoint, feedback, channel, inCorrection, crateId, modAddr, VmeModule*   */
	/* first, the horizontal controllers (OCH14xx-xx): */
	{"OCH1401-01",0,0,9,1,0,0x00700000,NULL},
//	{"SOA1401-01:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1401-02:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1401-02",0,0,12,1,0,0x00700000,NULL},
	{"OCH1402-01",0,0,13,1,0,0x00700000,NULL},
//	{"SOA1402-01:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1402-02:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1402-02",0,0,14,1,0,0x00700000,NULL},
	{"OCH1403-01",0,0,7,1,1,0x00700000,NULL},
//	{"SOA1403-01:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1403-02:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1403-02",0,0,8,1,1,0x00700000,NULL},
	{"OCH1404-01",0,0,9,1,1,0x00700000,NULL},
//	{"SOA1404-01:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1404-02:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1404-02",0,0,12,1,1,0x00700000,NULL},
	{"OCH1405-01",0,0,13,1,1,0x00700000,NULL},
//	{"SOA1405-01:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1405-02:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1405-02",0,0,14,1,1,0x00700000,NULL},
	{"OCH1406-01",0,0,7,1,2,0x00700000,NULL},
//	{"SOA1406-01:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1406-02:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1406-02",0,0,8,1,2,0x00700000,NULL},
	{"OCH1407-01",0,0,9,1,2,0x00700000,NULL},
//	{"SOA1407-01:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1407-02:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1407-02",0,0,12,1,2,0x00700000,NULL},
	{"OCH1408-01",0,0,13,1,2,0x00700000,NULL},
//	{"SOA1408-01:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1408-02:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1408-02",0,0,14,1,2,0x00700000,NULL},
	{"OCH1409-01",0,0,7,1,3,0x00700000,NULL},
//	{"SOA1409-01:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1409-02:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1409-02",0,0,8,1,3,0x00700000,NULL},
	{"OCH1410-01",0,0,9,1,3,0x00700000,NULL},
//	{"SOA1410-01:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1410-02:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1410-02",0,0,12,1,3,0x00700000,NULL},
	{"OCH1411-01",0,0,13,1,3,0x00700000,NULL},
//	{"SOA1411-01:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1411-02:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1411-02",0,0,14,1,3,0x00700000,NULL},
	{"OCH1412-01",0,0,7,1,0,0x00700000,NULL},
//	{"SOA1412-01:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1412-02:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1412-02",0,0,8,1,0,0x00700000,NULL},

	/* next, the vertical correctors (OCVxx-xx): */
	{"OCV1401-01",0,0,3,1,0,0x00700000,NULL},
//	{"SOA1401-01:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1401-02:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1401-02",0,0,4,1,0,0x00700000,NULL},
	{"OCV1402-01",0,0,5,1,0,0x00700000,NULL},
//	{"SOA1402-01:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1402-02:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1402-02",0,0,6,1,0,0x00700000,NULL},
	{"OCV1403-01",0,0,1,1,1,0x00700000,NULL},
//	{"SOA1403-01:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1403-02:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1403-02",0,0,2,1,1,0x00700000,NULL},
	{"OCV1404-01",0,0,3,1,1,0x00700000,NULL},
//	{"SOA1404-01:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1404-02:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1404-02",0,0,4,1,1,0x00700000,NULL},
	{"OCV1405-01",0,0,5,1,1,0x00700000,NULL},
//	{"SOA1405-01:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1405-02:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1405-02",0,0,6,1,1,0x00700000,NULL},
	{"OCV1406-01",0,0,1,1,2,0x00700000,NULL},
//	{"SOA1406-01:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1406-02:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1406-02",0,0,2,1,2,0x00700000,NULL},
	{"OCV1407-01",0,0,3,1,2,0x00700000,NULL},
//	{"SOA1407-01:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1407-02:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1407-02",0,0,4,1,2,0x00700000,NULL},
	{"OCV1408-01",0,0,5,1,2,0x00700000,NULL},
//	{"SOA1408-01:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1408-02:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1408-02",0,0,6,1,2,0x00700000,NULL},
	{"OCV1409-01",0,0,1,1,3,0x00700000,NULL},
//	{"SOA1409-01:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1409-02:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1409-02",0,0,2,1,3,0x00700000,NULL},
	{"OCV1410-01",0,0,3,1,3,0x00700000,NULL},
//	{"SOA1410-01:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1410-02:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1410-02",0,0,4,1,3,0x00700000,NULL},
	{"OCV1411-01",0,0,5,1,3,0x00700000,NULL},
//	{"SOA1411-01:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1411-02:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1411-02",0,0,6,1,3,0x00700000,NULL},
	{"OCV1412-01",0,0,1,1,0,0x00700000,NULL},
//	{"SOA1412-01:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1412-02:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1412-02",0,0,2,1,0,0x00700000,NULL}
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

bool isInCorrection(PSController *ctlr) {
	return (ctlr->inCorrection != 0);
}

PSController*
getPSControllerByName(char* ctlrName) {
	int i;

	for(i=0; i<NumOCM; i++) {
		if(strcmp(ctlrName, psCtlrArray[i].id)==0) {
			return &psCtlrArray[i];
		}
	}
	//PSController not found
	return NULL;
}

/* UpdateSetpoint will change a single PS channel's setpoint, BUT toggling
 * the UPDATE bit is STILL req'd to affect a physical change in channel's current.
 */
int UpdateSetpoint(PSController* ctlr, int32_t setpoint) {
	uint32_t value = 0;
	int32_t dacSetPoint = 0;
	int rc;

	if(!ctlr->inCorrection) return 0;

	ctlr->setpoint = setpoint;
	dacSetPoint = ctlr->setpoint * DAC_AMP_CONV_FACTOR;

	value = (ctlr->channel & PS_CHANNEL_MASK) << PS_CHANNEL_OFFSET;
	value = value | (dacSetPoint & 0xFFFFFF);

	/* write the 32 bits out to the power supply*/
	rc = VMIC2536_setOutput(ctlr->mod, value);
	if(rc) { goto bailout; }
	/* added for Milan G IE Power 04/08/2002*/
	usecSpinDelay(ISO_DELAY);

	/* toggle the PS_LATCH bit */
	rc = VMIC2536_setOutput(ctlr->mod, (value | PS_LATCH));
	if(rc) { goto bailout; }
	usecSpinDelay(ISO_DELAY);

	/* drop the PS_LATCH bit and data bits */
	rc = VMIC2536_setOutput(ctlr->mod, 0UL);
	if(rc) { goto bailout; }
	usecSpinDelay(ISO_DELAY);
	return 0;

bailout:
	syslog(LOG_INFO, "UpdateSetPoint: failed VME write--%#x\n\tid=%s\n",rc,ctlr->id);
	return -1;
}

void DistributeSetpoints(int32_t *spArray) {
    int i,rc;

    for(i=0; i<NumOCM; i++) {
    	rc = UpdateSetpoint(&psCtlrArray[i], spArray[i]);
    	if(rc != 0) {
    		syslog(LOG_INFO, "UpdateSetPoint: failed VME write--%#x\n\tid=%s\n",
    				rc, psCtlrArray[i].id);
    	}

    }

}

/* FIXME!!!
 * NOTE: this routine's argument is VmeModule*, reflecting the idea that
 * 			an UPDATE should be performed ONCE per Power Supply Bulk !!
 *
 * NOTE: this also implies each bulk is serviced by exactly ONE vmic2536.
 *		This assumption will be broken once the pwrSupply upgrades are complete...
 */
void ToggleUpdateBit(VmeModule* mod) {
	int rc;

	/* raise the UPDATE bit */
	rc = VMIC2536_setOutput(mod, UPDATE);
	if(rc) { goto bailout; }
    /* wait some time */
    usecSpinDelay(ISO_DELAY);

    /* drop the UPDATE bit */
    rc = VMIC2536_setOutput(mod, 0UL);
    if(rc) { goto bailout; }
    usecSpinDelay(ISO_DELAY);
    return;
bailout:
	syslog(LOG_INFO, "ToggleUpdateBit: failed VME write--rc=%#x,crate=%d,vmeAddr=%#x\n",
			rc,mod->crate->id,mod->vmeBaseAddr);
}

void ToggleUpdateBits() {
	int i;

	for(i=0; i<NumDioModules; i++) {
		ToggleUpdateBit(dioArray[i]);
	}
}

void SimultaneousSetpointUpdate(int32_t *spBuf) {
	DistributeSetpoints(spBuf);
	ToggleUpdateBits();
}

/* SetSingleSetpoint() will update PS channel setpoint AND toggle
 * the UPDATE bit to affect the change.
 *
 * NOTE: we bypass the vmic2536 API and use the sis1100 API directly here
 * 			because USE_MACRO_VME_ACCESSORS may be defined AND this routine
 * 			could be called asynchronously. Thus we invoke thread-safe routines
 * 			to avoid clobbering any concurrent VME block-transfers in progress.
 */
int SetSingleSetpoint(PSController* ctlr, int32_t setpoint) {
	int dacSetPoint;
	uint32_t value = 0;
	int rc, fd;
	uint32_t vmeAddr;

	dacSetPoint = setpoint * DAC_AMP_CONV_FACTOR;
	ctlr->setpoint = dacSetPoint;
	value = (ctlr->channel & PS_CHANNEL_MASK) << PS_CHANNEL_OFFSET;
	value = value | (dacSetPoint & 0xFFFFFF);

	fd = ctlr->mod->crate->fd;
	vmeAddr = ctlr->modAddr;
	/* write the 32 bits out to the power supply*/
	rc=vme_A24D32_write(fd,vmeAddr,value);
	if(rc) {
		syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
		return rc;
	}
	/* added for Milan G IE Power 04/08/2002*/
	usecSpinDelay(ISO_DELAY);

	/* raise the PS_LATCH bit */
	rc=vme_A24D32_write(fd,vmeAddr,(value | PS_LATCH));
	if(rc) {
		syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
		return rc;
	}
	usecSpinDelay(ISO_DELAY);

	/* drop the PS_LATCH bit and data bits */
	rc=vme_A24D32_write(fd,vmeAddr,0x00000000);
	if(rc) {
		syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
		return rc;
	}
	usecSpinDelay(ISO_DELAY);

	/* raise the UPDATE bit */
	rc=vme_A24D32_write(fd,vmeAddr,UPDATE);
	if(rc) {
		syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
		return rc;
	}
	usecSpinDelay(ISO_DELAY);

	/* drop the UPDATE bit */
	rc=vme_A24D32_write(fd,vmeAddr,0x00000000);
	if(rc) {
		syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
		return rc;
	}
	usecSpinDelay(ISO_DELAY);

	return 0;
}

VmeModule* InitializeDioModule(VmeCrate* vmeCrate, uint32_t baseAddr) {
	int rc;
	VmeModule *pmod = NULL;


	pmod = (VmeModule *)calloc(1,sizeof(VmeModule));
	if(pmod==NULL) {
		syslog(LOG_INFO, "Can't allocate mem for VMIC-2536\n");
		FatalErrorHandler(0);
	}
	pmod->crate = vmeCrate;
	pmod->type = "VMIC-2536";
	pmod->vmeBaseAddr = baseAddr;
	pmod->pcBaseAddr = vmeCrate->a24BaseAddr;

	rc = VMIC2536_Init(pmod);
	if(rc) {
		syslog(LOG_INFO, "Failed to initialize VMIC-2536: rc=%d",rc);
		FatalErrorHandler(0);
	}

	return pmod;
}

void ShutdownDioModules(VmeModule *modArray[], int numModules) {
	int i;

	for(i=0; i<numModules; i++) {
		free(modArray[i]);
	}
}

/*
 * The simplest thing to do here is to statically initialize psCtlrArray[i].mod
 * with a similarly statically initialized dioArray[j].
 */
void InitializePSControllers(VmeCrate** crateArray) {
	int i,j;

	/* First, initialize the vmic2536 DIO modules */
	/** Init the vmic2536 DIO modules */
	for(i=0; i<NumDioModules; i++) {
		dioArray[i] = InitializeDioModule(crateArray[dioConfig[i].vmeCrateID], dioConfig[i].baseAddr);
		syslog(LOG_INFO, "Initialized VMIC2536 DIO module[%d]\n",i);
	}

	for(i=0; i<NumDioModules; i++) {
		for(j=0; j<NumOCM; j++) {
			if((psCtlrArray[j].crateId == dioArray[i]->crate->id)
					&& (psCtlrArray[j].modAddr == dioArray[i]->vmeBaseAddr)) {
				psCtlrArray[j].mod = dioArray[i];
				syslog(LOG_INFO, "psCtlrArray[%d]: id=%s, crateId=%d\n",
						j,psCtlrArray[j].id,psCtlrArray[j].crateId);
			}
		}
	}
}
