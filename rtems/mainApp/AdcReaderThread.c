#include <stdlib.h>
#include <syslog.h>

#include <rtems/error.h>

#include <utils.h>
#include <ics110bl.h>
#include <DaqController.h>
#include <dataDefs.h>
#include <AdcReaderThread.h>


static void RegisterAtRendezvousPoint(ReaderThreadArg *argp) {
	rtems_status_code rc;

	rc = rtems_event_send(argp->controllerTID, argp->syncEvent);
	TestDirective(rc, "ReaderThread-->RegisterAtRendezvousPoint-->rtems_event_send()");
}

rtems_task ReaderThread(rtems_task_argument arg) {
	ReaderThreadArg *argp = (ReaderThreadArg *)arg;
	RawDataSegment ds = {0};
	size_t dsSize;
	rtems_status_code rc;
	uint32_t wordsRequested;
	uint32_t wordsRead;

	/* let the controller know that we're good to go... */
	RegisterAtRendezvousPoint(argp);
	for(;;) {
		uint16_t adcStatus = 0;

		/* block for the controller's msg... */
		rc = rtems_message_queue_receive(argp->rawDataQID,&ds,&dsSize,RTEMS_WAIT,RTEMS_NO_TIMEOUT);
		if(TestDirective(rc, "ReaderThread-->rtems_message_queue_receive()")) {
			break;
		}
		/* chk for FIFO-FULL or FIFO-not-1/2-FULL conditions */
		ICS110BGetStatus(argp->adc, &adcStatus);
		if((adcStatus&ICS110B_FIFO_FULL) || !(adcStatus&ICS110B_FIFO_HALF_FULL)) {
			syslog(LOG_INFO, "Adc[%d] (pre-BLT) has abnormal status=%#hx",argp->adc->crate->id, adcStatus);
			break;
		}

		/* get the data... */
		wordsRequested = ds.numFrames*ds.numChannelsPerFrame;
		int readStatus = ICS110BFifoRead(argp->adc, (uint32_t *)ds.buf, wordsRequested, &wordsRead);

		/* chk for FIFO-1/2-FULL (still ?!?) or FIFO-empty conditions */
		ICS110BGetStatus(argp->adc, &adcStatus);
		if((adcStatus&ICS110B_FIFO_HALF_FULL) || (adcStatus&ICS110B_FIFO_EMPTY)) {
			syslog(LOG_INFO, "Adc[%d] (post-BLT) has abnormal status=%#hx",argp->adc->crate->id, adcStatus);
			//break;
		}

		if(readStatus) {
			syslog(LOG_INFO, "Adc[%d] BLT problem: status = %d", argp->adc->crate->id, readStatus);
			//FatalErrorHandler(0);
		}
		if(wordsRead != wordsRequested) {
			/* FIXME -- any mods to a DataSegment won't be visible to the DaqController;
			 * we only have a *copy* of its DataSegment... :-(
			 */
			ds.numFrames = wordsRead/ds.numChannelsPerFrame;
			syslog(LOG_INFO, "Adc[%d]: asked for %u words, but read only %u\n",
					argp->adc->crate->id, wordsRequested, wordsRead);
		}
		/* let the controller know that our task is done...*/
		RegisterAtRendezvousPoint(argp);
	}

	/* clean up resources: DaqController will handle this... */
	syslog(LOG_INFO, "Adc[%d]: deleting self", argp->adc->crate->id);
	rtems_task_delete(RTEMS_SELF);
}

ReaderThreadArg* startReaderThread(VmeModule *mod, rtems_event_set syncEvent) {
	rtems_id tid;
	rtems_id qid;
	rtems_status_code rc;
	rtems_task_priority pri = 0;
	rtems_id daqControllerTID;
	ReaderThreadArg *arg = NULL;
	static int i = 0;

	rtems_task_ident(RTEMS_SELF, RTEMS_LOCAL, &daqControllerTID);

	pri = DefaultPriority+1; /* NOTE: RTEMS Classic api priority range from 1(highest) to 255(lowest) */

	/* these threads are lower priority than their master (DAQController)... */
	rc = rtems_task_create(rtems_build_name('R','D', 'R',(char)(i+48)),
							pri,
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&tid);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to create task %d: %s\n",i,rtems_status_text(rc));
		FatalErrorHandler(0);
	}

	rc = rtems_message_queue_create(rtems_build_name('R','D','Q',(char)(i+48)),
									1/*max queue size*/,
									sizeof(RawDataSegment)/*max msg size*/,
									RTEMS_LOCAL|RTEMS_FIFO/*attributes*/,
									&qid);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to create msg queue %d: %s\n",i,rtems_status_text(rc));
		FatalErrorHandler(0);
	}
	arg = (ReaderThreadArg *)calloc(1,sizeof(ReaderThreadArg));
	if(arg==NULL) {
		syslog(LOG_INFO, "Failed to allocate ReaderThreadArg #%d",i);
		FatalErrorHandler(0);
	}
	arg->readerTID = tid;
	arg->controllerTID = daqControllerTID;
	arg->syncEvent = syncEvent;
	arg->rawDataQID = qid;
	arg->adc = mod;

	rc = rtems_task_start(tid, ReaderThread, (rtems_task_argument)arg);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Failed to start task %d: %s\n",i,rtems_status_text(rc));
		FatalErrorHandler(0);
	}
	syslog(LOG_INFO, "Created ReaderThread %d with priority %d\n",i,pri);
	i++;

	return arg;
}
