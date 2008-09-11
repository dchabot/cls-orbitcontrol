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

static rtems_name DspQName = rtems_build_name('D','S','P','Q');

typedef struct {
	uint32_t numChannelsPerFrame;
	uint32_t numFrames;
	uint32_t *buf;
}RawDataSegment;

typedef struct {
	rtems_id controllerTID;
	rtems_id rawDataQID;
	VmeModule *adc;
}ReaderThreadArg;

typedef void (*Callback)(void *arg);

typedef struct {
	int crateNum;
	int numFrames;
	unsigned int *buf;
	Callback cb;
	void *cbArg;
}DspMsg;

static rtems_task DspThread(rtems_task_argument arg) {
	uint32_t numAdcs = arg;
	rtems_status_code rc;
	rtems_id dspQID;
	const uint32_t MaxDspMsgs = 10;
	DspMsg DspMsgs[numAdcs];
	size_t msgSize;
	int i;
	
	rc = rtems_message_queue_create(DspQName,numAdcs*MaxDspMsgs,sizeof(DspMsg),RTEMS_LOCAL|RTEMS_FIFO,&dspQID);
	TestDirective(rc, "rtems_message_queue_create()");
	
	for(;;) {
		/* gather raw-data sent by the ReaderThreads... */
		for(i=0; i<numAdcs; i++) {
			rc = rtems_message_queue_receive(dspQID,&DspMsgs[i],&msgSize,RTEMS_WAIT,0);
			TestDirective(rc, "rtems_message_queue_receive()");
		}
		/* Perform numeric operations... */
		for(i=0; i<numAdcs; i++) {
			DspMsgs[i].cb((void *)&DspMsgs[i]);
		}
		/* add result to the outbound queue... */
	}
}

static rtems_task ReaderThread(rtems_task_argument arg) {
	ReaderThreadArg *argp = (ReaderThreadArg *)arg;
	rtems_event_set mySynchEvent = (1<<(15+(argp->adc->crate->id)));
	RawDataSegment msg = {0};
	size_t msgSize;
	rtems_status_code rc;
	uint32_t wordsRead = 0;
	
	/* let the controller know that we're good to go... */
	rc = rtems_event_send(argp->controllerTID, mySynchEvent);
	TestDirective(rc, "rtems_event_send()");
	for(;;) {
		/* block for the controller's msg... */
		rc = rtems_message_queue_receive(argp->rawDataQID,&msg,&msgSize,RTEMS_WAIT,0/*timeout*/);
		TestDirective(rc, "rtems_message_queue_receive()");
		/* get the data... */
		ICS110BFifoRead(argp->adc, msg.buf, msg.numFrames, &wordsRead);
		/* forward info to the DataHandling layer... (DSP,transmission,etc) */
		//eg: rc = rtems_message_queue_send(dataQID, dataMsg, sizeof(dataMsg));
		/* let the controller know that our task is done...*/
		rc = rtems_event_send(argp->controllerTID, mySynchEvent);
		TestDirective(rc, "rtems_event_send()");
		//rtems_task_suspend(RTEMS_SELF);
	}
	
	/* clean up resources */
	rtems_message_queue_delete(argp->rawDataQID);
	free(argp);
	rtems_task_delete(RTEMS_SELF);
}

/*static void deleteReaderThreads(int numThreads) {
	int i;
	
	for(i=0; i<numThreads; i++) {
		//rtems_task_delete()
	}
}*/

