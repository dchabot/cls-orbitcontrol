#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <math.h> /* for ceil(double), sqrt(), etc... */

#include <tscDefs.h>
#include <ics110bl.h>
#include <utils.h>
#include <sis1100_api.h>

#include "tests.h"

static int numIrqsHandled;
const int adcMaxChannels = 16256;

typedef struct {
	uint32_t numChannelsPerFrame;
	uint32_t numFrames;
	uint32_t *buf;
}RawDataSegment;

typedef struct {
	RawDataSegment *ds;
	VmeModule *adc;
	double tscSum;
	double tscSumSqrd;
	double tscMax;
}AdcIrqArg;

static AdcIrqArg *irqArgs[numAdcModules];

static void AdcStartAcquisition(void) {
	int i;
	
	for(i=0; i<numAdcModules; i++) {
		int rc = ICS110BStartAcquisition(AdcModules[i]);
		if(rc) {
			syslog(LOG_INFO, "ICS110BStartAcquisition() failed !!\n");
			FatalErrorHandler(0);
		}
	}
}

static void AdcStopAcquisition(void) {
	int i;
	
	for(i=0; i<numAdcModules; i++) {
		int rc = ICS110BStopAcquisition(AdcModules[i]);
		if(rc) {
			syslog(LOG_INFO, "ICS110BStopAcquisition() failed !!\n");
		}
	}
}

static void
AdcIrqHandler(void *arg, uint8_t vector) {
	AdcIrqArg *parg = (AdcIrqArg *)arg;
	int status = 0;
	uint64_t now,then,tmp;
	now=then=tmp=0;
	uint32_t wordsRead = 0;
	
	//printf("In AdcIrqHandler #%d with vector %d\n", (int)parg->adc->crate->id, (int)vector);
	/* disable irq on the board */
	status = ICS110BInterruptControl(parg->adc, ICS110B_IRQ_DISABLE);
	/* read a 1/2 FIFO of data */
	rdtscll(then);
	status |= ICS110BFifoRead(parg->adc, parg->ds->buf, parg->ds->numFrames*parg->ds->numChannelsPerFrame, &wordsRead);
	rdtscll(now);
	if(ICS110BIsEmpty(parg->adc)) {
		syslog(LOG_INFO, "AdcIrqHandler[%d]: adc FIFO is empty ?!?!\n", parg->adc->crate->id);
	}
	tmp = now-then;
	parg->tscSum += (double)tmp;
	parg->tscSumSqrd += (double)(tmp*tmp);
	if(tmp > parg->tscMax) {
		parg->tscMax = (double)tmp;
	}
	/* reEnable irq at the board */
	status |= ICS110BInterruptControl(parg->adc, ICS110B_IRQ_ENABLE);
	/* reEnable irq at the sis3100 */
	vme_enable_irq_level(parg->adc->crate->fd, parg->adc->irqLevel);
	if(status) {
		syslog(LOG_INFO, "Failure in AdcIrqHandler[%d]: rc=%d\n", parg->adc->crate->id, status);
		FatalErrorHandler(0);
	}
	if(parg->adc->crate->id == 0) {
		numIrqsHandled++;
	}
}

static void 
InitializeAdcModules(double targetFrameRate, double *trueFrameRate, int numChannelsPerFrame) {
	int i;
	
	/* add and initialize 1 ics-110bl ADC per VmeCrate */
	for(i=0; i<numVmeCrates; i++) {
		int rc;
		
		AdcIrqArg *parg = (AdcIrqArg *)calloc(1, sizeof(AdcIrqArg));
		if(parg == NULL) {
			syslog(LOG_INFO, "Failed to allocated VmeModule!\n");
			FatalErrorHandler(0);
		}
		parg->adc = (VmeModule *)calloc(1,sizeof(VmeModule));
		parg->ds = (RawDataSegment *)calloc(1, sizeof(RawDataSegment));
		/* insert essential info here... */
		parg->tscSum = 0;
		parg->tscSumSqrd = 0;
		parg->tscMax = 0;
		parg->adc->crate = &VmeCrates[i];
		parg->adc->vmeBaseAddr = ICS110B_DEFAULT_BASEADDR;
		parg->adc->type = "ics-110bl";
		parg->adc->irqLevel = 3; /*factory default*/
		parg->adc->irqVector = 123;
		AdcModules[i] = parg->adc;
#ifdef USE_MACRO_VME_ACCESSORS
		rc = vme_set_mmap_entry(VmeCrates[i].fd, 0x00000000,
										0x39/*AM*/,0xFF010800/*hdr*/,
										(1<<24)-1,&(AdcModules[i]->pcBaseAddr));
#endif				
		rc = InitICS110BL(parg->adc, targetFrameRate, trueFrameRate,
							INTERNAL_CLOCK, ICS110B_INTERNAL, 
							numChannelsPerFrame);
		if(rc) {
			syslog(LOG_INFO, "Failed to initialize adc #%d\n",i);
			FatalErrorHandler(0);
		}
		syslog(LOG_INFO, "AdcModule[%d] rate is %.9f\n",i,*trueFrameRate);
		/* set up IRQ handler per card */
		if(parg->ds != NULL) {
			parg->ds->numChannelsPerFrame = numChannelsPerFrame;
			parg->ds->numFrames = 2*adcMaxChannels/numChannelsPerFrame; /* i.e. a 1/4 FIFO */
			parg->ds->buf = (uint32_t *)calloc(1,adcMaxChannels*2*sizeof(uint32_t));
		}
		else {
			syslog(LOG_INFO, "Can't allocate DataSegment #%d\n",i);
			FatalErrorHandler(0);
		}
		ICS110BSetIrqVector(AdcModules[i],parg->adc->irqVector);
		rc = vme_set_isr(parg->adc->crate->fd, 
							parg->adc->irqVector/*vector*/,
							AdcIrqHandler/*handler*/,
							(void *)parg/*handler arg*/);
		rc |= vme_enable_irq_level(parg->adc->crate->fd, parg->adc->irqLevel);
		if(rc) {
			syslog(LOG_INFO, "Failed to set ADC IRQ on pass #%d",i);
			FatalErrorHandler(0);
		}
		irqArgs[i] = parg;
	}
}

