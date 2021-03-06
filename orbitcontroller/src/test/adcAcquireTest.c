#include <syslog.h>
#include <tscDefs.h>
#include <ics110bl.h>
#include <sis1100_api.h>

#include "tests.h"

static double InitializeAdcModules(double targetFrameRate, int numChannelsPerFrame) {
	int i;
	double actualRate = 0.0;
	
	/* add and initialize 1 ics-110bl ADC per VmeCrate */
	for(i=0; i<numVmeCrates; i++) {
		VmeModule *pmod;
		int rc;
		
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
	
	return actualRate;
}

/* test the "responsiveness" of ics-110bl acquire enable/disable bit:
 * 
 * i.e. will the ADC acquire frames "instantaneously" once the enable bit is set ?
 * 		will the ADC stop acquiring "instantaneously" once the enable bit is cleared ???
 * 
 * ANSWER: yes... to both... 
 * 
 */
void StartAdcAcquireTest(void) {
	double trueSampleRate = 0.0;
	extern double tscTicksPerSecond;
	uint64_t now, then, start;
	int i;
	
	syslog(LOG_INFO, "StartAdcAcquireTest() beginning...\n");
	now=then=start=0;
	InitializeVmeCrate();
	trueSampleRate = InitializeAdcModules(10.0/*kHz*/, 32/*channelsPerFrame*/);
	syslog(LOG_INFO, "trueSampleRate=%.9g\n", trueSampleRate);

	/* need to start test on the edge of an RTEMS tick... */
	rtems_task_wake_after(1);
	/*we're only going to play with one ADC...*/
	ICS110BStartAcquisition(AdcModules[0]);
	rdtscll(start);
	/* after 1 tick (==1 ms), we "should" have 10 frames in the adc's FIFO */
	rtems_task_wake_after(1);
	ICS110BStopAcquisition(AdcModules[0]);
	rdtscll(now);
	syslog(LOG_INFO, "adcAcquireTest: slept for %.9f [s]\n", ((double)(now-start))/tscTicksPerSecond);
	
	for(i=0; !ICS110BIsEmpty(AdcModules[0]); i++) {
		uint32_t data = readD32(AdcModules[0], ICS110B_FIFO_OFFSET);
	}
	syslog(LOG_INFO, "adcAcquireTest: read %d frames, %d channels\n", i/32, i);
	
	ShutdownVmeCrates();
}