static void startReaderThreads(int numThreads, ReaderThreadArg** args) {
	rtems_status_code rc;
	rtems_task_priority pri = 0;
	int i;
	static rtems_id DAQControllerTID;

	rtems_task_ident(RTEMS_SELF, RTEMS_LOCAL, &DAQControllerTID);
	
	/* this directive is used to set AND get task priority info... */
	rc = rtems_task_set_priority(RTEMS_SELF, RTEMS_CURRENT_PRIORITY, &pri);
	TestDirective(rc, "rtems_task_set_priority()");
	pri += 1;
	for(i=0; i<numThreads; i++) {
		rtems_id tid;
		rtems_id qid;
		
		/* these threads are lower priority than their master (DAQController)... */
		rc = rtems_task_create(rtems_build_name('R','D', 'R',(char)i),
								pri,
								RTEMS_MINIMUM_STACK_SIZE*4,
								RTEMS_NO_FLOATING_POINT|RTEMS_LOCAL,
								RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
								&tid);
		if(rc != RTEMS_SUCCESSFUL) {
			syslog(LOG_INFO, "Failed to create task %d: %s\n",i,rtems_status_text(rc));
			FatalErrorHandler(0);
		}
		
		rc = rtems_message_queue_create(rtems_build_name('R','D','Q',(char)i),
										1/*max queue size*/,
										sizeof(RawDataSegment)/*max msg size*/,
										RTEMS_LOCAL|RTEMS_FIFO/*attributes*/,
										&qid);
		if(rc != RTEMS_SUCCESSFUL) {
			syslog(LOG_INFO, "Failed to create msg queue %d: %s\n",i,rtems_status_text(rc));
			FatalErrorHandler(0);
		}
		ReaderThreadArg *arg = (ReaderThreadArg *)calloc(1,sizeof(ReaderThreadArg));
		arg->controllerTID = DAQControllerTID;
		arg->rawDataQID = qid;
		arg->adc = AdcModules[i];
		/* the DAQController needs to retain this info... */
		args[i] = arg;
		
		rc = rtems_task_start(tid, ReaderThread, (rtems_task_argument)arg);
		if(rc != RTEMS_SUCCESSFUL) {
			syslog(LOG_INFO, "Failed to start task %d: %s\n",i,rtems_status_text(rc));
			FatalErrorHandler(0);
		}
		syslog(LOG_INFO, "Created ReaderThread %d with priority %d\n",i,pri);
	}
}

static FILE* getOutputFile(const char* filename) {
	char basepath[64] = "/home/tmp/rtems-rawdata/";
	FILE *f;
	char *outfile;
	
	/* form output filename */
	outfile = strncat(basepath, filename, sizeof(basepath));
	syslog(LOG_INFO, "outfile: %s\n", outfile);

	f = fopen(outfile, "w");
	if(f == NULL) {
	    syslog(LOG_CRIT, "fopen dump_data: %s", strerror(errno));
	    FatalErrorHandler(0);
	}
	
	return f;
}

static void
writeToFile(FILE* f, void* buf, size_t size) {
	int n=0;

	n = fwrite(buf, 1, size, f);
	fflush(f);
	if(n != size) {
	    syslog(LOG_INFO, "Error: wrote only %d words, not %lu\n",n,size);
	}
}

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

