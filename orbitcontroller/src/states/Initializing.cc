/*
 * Initializing.cc
 *
 *  Created on: Feb 18, 2009
 *      Author: chabotd
 */

#include <Initializing.h>
#include <OrbitControlException.h>

struct DioConfig {
	uint32_t baseAddr;
	uint32_t crateId;
};

static DioConfig dioconfig[] = {
	{VMIC_2536_DEFAULT_BASE_ADDR,0},
	{VMIC_2536_DEFAULT_BASE_ADDR,1},
	{VMIC_2536_DEFAULT_BASE_ADDR,2},
	{VMIC_2536_DEFAULT_BASE_ADDR,3}
#if 0
	,{VMIC_2536_DEFAULT_BASE_ADDR+0x20,0},
	{VMIC_2536_DEFAULT_BASE_ADDR+0x20,1},
	{VMIC_2536_DEFAULT_BASE_ADDR+0x20,2},
	{VMIC_2536_DEFAULT_BASE_ADDR+0x20,3},
/* this oddball module controls chicane pwr supplies. Must be last in struct!! */
	{VMIC_2536_DEFAULT_BASE_ADDR+0x10,3}
#endif
};

const uint32_t NumDioModules = sizeof(dioconfig)/sizeof(DioConfig);


Initializing* Initializing::instance=0;

Initializing::Initializing()
	: State("Initializing",INITIALIZING),oc(OrbitController::instance) { }

Initializing* Initializing::getInstance() {
	if(instance==0) {
		instance = new Initializing();
	}
	return instance;
}

void Initializing::entryAction() {
	oc->mode = INITIALIZING;
	syslog(LOG_INFO, "OrbitController: entering state %s",toString().c_str());
}

void Initializing::exitAction() {
	syslog(LOG_INFO, "OrbitController: leaving state %s.\n",toString().c_str());
}

