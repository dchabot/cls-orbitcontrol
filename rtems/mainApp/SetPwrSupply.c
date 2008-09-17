#include <stdint.h>

#include <sis1100_api.h>
#include <utils.h> /* tscSpinDelay() */
#include <psDefs.h>



void SetPwrSupply(int fd, uint32_t vmeAddr, uint32_t channel, uint32_t setpoint) {
	int dacSetPoint;
    uint32_t value = 0;
    int rc;


    dacSetPoint = setpoint * DAC_AMP_CONV_FACTOR;

	value = (channel & PS_CHANNEL_MASK) << PS_CHANNEL_OFFSET;
    value = value | (dacSetPoint & 0xFFFFFF);

    /* write the 32 bits out to the power supply*/
    //VmeWrite_32(mod,vmeAddr,value);
    rc=vme_A24D32_write(fd,vmeAddr,value);
    if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
    /* added for Milan G IE Power 04/08/2002*/
    usecSpinDelay(30);

    /* raise the PS_LATCH bit */
    //VmeWrite_32(mod,vmeAddr,(value | PS_LATCH));
    rc=vme_A24D32_write(fd,vmeAddr,(value | PS_LATCH));
    if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
    /* wait some time */
    usecSpinDelay(7);

    /* drop the PS_LATCH bit and data bits */
    //VmeWrite_32(mod,vmeAddr,0x00000000);
    rc=vme_A24D32_write(fd,vmeAddr,0x00000000);
    if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
	/* wait some time */
	usecSpinDelay(7);

    /* raise the UPDATE bit */
    //VmeWrite_32(mod,vmeAddr,UPDATE);
	rc=vme_A24D32_write(fd,vmeAddr,UPDATE);
    if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
    /* wait some time */
    usecSpinDelay(7);

    /* drop the UPDATE bit */
	//VmeWrite_32(mod,vmeAddr,0x00000000);
    rc=vme_A24D32_write(fd,vmeAddr,0x00000000);
	if(rc) {
    	syslog(LOG_INFO, "SetPwrSupply: failed VME write--%#x\n",rc);
    	return;
    }
}