void StartPeriodicTest(void) {
	void Load_SIS1100_Driver(int);
	
	extern double tscTicksPerSecond;
	const double adcFrameRate_kHz = 10.0; // in kHz
	const double HzPerkHz = 1000.0;
	double adcFramesPerTick;
	const int adcChannelsPerFrame = 32;
	const int adcBytesPerChannel = 4;
	const int adcMaxChannels = 16256;
	rtems_interval rtemsTicksPerSecond;
	uint64_t then;
	uint64_t now = 0;
	int i,j,k,x;
	//these "sizes" are in "word-sizes" (4 bytes/word)
	int readoutSizes[] = {4,16,64,256,1024,2048};//,4096,8192,16256};
	int numTestIterations = sizeof(readoutSizes)/sizeof(readoutSizes[0]);
	const int numMeasurements = 1000;
	const int numReaderThreads = numVmeCrates;
	RawDataSegment msgs[numReaderThreads];
	rtems_status_code rc;
	static rtems_event_set synchEvents;
	
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);
	
	syslog(LOG_INFO, "Initializing hardware...\n");
	InitializeVmeCrates();
	InitializeAdcModules(adcFrameRate_kHz, adcChannelsPerFrame);
	syslog(LOG_INFO, "DONE\n");
	
		
	ReaderThreadArg **rdrArgs = (ReaderThreadArg **)calloc(numReaderThreads, sizeof(ReaderThreadArg*));
	startReaderThreads(numReaderThreads, rdrArgs);
	for(i=0; i<numReaderThreads; i++) {
		synchEvents |= (1<<(15+i));
	}
	rtems_event_set eventsIn=0;
	rc = rtems_event_receive(synchEvents, RTEMS_EVENT_ALL|RTEMS_WAIT, 0, &eventsIn);
	TestDirective(rc, "rtems_event_receive()");
	syslog(LOG_INFO, "Synchronized with ReaderThreads...\n");
	
	/* @ 10 kHz the inter-frame period is 100 microsec, or 10 Frames per Tick (1 tick == 1ms) */
	adcFramesPerTick = (adcFrameRate_kHz*HzPerkHz)/((double)rtemsTicksPerSecond);
	for(i = 0; i<numTestIterations; i++) {
		double sum = 0.0;
		double sumSqrs = 0.0;
		int dataSize = readoutSizes[i];
		int numDaqPeriods = 0;
		int daqTicks = (adcMaxChannels/(adcChannelsPerFrame*adcFramesPerTick));
		//data = (unsigned int *)calloc(1, dataSize*adcBytesPerChannel);
		FILE *fps[numReaderThreads];
		char filename[128] = {0};
		
		for(x=0; x<numReaderThreads; x++) {
			msgs[x].numChannelsPerFrame = adcChannelsPerFrame;
			msgs[x].numFrames = dataSize;
			msgs[x].buf = (uint32_t *)calloc(1,dataSize*adcBytesPerChannel);
		}
		
		if(dataSize*numMeasurements < adcMaxChannels) {
			numDaqPeriods = 1;
		}
		else {
			numDaqPeriods = (int)ceil((((double)dataSize*(double)numMeasurements)/((double)adcMaxChannels)));
		}

		syslog(LOG_INFO, "dataSize=%d [channels], numDaqPeriods=%d, daqTicks=%d\n",dataSize,numDaqPeriods,daqTicks);
		
		/*for(x=0; x<NumReaderThreads; x++) {
			sprintf(filename, "Card%d-%d-channels.dat", x, dataSize);
			fps[x] = getOutputFile(filename);
		}*/
		
		double maxTime = 0;
		for(j=0; j<numDaqPeriods; j++) {
			
			AdcStartAcquisition();
			rtems_task_wake_after(daqTicks+1); // let frames accumulate
			AdcStopAcquisition();
			
			for(x=0; x<numAdcModules; x++) {
				/* check for full-fifo condition... */
				if(ICS110BIsFull(AdcModules[x])) {
					int tmp;
					double actualRate = 0.0;
					
					syslog(LOG_INFO, "ADC[%d] is full: reconfiguring...",x);
					AdcStopAcquisition();
					for(tmp=0; tmp<numAdcModules; tmp++) {
						InitICS110BL(AdcModules[tmp], adcFrameRate_kHz, &actualRate,
								INTERNAL_CLOCK, ICS110B_INTERNAL, 
								adcChannelsPerFrame);
					}
					syslog(LOG_INFO, "DONE\n");
					
					AdcStartAcquisition();
					rtems_task_wake_after(daqTicks+1); // let frames accumulate
					AdcStopAcquisition();
					break;
				}
			}
		
			for(k=0; k<(numMeasurements/numDaqPeriods); k++) {
				uint64_t tmp;
				//uint32_t count;
				
				rdtscll(then);
				/* release (wake up) the ReaderThreads... */
				for(x=0; x<numReaderThreads; x++) {
					rc = rtems_message_queue_send(rdrArgs[x]->rawDataQID,&msgs[x],sizeof(msgs[x]));
					TestDirective(rc, "rtems_message_queue_send()");
				}
				/* block until the ReaderThreads are at their sync-point... */
				rc = rtems_event_receive(synchEvents, RTEMS_EVENT_ALL|RTEMS_WAIT, 0, &eventsIn);
				rdtscll(now);
				TestDirective(rc, "rtems_event_receive()");
				tmp = (now-then);
				sum += (double)tmp;
				sumSqrs += (double)(tmp*tmp);
				if(tmp > maxTime) {
					maxTime = (double)tmp;
				}
				/* send raw-data to host for examination */
				/*for(x=0; x<NumReaderThreads; x++) {
					writeToFile(fps[x],(void *)msgs[x].buf,
								(size_t)(msgs[x].numFrames*sizeof(uint32_t)));
				}*/
			}
		}
		double stdDev = (1.0/(double)(numMeasurements))*sumSqrs - (1.0/(double)(numMeasurements*numMeasurements))*(sum*sum);
		stdDev = sqrt(stdDev);
		stdDev /= tscTicksPerSecond;
		double avg = sum/((double)numMeasurements);
		avg /= tscTicksPerSecond;
		maxTime /= tscTicksPerSecond;
		
		syslog(LOG_INFO, "%d words/BLT, Avg time = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",dataSize,avg,stdDev,maxTime);
		for(x=0; x<numReaderThreads; x++) {
			/*fflush(fps[x]);
			fclose(fps[x]);*/
			free(msgs[x].buf);
		}
	}
	ShutdownVmeCrates();
	syslog(LOG_INFO, "StartPeriodicTest() exiting...\n");
}