void StartIrqTest(void) {
	extern double tscTicksPerSecond;/* = 2992044000.0*/
	const double adcFrameRate_kHz = 10.0; // in kHz
	double *adcTrueFrameRate;
//	const double HzPerkHz = 1000.0;
//	double adcFramesPerTick;
	const int adcChannelsPerFrame = 32;
//	const int adcBytesPerChannel = 4;
	rtems_interval rtemsTicksPerSecond;
	int i;
	const int numMeasurements = 12000;
	uint64_t now,then;
	now=then=0;
	void SpawnThread(void);
	
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);
	syslog(LOG_INFO, "Initializing vme crates...\n");
	InitializeVmeCrate();
	syslog(LOG_INFO, "Initializing adc modules...\n");
	InitializeAdcModules(adcFrameRate_kHz, adcTrueFrameRate, adcChannelsPerFrame);
	syslog(LOG_INFO, "Enabling ADC interrupts...");
	for(i=0; i<numVmeCrates; i++) {
		ICS110BInterruptControl(AdcModules[i],ICS110B_IRQ_ENABLE);
	}
	syslog(LOG_INFO, "DONE... Starting ADC data acquisition\n");
	rdtscll(then);
	AdcStartAcquisition();
	rdtscll(now);
	syslog(LOG_INFO,"AdcStartAcquisition() duration = %.9f\n",((double)(now-then)/(tscTicksPerSecond)));
	for(;;) {
		rtems_task_wake_after(1);
		if(numIrqsHandled < numMeasurements) { continue; }
		else {
			AdcStopAcquisition();
			for(i=0; i<numVmeCrates; i++) {
				int rc;
				
				rc = ICS110BInterruptControl(AdcModules[i], ICS110B_IRQ_DISABLE);
				rc |= vme_disable_irq_level(AdcModules[i]->crate->fd, AdcModules[i]->irqLevel);
				rc |= vme_clr_isr(AdcModules[i]->crate->fd, AdcModules[i]->irqVector);
				if(rc) {
					syslog(LOG_INFO, "Failure cleaning up interrupt resources of card %d rc=%d \n",i,rc);
					FatalErrorHandler(0);
				}
			}
			for(i=0; i<numVmeCrates; i++) {	
				double stdDev = (1.0/(double)(numMeasurements))*(irqArgs[i]->tscSumSqrd) - (1.0/(double)(numMeasurements*numMeasurements))*((irqArgs[i]->tscSum)*(irqArgs[i]->tscSum));
				stdDev = sqrt(stdDev);
				stdDev /= tscTicksPerSecond;
				double avg = irqArgs[i]->tscSum/((double)numMeasurements);
				avg /= tscTicksPerSecond;
				double maxTime = irqArgs[i]->tscMax;
				maxTime /= tscTicksPerSecond;
				
				syslog(LOG_INFO, "%d kWords/BLT, Avg time = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",adcMaxChannels,avg,stdDev,maxTime);
			}
			break;
		}
	}
	ShutdownVmeCrates();
	syslog(LOG_INFO, "StartIrqTest(): SpawnThread test...\n");
	SpawnThread();
	syslog(LOG_INFO, "StartIrqTest() exiting...\n");
}

