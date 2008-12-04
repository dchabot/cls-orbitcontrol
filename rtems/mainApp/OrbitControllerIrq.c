#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include <sis1100_api.h>
#include <ics110bl.h>
#include <vmic2536.h>
#include <utils.h> /*TestDirective()*/

//#include <rtems-gdb-stub.h>
#include <OrbitController.h>
#include <dataDefs.h>
#include <AdcReaderThread.h>
#include <DataHandler.h>
#include <PSController.h>

/*FIXME -- global var */
rtems_id OcmSetpointQID = 0;

typedef struct {
	VmeModule *adc;
	rtems_id barrierID;
} AdcIsrArg;

static void AdcIsr(void *arg, uint8_t vector) {
	AdcIsrArg *parg = (AdcIsrArg *)arg;
	int rc;

	/* disable irq on the board */
	ICS110BInterruptControl(parg->adc, ICS110B_IRQ_DISABLE);
	/* inform the OrbitController of this event */
	rc = rtems_barrier_wait(parg->barrierID,RTEMS_NO_TIMEOUT);
	TestDirective(rc,"rtems_barrier_wait-AdcIsr");
}

static void RegisterAdcIsr(VmeModule* adc, rtems_id bid) {
	int rc;
	AdcIsrArg *parg = NULL;

	parg = (AdcIsrArg *)calloc(1,sizeof(AdcIsrArg));
	if(parg==NULL) {
		syslog(LOG_INFO, "Failed to allocated AdcIsrArg\n");
		FatalErrorHandler(0);
	}
	parg->adc = adc;
	parg->barrierID = bid;
	rc = vme_set_isr(adc->crate->fd,
						adc->irqVector/*vector*/,
						AdcIsr/*handler*/,
						parg/*handler arg*/);
	if(rc) {
		syslog(LOG_INFO, "Failed to set ADC Isr, crate# %d\n",adc->crate->id);
		FatalErrorHandler(0);
	}
	ICS110BSetIrqVector(adc, adc->irqVector);
	/* arm the appropriate VME interrupt level at each sis3100 & ADC... */
	rc = vme_enable_irq_level(adc->crate->fd, adc->irqLevel);
	TestDirective(rc, "vme_enable_irq_level()");
	ICS110BInterruptControl(adc,ICS110B_IRQ_ENABLE);
}


