#include <stdint.h>
#include <syslog.h>

#include <sis1100_api.h>
#include <utils.h> /* tscSpinDelay() */
#include <vmeDefs.h>
#include <vmic2536.h>
#include "PSController.h"
#include "psDefs.h"



void SetSingleChannel(PSController* ctlr, uint32_t setpoint) {
	int dacSetPoint;
    uint32_t value = 0;
    int rc;


    dacSetPoint = setpoint * DAC_AMP_CONV_FACTOR;

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

    /* raise the UPDATE bit */
	rc = VMIC2536_setOutput(ctlr->mod, UPDATE);
	if(rc) { goto bailout; }
	usecSpinDelay(ISO_DELAY);

    /* drop the UPDATE bit */
	rc = VMIC2536_setOutput(ctlr->mod, 0UL);
	if(rc) { goto bailout; }
	usecSpinDelay(ISO_DELAY);

bailout:
	syslog(LOG_INFO, "UpdateSetPoint: failed VME write--%#x\n\tid=%s\n",rc,ctlr->id);
}
