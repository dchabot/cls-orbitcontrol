#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>

#include <sis1100_api.h>
#include <ics110bl.h>
#include <vmic2536.h>
#include <utils.h> /*TestDirective()*/

#include "DaqController.h"

/* FIXME -- refactor (extract method) to init a *single* crate
 *
 * Assumes one sis1100/3100 per crate
 * AND that the VME-PCI devices have been initialized
 */
void InitializeVmeCrates(VmeCrate *crateArray[], int numCrates) {
	int i;
	extern int errno;

	for(i=0; i<numCrates; i++) {
		int fd;
		char devName[64];

		crateArray[i] = (VmeCrate *)calloc(1,sizeof(VmeCrate));
		if(crateArray[i]==NULL) {
			syslog(LOG_INFO, "Failed to allocate vme crate[%d]:\n\t%s",i,strerror(errno));
			FatalErrorHandler(0);
		}
		crateArray[i]->id = i;
		sprintf(devName, "/dev/sis1100_%d",i);
		fd = open(devName, O_RDWR, 0);
		if(fd < 0) {
			syslog(LOG_INFO, "Failed to open %s: %s\n",devName, strerror(fd));
			FatalErrorHandler(0);
		}
		crateArray[i]->fd = fd;
#ifdef USE_MACRO_VME_ACCESSORS
		rtems_status_code rc = vme_set_mmap_entry(fd, 0x00000000,
								0x39/*AM*/,0xFF010800/*hdr*/,
								(1<<24)-1,&(crateArray[i]->a24BaseAddr));
		TestDirective(rc, "vme_set_mmap_entry()");
		syslog(LOG_INFO, "crate# %d, VME A24 base-address=%p",
				i,crateArray[i]->a24BaseAddr);
#endif
		/* do a VME System-Reset */
		vmesysreset(fd);
	}
	syslog(LOG_INFO, "Initialized %d vme crates\n",numCrates);
}

void ShutdownVmeCrates(VmeCrate *crateArray[], int numCrates) {
	int i;
	rtems_status_code rc;

	for(i=0; i<numCrates; i++) {
		rc = vme_clr_mmap_entry(crateArray[i]->fd, &crateArray[i]->a24BaseAddr, (1<<24)-1);
		TestDirective(rc, "vme_clr_mmap_entry()");
		crateArray[i]->id = 0;
		close(crateArray[i]->fd);
		free(crateArray[i]);
	}
}

/* FIXME -- refactor (extract method) to init a *single* module */
VmeModule* InitializeAdcModule(VmeCrate *vmeCrate,
							uint32_t baseAddr,
							double targetFrameRate,
							int numChannelsPerFrame,
							double *trueFrameRate)
{
	VmeModule *pmod;
	double actualRate = 0.0;
	int rc;

	pmod = (VmeModule *)calloc(1, sizeof(VmeModule));
	if(pmod == NULL) {
		syslog(LOG_INFO, "Failed to allocated ADC Module!\n");
		FatalErrorHandler(0);
	}
	/* insert essential info here...*/
	pmod->crate = vmeCrate;
	pmod->vmeBaseAddr = baseAddr;
	pmod->type = "ics-110bl";
	pmod->irqLevel = ICS110B_DEFAULT_IRQ_LEVEL;
	pmod->irqVector = ICS110B_DEFAULT_IRQ_VECTOR;
#ifdef USE_MACRO_VME_ACCESSORS
	pmod->pcBaseAddr = vmeCrate->a24BaseAddr;
#endif
	rc = InitICS110BL(pmod, targetFrameRate, &actualRate,
						INTERNAL_CLOCK, ICS110B_INTERNAL,
						numChannelsPerFrame);
	if(rc) {
		syslog(LOG_INFO, "Failed to initialize Adc !!\n");
		FatalErrorHandler(0);
	}
	*trueFrameRate = actualRate;

	return pmod;
}

void ShutdownAdcModules(VmeModule *modArray[], int numModules) {
	int i;

	for(i=0; i<numModules; i++) {
		free(modArray[i]);
	}
}

void AdcStartAcquisition(VmeModule *modArray[], int numModules) {
	int i;

	for(i=0; i<numModules; i++) {
		int rc = ICS110BStartAcquisition(modArray[i]);
		if(rc) {
			syslog(LOG_INFO, "ICS110BStartAcquisition() failed !!\n");
			FatalErrorHandler(0);
		}
	}
}

void AdcStopAcquisition(VmeModule *modArray[], int numModules) {
	int i;

	for(i=0; i<numModules; i++) {
		int rc = ICS110BStopAcquisition(modArray[i]);
		if(rc) {
			syslog(LOG_INFO, "ICS110BStopAcquisition() failed !!\n");
		}
	}
}

/* XXX -- still need to set the correct VME interrupt level to be "fully armed"... */
void AdcInstallIsr(VmeModule *mod, sis1100VmeISR isr, void *isrArg) {
	rtems_status_code rc;

	rc = vme_set_isr(mod->crate->fd,
						mod->irqVector/*vector*/,
						isr/*handler*/,
						isrArg/*handler arg*/);
	if(rc) {
		syslog(LOG_INFO, "Failed to set ADC Isr\n");
		FatalErrorHandler(0);
	}
	ICS110BSetIrqVector(mod, mod->irqVector);
}

void AdcRemoveIsr(VmeModule *mod) {
	rtems_status_code rc;

	rc = vme_disable_irq_level(mod->crate->fd, (1<<mod->irqLevel));
	rc |= vme_clr_isr(mod->crate->fd, mod->irqVector);
	if(rc) {
		syslog(LOG_INFO, "Failed to remove ADC Isr\n");
		FatalErrorHandler(0);
	}
}

void StartDaqController(rtems_task_entry entryPoint) {
	rtems_id tid;
	rtems_status_code rc;

	rc = rtems_task_create(DaqControllerThreadName,
							DaqThreadPriority,/*priority*/
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,/*XXX floating point context req'd ??*/
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&tid);
	TestDirective(rc, "rtems_task_create()");
	rc = rtems_task_start(tid, entryPoint, 0);
	TestDirective(rc, "rtems_task_start()");
}