void Initializing::stateAction() {
	rtems_status_code rc;
	const double adcSampleRate = 10.15;//kHz. results in 10.05 kHz frame-rate.

	syslog(LOG_INFO, "OrbitController: initializing...\n");
	oc->modeChangePublisher = new Publisher();
	rc = rtems_semaphore_create(rtems_build_name('O','R','B','m'), \
					1 /*initial count*/, RTEMS_BINARY_SEMAPHORE | \
					RTEMS_INHERIT_PRIORITY | RTEMS_PRIORITY, \
					RTEMS_NO_PRIORITY, &oc->mutexId);
	TestDirective(rc, "OrbitController: mutex create failure");
	//create thread and barriers for Rendezvous Pattern
	oc->ocThreadName = rtems_build_name('O','R','B','C');
	rc = rtems_task_create(oc->ocThreadName,
							oc->ocThreadPriority,
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_FLOATING_POINT|RTEMS_LOCAL,
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&oc->ocTID);
	TestDirective(rc,"OrbitController: task_create failure");
	oc->isrBarrierName = rtems_build_name('i','s','r','B');
	rc = rtems_barrier_create(oc->isrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&oc->isrBarrierId);
	TestDirective(rc,"OrbitController: ISR barrier_create() failure");
	oc->rdrBarrierName = rtems_build_name('a','d','c','B');
	rc = rtems_barrier_create(oc->rdrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&oc->rdrBarrierId);
	TestDirective(rc,"OrbitController: RDR barrier_create() failure");
	oc->spQueueName = rtems_build_name('S','P','Q','1');
	rc = rtems_message_queue_create(oc->spQueueName,
									NumOcm*10/*FIXME--(see ntbk#2 pg22) max msgs in queue*/,
									sizeof(OrbitController::SetpointMsg)/*max msg size (bytes)*/,
									RTEMS_LOCAL|RTEMS_FIFO,
									&oc->spQueueId);
	TestDirective(rc, "OrbitController: power-supply msg_queue_create() failure");
	oc->bpmThreadName = rtems_build_name('B','P','M','t');
	rc = rtems_task_create(oc->bpmThreadName,
								oc->bpmThreadPriority,
								RTEMS_MINIMUM_STACK_SIZE*8,
								RTEMS_FLOATING_POINT|RTEMS_LOCAL,
								RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
								&oc->bpmTID);
	TestDirective(rc,"BpmController: thread_create failure");
	oc->adcQueueName = rtems_build_name('B','P','M','q');
	rc = rtems_message_queue_create(oc->adcQueueName,
									oc->adcMaxMsgs/*max msgs in queue*/,
									oc->adcMsgSize/*max msg size (bytes)*/,
									RTEMS_LOCAL|RTEMS_FIFO,
									&oc->adcQueueId);
	TestDirective(rc,"BpmController: msg_q_create failure");
	oc->bpmEventPublisher = new Publisher();
	oc->stateQueueName = rtems_build_name('S','T','A','Q');
	rc = rtems_message_queue_create(oc->stateQueueName,
										5/* FIXME -- max msgs in queue*/,
										sizeof(State*)/*max msg size (bytes)*/,
										RTEMS_LOCAL|RTEMS_FIFO,
										&oc->stateQueueId);
	TestDirective(rc, "OrbitController: State msg_q_create failure");
	oc->ocmThreadName = rtems_build_name('O','C','M','t');
	rc = rtems_task_create(oc->ocmThreadName,
								oc->ocmThreadPriority,
								RTEMS_MINIMUM_STACK_SIZE*8,
								RTEMS_FLOATING_POINT|RTEMS_LOCAL,
								RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
								&oc->ocmTID);
	TestDirective(rc,"OcmController: thread create failure");

	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &oc->rtemsTicksPerSecond);

	//initialize hardware: VME crates, ADC, and DIO modules
	for(uint32_t i=0; i<NumVmeCrates; i++) {
		oc->crateArray.push_back(new VmeCrate(i));
	}
	for(uint32_t i=0; i<NumAdcModules; i++) {
		oc->adcArray.push_back(new Ics110blModule(oc->crateArray[i],ICS110B_DEFAULT_BASEADDR));
		oc->adcArray[i]->initialize(adcSampleRate,INTERNAL_CLOCK,ICS110B_INTERNAL);
		syslog(LOG_INFO,"%s[%u]: framerate=%g kHz, ch/Frame = %d\n",
								oc->adcArray[i]->getType(),i,
								oc->adcArray[i]->getFrameRate(),
								oc->adcArray[i]->getChannelsPerFrame());
	}
	oc->adcFrameRateSetpoint = adcSampleRate;
	oc->adcFrameRateFeedback = oc->adcArray[0]->getFrameRate();
	//set up our buffer pool
	uint32_t numFrames = HALF_FIFO_LENGTH/ICS110B_DEFAULT_CHANNELS_PER_FRAME; //max of 512 frames
	uint32_t numChannelsPerFrame = ICS110B_DEFAULT_CHANNELS_PER_FRAME; //32 channels/frame
	//bufLength is approx. 3 MB. See Notebook #2, pg 25-26 for logic.
	uint32_t bufLength = NumAdcModules*numChannelsPerFrame*numFrames*(oc->adcMaxMsgs+2)*sizeof(int32_t);
	oc->bufPool = new int32_t[bufLength];
	oc->bufPoolName = rtems_build_name('B','U','F','R');
	rc = rtems_region_create(oc->bufPoolName,oc->bufPool,bufLength,
								numChannelsPerFrame*sizeof(int32_t),
								RTEMS_LOCAL|RTEMS_FIFO,&oc->bufPoolId);
	TestDirective(rc,"OrbitController: failure initializing buffer pool Region");

	for(uint32_t i=0; i<NumDioModules; i++) {
		oc->dioArray.push_back(new Vmic2536Module(oc->crateArray[dioconfig[i].crateId],
									dioconfig[i].baseAddr));
		oc->dioArray[i]->initialize();
		//FIXME -- when all OCM are "fast" do we need 4 or 8 PowerSupplyBulk objects ???
		oc->psbArray.push_back(new PowerSupplyBulk(oc->dioArray[i]));
	}
	for(uint32_t i=0; i<NumAdcModules; i++) {
		oc->isrArray.push_back(new AdcIsr(oc->adcArray[i],oc->isrBarrierId));
		oc->rdrArray.push_back(new AdcReader(oc->adcArray[i], oc->rdrBarrierId));
		oc->rdrArray[i]->start(0);
	}
	oc->rendezvousWithAdcReaders();
	oc->initialized = true;
	syslog(LOG_INFO, "OrbitController: initialized and synchronized with AdcReaders...\n");
}