rtems_task OrbitControllerIrq(rtems_task_argument arg) {
	extern int errno;
	static VmeCrate *crateArray[NumVmeCrates];
	static VmeModule *adcArray[NumAdcModules];
	int adcFramesPerTick;
	double adcFrequency = 0.0;
	double adcTrueFrequency = 0.0;
	static AdcIsrArg *IsrArgList[NumAdcModules];
	static ReaderThreadArg *rdrArray[NumReaderThreads];
	RawDataSegment rdSegments[NumReaderThreads];
	extern rtems_id OcmSetpointQID;
	uint32_t ocmSetpointMQpending = 0;
	rtems_id isrBarrierId;
	rtems_id rdrBarrierId;
	rtems_id rawDataQID = 0;
	rtems_id dataHandlerTID = 0;
	rtems_status_code rc;
	rtems_interval rtemsTicksPerSecond;
	int i;
	uint64_t loopIterations = 0;
	const int readSizeFrames = (HALF_FIFO_LENGTH/(/*4**/AdcChannelsPerFrame)); /* @ 32 channels/frame, readSizeFrames==508 */

	/* Begin... */
	syslog(LOG_INFO, "OrbitControllerIrq: initializing...\n");
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);

	/* setup software-to-hardware objects */
	for(i=0; i<NumVmeCrates; i++) {
		crateArray[i] = InitializeVmeCrate(i);
	}
	syslog(LOG_INFO, "Initialized %d VME crates.\n",i);

	/** Initialize the ics1100-bl ADC modules */
	if(adcFrequency==0.0) {
		/* use default */
		adcFrequency = AdcDefaultFrequency;
	}

	for(i=0; i<NumAdcModules; i++) {
		adcArray[i] = InitializeAdcModule(crateArray[i],
											ICS110B_DEFAULT_BASEADDR,
											adcFrequency,
											AdcChannelsPerFrame,
											&adcTrueFrequency);
		syslog(LOG_INFO, "Adc[%d] rate is %.6f kHz\n",i,adcTrueFrequency);
	}
	adcFramesPerTick = (int)ceil((adcTrueFrequency*HzPerkHz)/((double)rtemsTicksPerSecond));

	rc = rtems_barrier_create(rtems_build_name('i','s','r','B'),
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&isrBarrierId);
	TestDirective(rc, "rtems_barrier_create()-isr barrier");
	/* register ADC interrupt service routines with sis1100 driver... */
	for(i=0; i<NumAdcModules; i++) {
		RegisterAdcIsr(adcArray[i], isrBarrierId);
	}

	rc = rtems_barrier_create(rtems_build_name('a','d','c','B'),
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&rdrBarrierId);
	TestDirective(rc, "rtems_barrier_create()-adc barrier");
	/* set up AdcReaderThreads and synchronize with them (Rendezvous pattern) */
	for(i=0; i<NumReaderThreads; i++) {
		rdrArray[i] = startReaderThread(adcArray[i], rdrBarrierId);
		/* initialize the invariants of the raw-data segment array...*/
		rdSegments[i].adc = adcArray[i];
		rdSegments[i].numChannelsPerFrame = AdcChannelsPerFrame;
		rdSegments[i].numFrames = readSizeFrames;
	}
	rc = rtems_barrier_wait(rdrBarrierId, 5000);/*FIXME--debugging timeouts*/
	if(TestDirective(rc,"rtems_barrier_wait-OrbitController rdr barrier")) {
		FatalErrorHandler(0);
	}
	syslog(LOG_INFO, "OrbitControllerIrq: synchronized with ReaderThreads...\n");

	/* fire up the AdcDataHandler thread...*/
	dataHandlerTID = StartDataHandler(NumReaderThreads);
	rc = rtems_message_queue_ident(RawDataQueueName, RTEMS_LOCAL, &rawDataQID);
	TestDirective(rc, "rtems_message_queue_ident()");

	InitializePSControllers(crateArray);

	/* start ADC acquistion on the "edge" of a rtems_clock_tick()...*/
	rtems_task_wake_after(2);
	/* enable acquire at the ADCz */
	AdcStartAcquisition(adcArray, NumAdcModules);

	/* XXX -- main control loop */
	for(;;loopIterations++) {
		/* Wait for notification of ADC "fifo-half-full" event... */
		rc=rtems_barrier_wait(isrBarrierId,5000);
		if(TestDirective(rc, "rtems_barrier_wait()--isr barrier")) {
			break; /* restart */
		}

		/* disable acquisition at the ADCs... */
		AdcStopAcquisition(adcArray, NumAdcModules);

		/* get raw-data buffers -- readSizeFrames*AdcChannelsPerFrame each...*/
		for(i=0; i<NumReaderThreads; i++) {
			rdSegments[i].buf = (int32_t *)calloc(1, readSizeFrames*AdcChannelsPerFrame*sizeof(int32_t));
			if(rdSegments[i].buf==NULL) {
				syslog(LOG_INFO, "Failed to c'allocate rdSegment buffers: %s", strerror(errno));
				FatalErrorHandler(0);
			}
			/* unleash the ReaderThreads... mmwwaahahaha... */
			rc = rtems_message_queue_send(rdrArray[i]->rawDataQID,&rdSegments[i],sizeof(RawDataSegment));
			TestDirective(rc, "OrbitControllerIrq-->rtems_message_queue_send()-->ReaderThread queue");
		}
		/* block until the ReaderThreads are at their sync-point... */
		rc = rtems_barrier_wait(rdrBarrierId, 5000);/*FIXME--debugging timeouts*/
		if(TestDirective(rc,"rtems_barrier_wait-OrbitController rdr barrier")) {
			break;
		}

		/* reset (clear) each ADC FIFO, then re-enable acquisition... */
		for(i=0; i<NumReaderThreads; i++) {
			ICS110BFIFOReset(adcArray[i]);
		}
		AdcStartAcquisition(adcArray, NumAdcModules);

		/* RE-enable irqs at each ADC and sis3100... */
		for(i=0; i<NumReaderThreads; i++) {
			rc = vme_enable_irq_level(crateArray[i]->fd, adcArray[i]->irqLevel);
			TestDirective(rc, "vme_enable_irq_level()");
			ICS110BInterruptControl(adcArray[i], ICS110B_IRQ_ENABLE);
		}
		/* hand raw-data buffers off to DataHandling thread */
		rc = rtems_message_queue_send(rawDataQID, rdSegments, sizeof(rdSegments));
		if(TestDirective(rc, "OrbitControllerIrq-->rtems_message_queue_send()-->RawDataQueue")<0) {
			break;
		}

		/* chk if the OcmSetpointQueue is available for queries: */
		rc = rtems_message_queue_get_number_pending(OcmSetpointQID,
                                                     &ocmSetpointMQpending);
		//rc= rtems_object_get_classic_name(OcmSetpointQID, &OcmSetpointQName);
		if(rc == RTEMS_SUCCESSFUL) {
			/* If we have PowerSupply setpoints to demux, do that now. */
			if(ocmSetpointMQpending > 0) {
				spMsg msg;
				size_t spBufSize;

				rc = rtems_message_queue_receive(OcmSetpointQID, &msg,
                                &spBufSize, RTEMS_NO_WAIT, RTEMS_NO_TIMEOUT);
				if(TestDirective(rc, "rtems_message_queue_receive")) { break; }
				/* update the global PSController psControllerArray[NumOCM] */
				SimultaneousSetpointUpdate(msg.buf);
				/*for(i=0; i<NumOCM; i++) {
					syslog(LOG_INFO, "msg.buf[%i] = %d\n",i,msg.buf[i]);
				}*/
				free(msg.buf);
			}
		}

	} /* end main acquisition-loop */
	AdcStopAcquisition(adcArray, NumAdcModules);

	/* shut down interrupt generation/handling */
	for(i=0; i<NumAdcModules; i++) {
		rc = vme_disable_irq_level(crateArray[i]->fd, adcArray[i]->irqLevel);
		if(rc) {
			syslog(LOG_INFO, "OrbitControllerIrq: failed to IRQ-mask for Adc[%d], rc=%d\n",crateArray[i]->fd,rc);
		}
		rc = vme_clr_isr(crateArray[i]->fd, adcArray[i]->irqVector);
		if(rc) {
			syslog(LOG_INFO, "OrbitControllerIrq: failed to remove ISR for Adc[%d], rc=%d\n",crateArray[i]->fd,rc);
			//FatalErrorHandler(0);
		}
		free(IsrArgList[i]);
	}
	/* clean up resources */
	ShutdownAdcModules(adcArray, NumAdcModules);
	ShutdownVmeCrates(crateArray, NumVmeCrates);
	for(i=0; i<NumReaderThreads; i++) {
		rtems_message_queue_delete(rdrArray[i]->rawDataQID);
		rtems_task_delete(rdrArray[i]->readerTID);
		free(rdrArray[i]);
	}
	/* clean up DataHandler resources */
	syslog(LOG_INFO, "RawDataQueue: max outstanding msgs=%u\n", getMaxMsgs());
	rtems_task_delete(dataHandlerTID);
	rtems_message_queue_delete(rawDataQID);
	/* clean up self */
	/* FIXME -- rtems_region_delete(rawDataRID);*/
	syslog(LOG_INFO, "OrbitControllerIrq exiting... loop iterations=%lld\n",loopIterations);
	rtems_task_delete(RTEMS_SELF);
}
