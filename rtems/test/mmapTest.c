#include <sis1100_api.h>
#include <ics110bl.h>

#include "tests.h"

static void InitializeAdcModules(double targetFrameRate, int numChannelsPerFrame) {
	int i;
	
	/* add and initialize 1 ics-110bl ADC per VmeCrate */
	for(i=0; i<numVmeCrates; i++) {
		VmeModule *pmod;
		int rc;
		double actualRate = 0.0;
		
		pmod = (VmeModule *)calloc(1, sizeof(VmeModule));
		if(pmod == NULL) {
			syslog(LOG_INFO, "Failed to allocated VmeModule!\n");
			FatalErrorHandler(0);
		}
		/* insert essential info here... */
		pmod->crate = &VmeCrates[i];
		pmod->vmeBaseAddr = ICS110B_DEFAULT_BASEADDR;
		pmod->type = "ics-110bl";
		AdcModules[i] = pmod;
#ifdef USE_MACRO_VME_ACCESSORS
		rc = vme_set_mmap_entry(VmeCrates[i].fd, 0x00000000,
								0x39/*AM*/,0xFF010800/*hdr*/,
								(1<<24)-1,&(pmod->pcBaseAddr));
#endif		
		rc = InitICS110BL(pmod, targetFrameRate, &actualRate,
							INTERNAL_CLOCK, ICS110B_INTERNAL, 
							numChannelsPerFrame);
		if(rc) {
			syslog(LOG_INFO, "Failed to initialize adc %d\n");
			FatalErrorHandler(0);
		}
		syslog(LOG_INFO, "AdcModule[%d] rate is %.9f\n",i,actualRate);
	}
}

void StartMmapTest(const uint16_t aVector) {
	int i;
	uint16_t data = 0;
	
	syslog(LOG_INFO, "StartMmapTest() starting...\n");
	InitializeVmeCrate();
	InitializeAdcModules(10.0/*kHz*/, 32/*channelsPerFrame*/);
	
	/* try out some mmap'd accesses here... */
	for(i=0; i<numVmeCrates; i++) {
		printf("Reading from AdcModules[%d]\n",i);
		data = VmeRead_16(AdcModules[i], ICS110B_CTRL_OFFSET);
		printf("ctl reg=%#hx\n",data);
		data = VmeRead_16(AdcModules[i], ICS110B_STAT_RESET_OFFSET);
		printf("stat reg=%#hx\n",data);
		uint16_t vec = 0;
		uint16_t newvec = 0;
		vec = VmeRead_16(AdcModules[i], ICS110B_IVECT_OFFSET);
		printf("vec=%#hx\n",vec&0x00FF);
		vec=aVector;
		printf("Setting vec to %#hx\n",vec);
		VmeWrite_16(AdcModules[i],ICS110B_IVECT_OFFSET,vec);
		newvec = VmeRead_16(AdcModules[i], ICS110B_IVECT_OFFSET);
		printf("new vec=%#hx\n",newvec&0x00FF);
	}
	
/* clean up resources */	
	for(i=0; i<numVmeCrates; i++) {
		int rc;
		
		rc = vme_clr_mmap_entry(VmeCrates[i].fd, &(AdcModules[i]->pcBaseAddr), (1<<24)/*4 MB*/);
		if(rc) {
			syslog(LOG_INFO, "Failed to clear mmap entry #%d\n",i);
			FatalErrorHandler(0);
		}
	}
	ShutdownVmeCrates();
	syslog(LOG_INFO, "StartMmapTest() exiting...\n");
}
