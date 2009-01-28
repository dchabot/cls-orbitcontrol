/*
 * OrbitController.cpp
 *
 *  Created on: Dec 11, 2008
 *      Author: chabotd
 */

#include <OrbitController.h>
#include <OrbitControlException.h>
#include <rtems/error.h>
#include <sis1100_api.h>
#include <syslog.h>
#include <ics110bl.h>
#include <vmic2536.h>

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


/* THE singleton instance of this class */
OrbitController* OrbitController::instance = 0;

OrbitController::OrbitController() :
	//ctor-initializer list
	mcCallback(0), mcCallbackArg(0),
	mutexId(0),ocTID(0),ocThreadName(0),ocThreadArg(0),
	ocThreadPriority(OrbitControllerPriority),
	rtemsTicksPerSecond(0),adcFramesPerTick(0),
	adcFrameRateSetpoint(0),adcFrameRateFeedback(0),
	isrBarrierId(0),isrBarrierName(0),
	rdrBarrierId(0),rdrBarrierName(0),
	initialized(false),mode(ASSISTED),
	spQueueId(0),spQueueName(0),
	samplesPerAvg(5000),
	bpmMsgSize(sizeof(rdSegments)),bpmMaxMsgs(10),
	bpmTID(0),bpmThreadName(0),
	bpmThreadArg(0),bpmThreadPriority(OrbitControllerPriority+3),
	bpmQueueId(0),bpmQueueName(0),
	bpmCB(0),bpmCBArg(0)
{ }

OrbitController::~OrbitController() {
	syslog(LOG_INFO, "Destroying OrbitController instance!!\n");
	stopAdcAcquisition();
	resetAdcFifos();
	if(mutexId) { rtems_semaphore_delete(mutexId); }
	if(ocTID) { rtems_task_delete(ocTID); }
	if(spQueueId) { rtems_message_queue_delete(spQueueId); }
	if(isrBarrierId) { rtems_barrier_delete(isrBarrierId); }
	if(rdrBarrierId) { rtems_barrier_delete(rdrBarrierId); }
	if(bpmTID) { rtems_task_delete(bpmTID); }
	if(bpmQueueId) { rtems_message_queue_delete(bpmQueueId); }
	/* XXX -- container.clear() canNOT call item dtors if items are pointers!! */
	map<string,Bpm*>::iterator bpmit;
	for(bpmit=bpmMap.begin(); bpmit!=bpmMap.end(); bpmit++) { delete bpmit->second; }
	bpmMap.clear();
	map<string,Ocm*>::iterator ocmit;
	for(ocmit=ocmMap.begin(); ocmit!=ocmMap.end(); ocmit++) { delete ocmit->second; }
	ocmMap.clear();
	for(uint32_t i=0; i<isrArray.size(); i++) { delete isrArray[i]; }
	isrArray.clear();
	for(uint32_t i=0; i<rdrArray.size(); i++) { delete rdrArray[i]; }
	rdrArray.clear();
	for(uint32_t i=0; i<adcArray.size(); i++) { delete adcArray[i]; }
	adcArray.clear();
	for(uint32_t i=0; i<dioArray.size(); i++) { delete dioArray[i]; }
	dioArray.clear();
	for(uint32_t i=0; i<psbArray.size(); i++) { delete psbArray[i]; }
	psbArray.clear();
	for(uint32_t i=0; i<crateArray.size(); i++) { delete crateArray[i]; }
	crateArray.clear();
	instance = 0;
}

OrbitController* OrbitController::getInstance() {
	//FIXME -- not thread-safe!!
	if(instance==0) {
		instance = new OrbitController();
	}
	return instance;
}

void OrbitController::destroyInstance() { delete instance; }

void OrbitController::initialize(const double adcSampleRate) {
	rtems_status_code rc;
	syslog(LOG_INFO, "OrbitController: initializing...\n");
	rc = rtems_semaphore_create(rtems_build_name('O','R','B','m'), \
					1 /*initial count*/, RTEMS_BINARY_SEMAPHORE | \
					RTEMS_INHERIT_PRIORITY | RTEMS_PRIORITY, \
					RTEMS_NO_PRIORITY, &mutexId);
	TestDirective(rc, "OrbitController: mutex create failure");
	//create thread and barriers for Rendezvous Pattern
	ocThreadName = rtems_build_name('O','R','B','C');
	rc = rtems_task_create(ocThreadName,
							ocThreadPriority,
							RTEMS_MINIMUM_STACK_SIZE*8,
							RTEMS_FLOATING_POINT|RTEMS_LOCAL,
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&ocTID);
	TestDirective(rc,"OrbitController: task_create failure");
	isrBarrierName = rtems_build_name('i','s','r','B');
	rc = rtems_barrier_create(isrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&isrBarrierId);
	TestDirective(rc,"OrbitController: ISR barrier_create() failure");
	rdrBarrierName = rtems_build_name('a','d','c','B');
	rc = rtems_barrier_create(rdrBarrierName,
								RTEMS_BARRIER_AUTOMATIC_RELEASE|RTEMS_LOCAL,
								NumAdcModules+1,
								&rdrBarrierId);
	TestDirective(rc,"OrbitController: RDR barrier_create() failure");
	spQueueName = rtems_build_name('S','P','Q','1');
	rc = rtems_message_queue_create(spQueueName,
									NumOcm+1/*max msgs in queue*/,
									sizeof(SetpointMsg)/*max msg size (bytes)*/,
									RTEMS_LOCAL|RTEMS_FIFO,
									&spQueueId);
	TestDirective(rc, "OrbitController: power-supply msg_queue_create() failure");
	bpmThreadName = rtems_build_name('B','P','M','t');
	rc = rtems_task_create(bpmThreadName,
								bpmThreadPriority,
								RTEMS_MINIMUM_STACK_SIZE*8,
								RTEMS_FLOATING_POINT|RTEMS_LOCAL,
								RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
								&bpmTID);
	TestDirective(rc,"BpmController: thread_create failure");
	bpmQueueName = rtems_build_name('B','P','M','q');
	rc = rtems_message_queue_create(bpmQueueName,
									bpmMaxMsgs/*FIXME -- max msgs in queue*/,
									bpmMsgSize/*max msg size (bytes)*/,
									RTEMS_LOCAL|RTEMS_FIFO,
									&bpmQueueId);

	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &rtemsTicksPerSecond);
	//initialize hardware: VME crates, ADC, and DIO modules
	for(uint32_t i=0; i<NumVmeCrates; i++) {
		crateArray.push_back(new VmeCrate(i));
	}
	for(uint32_t i=0; i<NumAdcModules; i++) {
		adcArray.push_back(new Ics110blModule(crateArray[i],ICS110B_DEFAULT_BASEADDR));
		adcArray[i]->initialize(adcSampleRate,INTERNAL_CLOCK,ICS110B_INTERNAL);
		syslog(LOG_INFO,"%s[%u]: framerate=%g kHz, ch/Frame = %d\n",
									adcArray[i]->getType(),i,
									adcArray[i]->getFrameRate(),
									adcArray[i]->getChannelsPerFrame());
	}
	adcFrameRateSetpoint = adcSampleRate;
	adcFrameRateFeedback = adcArray[0]->getFrameRate();
	for(uint32_t i=0; i<NumDioModules; i++) {
		dioArray.push_back(new Vmic2536Module(crateArray[dioconfig[i].crateId],
												dioconfig[i].baseAddr));
		dioArray[i]->initialize();
		//FIXME -- when all OCM are "fast" do we need 4 or 8 PowerSupplyBulk objects ???
		psbArray.push_back(new PowerSupplyBulk(dioArray[i],30));
	}
	for(uint32_t i=0; i<NumAdcModules; i++) {
		isrArray.push_back(new AdcIsr(adcArray[i],isrBarrierId));
		rdrArray.push_back(new AdcReader(adcArray[i], rdrBarrierId));
		rdrArray[i]->start(0);
	}
	rendezvousWithAdcReaders();
	initialized = true;
	mode=STANDBY;
	syslog(LOG_INFO, "OrbitController: initialized and synchronized with AdcReaders...\n");
}

void OrbitController::start(rtems_task_argument ocThreadArg,
							rtems_task_argument bpmThreadArg) {
	if(initialized==false) {
		initialize();
	}
	//fire up the OrbitController and BpmController threads:
	this->ocThreadArg = ocThreadArg;
	rtems_status_code rc = rtems_task_start(ocTID,ocThreadStart,(rtems_task_argument)this);
	TestDirective(rc,"Failed to start OrbitController thread");

	this->bpmThreadArg = bpmThreadArg;
	rc = rtems_task_start(bpmTID,bpmThreadStart,(rtems_task_argument)this);
	TestDirective(rc, "Failed to start BpmController thread");
}

OrbitControllerMode OrbitController::getMode() {
	OrbitControllerMode m;
	lock();
	m=mode;
	unlock();
	return m;
}

void OrbitController::setMode(OrbitControllerMode mode) {
	lock();
	this->mode = mode;
	unlock();
	if(mcCallback) {
		this->mcCallback(mcCallbackArg);
	}
}

//FIXME -- refactor the BPM and OCM callback mechanisms using standard
// Observer && Command patterns (i.e. vectors of observers and their cmds,publish,etc)
void OrbitController::setModeChangeCallback(OrbitControllerModeChangeCallback cb, void* cbArg) {
	mcCallback = cb;
	mcCallbackArg = cbArg;
}
/********************** OcmController public interface *********************/
Ocm* OrbitController::registerOcm(const string& str,
									uint32_t crateId,
									uint32_t vmeAddr,
									uint8_t ch) {
	Ocm *ocm = NULL;
	//find the matching DIO module controlling this OCM:
	for(uint32_t i=0; i<dioArray.size(); i++) {
		if(crateId==dioArray[i]->getCrate()->getId()
				&& vmeAddr==dioArray[i]->getVmeBaseAddr()) {
			ocm = new Ocm(str,dioArray[i],ch);
			break;
		}
	}
	if(ocm != NULL) {
		//stuff this OCM into our ocmMap:
		pair<map<string,Ocm*>::iterator,bool> ret;
		ret = ocmMap.insert(pair<string,Ocm*>(ocm->getId(),ocm));
		if(ret.second != false) {
			syslog(LOG_INFO, "OcmController: added %s to ocmMap.\n",ocm->getId().c_str());
		}
		else {
			syslog(LOG_INFO, "Failed to insert %s in ocmMap; it already exists!\n",
									ocm->getId().c_str());
		}
	}
	else {
		syslog(LOG_INFO, "OcmController: couldn't match DIO module with Ocm id=%s addr=%#x!!\n",
						  str.c_str(),vmeAddr);
	}
	return ocm;
}

void OrbitController::unregisterOcm(Ocm* ocm) {
	map<string,Ocm*>::iterator it;
	it = ocmMap.find(ocm->getId());
	if(it != ocmMap.end()) {
		ocmMap.erase(it);
		delete ocm;
	}
	else {
		syslog(LOG_INFO, "Failed to remove %s in ocmMap; it doesn't exist!\n",
				ocm->getId().c_str());
	}
}

Ocm* OrbitController::getOcmById(const string& id) {
	map<string,Ocm*>::iterator it;
	it = ocmMap.find(id);
	if(it != ocmMap.end()) { return it->second; }
	else { return NULL; }
}

void OrbitController::showAllOcms() {
	map<string,Ocm*>::iterator it;
	for(it=ocmMap.begin(); it!=ocmMap.end(); it++) {
		syslog(LOG_INFO, "%s: setpoint=%i\tinCorrection=%s\n",
				it->second->getId().c_str(),
				it->second->getSetpoint(),
				it->second->isEnabled()?"true":"false");
	}
}

void OrbitController::setOcmSetpoint(Ocm* ocm, int32_t val) {
	SetpointMsg msg(ocm, val);
	rtems_status_code rc = rtems_message_queue_send(spQueueId,(void*)&msg,sizeof(msg));
	TestDirective(rc,"OcmController: msg_q_send failure");
}

//XXX -- vmat & hmat are populated in Row-Major order (i.e - "ith row, jth column")
void OrbitController::setVerticalResponseMatrix(double v[NumVOcm*NumBpm]) {
	uint32_t row,col;
	lock();
	for(col=0; col<NumBpm; col++) {
		for(row=0; row<NumVOcm; row++) {
			vmat[row][col] = v[col*NumOcm+row];
		}
	}
	unlock();
}

void OrbitController::setHorizontalResponseMatrix(double h[NumHOcm*NumBpm]) {
	uint32_t row,col;
	lock();
	for(col=0; col<NumBpm; col++) {
		for(row=0; row<NumHOcm; row++) {
			hmat[row][col] = h[col*NumOcm+row];
		}
	}
	unlock();
}

void OrbitController::setDispersionVector(double d[NumBpm]) {
	lock();
	for(uint32_t col=0; col<NumBpm; col++) {
		dmat[col] = d[col];
	}
	unlock();
}

/*********************** BpmController public interface ********************/
void OrbitController::registerBpm(Bpm *bpm) {
	pair<map<string,Bpm*>::iterator,bool> ret;
	ret = bpmMap.insert(pair<string,Bpm*>(bpm->getId(),bpm));
	if(ret.second == false) {
		syslog(LOG_INFO, "Failed to insert %s in bpmMap; it already exists!\n",
							bpm->getId().c_str());
	}
}

void OrbitController::unregisterBpm(Bpm *bpm) {
	map<string,Bpm*>::iterator it;
	it = bpmMap.find(bpm->getId());
	if(it != bpmMap.end()) {
		bpmMap.erase(it);
	}
	else {
		syslog(LOG_INFO, "Failed to remove %s in bpmMap; it doesn't exist!\n");
	}
}

Bpm* OrbitController::getBpmById(const string& id) {
	map<string,Bpm*>::iterator it;
	it = bpmMap.find(id);
	if(it != bpmMap.end()) { return it->second; }
	else { return NULL; }
}

void OrbitController::showAllBpms() {
	map<string,Bpm*>::iterator it;
	for(it=bpmMap.begin(); it!=bpmMap.end(); it++) {
		syslog(LOG_INFO, "%s: x=%.3e\ty=%.3e\tisEnabled=%s\n",it->second->getId().c_str(),
												it->second->getX(),
												it->second->getY(),
												it->second->isEnabled()?"yes":"no");
	}
}

void OrbitController::setBpmValueChangeCallback(BpmValueChangeCallback cb, void* cbArg) {
	bpmCB=cb;
	bpmCBArg=cbArg;
}

/*********************** private methods *****************************************/
void OrbitController::lock() {
	rtems_status_code rc = rtems_semaphore_obtain(mutexId,RTEMS_WAIT,RTEMS_NO_TIMEOUT);
	TestDirective(rc, "OrbitController: mutex lock failure");
}

void OrbitController::unlock() {
	rtems_status_code rc = rtems_semaphore_release(mutexId);
	TestDirective(rc, "OrbitController: mutex unlock failure");
}

rtems_task OrbitController::ocThreadStart(rtems_task_argument arg) {
	OrbitController *oc = (OrbitController*)arg;
	oc->ocThreadBody(oc->ocThreadArg);
}

rtems_task OrbitController::ocThreadBody(rtems_task_argument arg) {
	static double sums[TOTAL_BPMS*2];
	static double sorted[TOTAL_BPMS*2];

	syslog(LOG_INFO, "OrbitController: entering main processing loop\n");
	for(;;) {
		OrbitControllerMode lmode = getMode();
		switch(lmode) {
		case STANDBY:
			stopAdcAcquisition();
			resetAdcFifos();
			while((lmode=getMode())==STANDBY) {
				syslog(LOG_INFO, "OrbitController: mode=STANDBY\n");
				rtems_task_wake_after(2000);
			}
			syslog(LOG_INFO, "OrbitController: leaving STANDBY. New mode=%i\n",lmode);
			break;
		case ASSISTED:
		case AUTONOMOUS:
			//start on the "edge" of a clock-tick:
			rtems_task_wake_after(2);
			startAdcAcquisition();
			while((lmode=getMode())!=STANDBY) {
				//Wait for notification of ADC "fifo-half-full" event...
				rendezvousWithIsr();
				stopAdcAcquisition();
				activateAdcReaders();
				//Wait (block) 'til AdcReaders have completed their block-reads: ~3 ms duration
				rendezvousWithAdcReaders();
				resetAdcFifos();
				startAdcAcquisition();
				enableAdcInterrupts();
				/* At this point, we have approx 50 ms to "do our thing" (at 10 kHz ADC framerate)
				 * before ADC FIFOs reach their 1/2-full point and trigger another interrupt.
				 */
				if(lmode==ASSISTED) {
					/* If we have new OCM setpoints to deliver, do it now
					 * We'll wait up to 20 ms for ALL the setpoints to enqueue
					 */
					int maxIter=20;
					uint32_t numMsgs=0;
					do {
						rtems_status_code rc = rtems_message_queue_get_number_pending(spQueueId,&numMsgs);
						TestDirective(rc, "OrbitController: OCM msg_q_get_number_pending failure");
						if(numMsgs < NumOcm) { rtems_task_wake_after(1); }
						else { break; }
					} while(--maxIter);
					//deliver all the OCM setpoints we have.
					if(numMsgs > 0) {
						for(uint32_t i=numMsgs; i; i--) {
							SetpointMsg *msg;
							size_t msgsz;
							rtems_status_code rc = rtems_message_queue_receive(spQueueId,msg,&msgsz,RTEMS_NO_WAIT,RTEMS_NO_TIMEOUT);
							TestDirective(rc,"OcmController: msq_q_rcv failure");
							msg->ocm->setSetpoint(msg->sp);
						}
						for(uint32_t i=0; i<psbArray.size(); i++) {
							psbArray[i]->updateSetpoints();
						}
#ifdef OC_DEBUG
						syslog(LOG_INFO, "OrbitController: updated %i OCM setpoints\n",numMsgs);
#endif
					}
				}
				else if(lmode==AUTONOMOUS) {
					/* Calc and deliver new OCM setpoints:
					 * NOTE: testing shows calc takes ~1ms
					 * while OCM setpoint delivery req's ~5ms
					 * for 48 OCMs (scales linearly with # OCM)
					 */
					uint32_t numSamples = sumAdcSamples(sums,rdSegments);
					sortBPMData(sorted,sums,rdSegments[0]->getChannelsPerFrame());
					double sf = getBpmScaleFactor(numSamples);
					map<string,Bpm*>::iterator it;
					int i;
					for(i=0,it=bpmMap.begin(); i<TOTAL_BPMS; i++, it++) {
						sorted[i] *= sf/it->second->getXVoltsPerMilli();
						sorted[i+1] *= sf/it->second->getYVoltsPerMilli();
					}
					//FIXME -- refactor calcs to private method
					//TODO: calc dispersion effect
					//Calc horizontal OCM setpoints:
					//First, generate a vector<Bpm*> of all BPMs participating in the correction
					vector<Bpm*> bpms(NumBpm);
					for(it=bpmMap.begin(); it!=bpmMap.end(); it++) {
						if(it->second->isEnabled()) { bpms.push_back(it->second); }
					}
					lock();
					double h[NumHOcm];
					memset(h,0,sizeof(h));
					for(uint32_t i=0; i<NumHOcm; i++) {
						for(uint32_t j=0; j<NumBpm; j++) {
							h[i] += hmat[i][j]*sorted[bpms[j]->getAdcOffset()];
						}
					}
					//calc vertical OCM setpoints
					double v[NumVOcm];
					memset(v,0,sizeof(v));
					for(uint32_t i=0; i<NumVOcm; i++) {
						for(uint32_t j=0; j<NumBpm; j++) {
							v[i] += vmat[i][j]*sorted[bpms[j]->getAdcOffset()+1];
						}
					}
					unlock();
					//scale OCM setpoints (max step && %-age to apply)
					//distribute new OCM setpoints
					//foreach Ocm:ocm->setSetpoint(val);
					for(uint32_t i=0; i<psbArray.size(); i++) {
						psbArray[i]->updateSetpoints();
					}
				} //vector<Bpm*> bpms destroyed here
				//hand raw ADC data off to processing thread
				enqueueAdcData();
			}//end while(mode != STANDBY) { }
			break;
		default:
			stopAdcAcquisition();
			resetAdcFifos();
			syslog(LOG_INFO, "OrbitController: undefined mode=%i encountered!! Entering STANDBY mode...\n",getMode());
			setMode(STANDBY);
			break;
		}
	}
	//state exit: silence the ADC's
	stopAdcAcquisition();
	resetAdcFifos();
	//FIXME -- temporary!!!
	rtems_task_wake_after(1000);
	TestDirective(rtems_event_send((rtems_id)arg,1),"OrbitController: problem sending event to UI thread");
	TestDirective(rtems_task_delete(ocTID),"OrbitController: problem deleting ocThread");
}

void OrbitController::startAdcAcquisition() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		adcArray[i]->startAcquisition();
	}
}

void OrbitController::stopAdcAcquisition() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		adcArray[i]->stopAcquisition();
	}
}

void OrbitController::resetAdcFifos() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		adcArray[i]->resetFifo();
	}
}

void OrbitController::enableAdcInterrupts() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		int rc = vme_enable_irq_level(crateArray[i]->getFd(),adcArray[i]->getIrqLevel());
		if(rc) {
			throw OrbitControlException("OrbitController: vme_enable_irq_level() failure!!",rc);
		}
		adcArray[i]->enableInterrupt();
	}
}

void OrbitController::disableAdcInterrupts() {
	for(uint32_t i=0; i<adcArray.size(); i++) {
		int rc = vme_disable_irq_level(crateArray[i]->getFd(),adcArray[i]->getIrqLevel());
		if(rc) {
			throw OrbitControlException("OrbitController: vme_disable_irq_level() failure!!",rc);
		}
		adcArray[i]->disableInterrupt();
	}
}

void OrbitController::rendezvousWithIsr() {
	/* Wait for notification of ADC "fifo-half-full" event... */
	rtems_status_code rc = rtems_barrier_wait(isrBarrierId,barrierTimeout);/*FIXME--debugging timeouts*/
	TestDirective(rc, "OrbitController: ISR barrier_wait() failure");
}

void OrbitController::rendezvousWithAdcReaders() {
	/* block until the ReaderThreads are at their sync-point... */
	rtems_status_code rc = rtems_barrier_wait(rdrBarrierId, barrierTimeout);/*FIXME--debugging timeouts*/
	TestDirective(rc,"OrbitController: RDR barrier_wait() failure");
}

void OrbitController::activateAdcReaders() {
	for(uint32_t i=0; i<NumAdcModules; i++) {
		rdSegments[i] = new AdcData(adcArray[i],
							HALF_FIFO_LENGTH/adcArray[i]->getChannelsPerFrame());
		//this'll unblock the associated AdcReader thread:
		rdrArray[i]->read(rdSegments[i]);
	}
}

void OrbitController::enqueueAdcData() {
	rtems_status_code rc = rtems_message_queue_send(bpmQueueId,rdSegments,sizeof(rdSegments));
	TestDirective(rc, "BpmController: msq_q_send failure");
}

rtems_task OrbitController::bpmThreadStart(rtems_task_argument arg) {
	OrbitController *oc = (OrbitController*)arg;
	oc->bpmThreadBody(oc->bpmThreadArg);
}

rtems_task OrbitController::bpmThreadBody(rtems_task_argument arg) {
	uint32_t nthFrame,nthAdc,nthChannel;
	uint32_t localSamplesPerAvg = samplesPerAvg;
	uint32_t numSamplesSummed=0;
	AdcData *ds[NumAdcModules];
	static double sums[NumBpmChannels];
	static double sorted[NumBpmChannels];

	syslog(LOG_INFO, "BpmController: entering main loop...\n");
	for(;;) {
		uint32_t bytes;
		rtems_status_code rc = rtems_message_queue_receive(bpmQueueId,ds,&bytes,RTEMS_WAIT,RTEMS_NO_TIMEOUT);
		TestDirective(rc,"BpmController: msg_q_rcv failure");
		if(bytes != bpmMsgSize) {
			syslog(LOG_INFO, "BpmController: received %u bytes in msg: was expecting %u\n",
								bytes,bpmMsgSize);
			//FIXME -- handle this error!!!
		}
		/* XXX -- assumes each ADC Data Segment has an equal # of frames and channels per frame */
        uint32_t nFrames = ds[0]->getFrames();
        uint32_t frameOffset = ds[0]->getChannelsPerFrame();
		for(nthFrame=0; nthFrame<nFrames; nthFrame++) { /* for each ADC frame... */
			int nthFrameOffset = nthFrame*frameOffset;

			for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
				int nthAdcOffset = nthAdc*frameOffset;
				int32_t *buf = ds[nthAdc]->getBuffer();

				for(nthChannel=0; nthChannel<frameOffset; nthChannel++) { /* for each channel of this frame... */
					sums[nthAdcOffset+nthChannel] += (double)buf[nthFrameOffset+nthChannel];
					//sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
				}

			}
			numSamplesSummed++;

			if(numSamplesSummed==localSamplesPerAvg) {
				sortBPMData(sorted,sums,ds[0]->getChannelsPerFrame());
				/* scale & update each BPM object, then execute client's BpmValueChangeCallback */
				map<string,Bpm*>::iterator it;
				int i;
				double cf = getBpmScaleFactor(numSamplesSummed);
				for(it=bpmMap.begin(),i=0; it!=bpmMap.end(); it++,i++) {
					double x = sorted[i]*cf/it->second->getXVoltsPerMilli();
					it->second->setX(x);
					double y = sorted[i+1]*cf/it->second->getYVoltsPerMilli();
					it->second->setY(y);
				}
				if(bpmCB != 0) {
					/* fire record processing */
					this->bpmCB(bpmCBArg);
				}
				/* zero the array of running-sums,reset counter, update num pts in avg */
				memset(sums, 0, sizeof(double)*NumBpmChannels);
				//memset(sumsSqrd, 0, sizeof(double)*NumBpmChannels);
#ifdef OC_DEBUG
				static int cnt=1;
				syslog(LOG_INFO, "BpmController: finished processing block %i with %u frames\n",cnt++,numSamplesSummed);
#endif
				numSamplesSummed=0;
				/* XXX - SamplesPerAvg can be set via orbitcontrolUI app */
				if(localSamplesPerAvg != samplesPerAvg) {
					syslog(LOG_INFO, "BpmController: changing SamplesPerAvg=%d to %d\n",
									localSamplesPerAvg,samplesPerAvg);
					localSamplesPerAvg = samplesPerAvg;
				}
			}
		}
		/* release memory allocated in OrbitController::activateAdcReaders() */
		for(uint32_t i=0; i<NumAdcModules; i++) {
			delete ds[i];
		}

	}//end for(;;) -- go wait for more ADC data
}

double OrbitController::getBpmScaleFactor(uint32_t numSamples) {
	const int mmPerMeter = 1000;
	// accounts for 24-bits of adc-data stuffed into the 3 MSB of a 32-bit word
	const int ShiftFactor = (1<<8);
	// accounts for the voltage-divider losses of an LPF aft of each Bergoz unit
	const double LPF_Factor = 1.015;
	// AdcPerVolt==(1<<23)/(20 Volts)
	const double AdcPerVolt = 419430.4;

	return LPF_Factor/(numSamples*ShiftFactor*AdcPerVolt*mmPerMeter);
}

/**
 * Returns BPM data in sortedArray in x,y sequences, in "ring-order"
 *
 * @param sortedArray
 * @param rawArray
 * @param adcChannelsPerFrame
 */
void OrbitController::sortBPMData(double *sortedArray,
					double *rawArray,
					uint32_t adcChannelsPerFrame) {
	int i,j;
	int nthAdc = 0;

	for(i=0,j=0; j<adc0ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc0ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc0ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc1ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc1ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc1ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc2ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc2ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc2ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc3ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc3ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc3ChMap[j]+1];
	}
	nthAdc = 0;
	for(j=0; j<adc4ChMap_LENGTH; i++,j++) {
		sortedArray[i] = rawArray[nthAdc+adc4ChMap[j]];
		sortedArray[i+1] = rawArray[nthAdc+adc4ChMap[j]+1];
	}
	//do this once only, but be SURE that all BPMs have been added to bpmMap at this point!
	static int once = 1;
	if(once) {
		once=0;
		map<string,Bpm*>::iterator it;
		for(i=0,it=bpmMap.begin(); it!=bpmMap.end(); it++,i++) {
			it->second->setAdcOffset(i);
		}
	}
}

/**
 * FIXME -- refactor so we're no duplicating functionality!!!
 * At least the two, inner loops can be extracted...
 *
 * @param sums ptr to array of doubles; storage for running sums
 * @param ds input array of AdcData ptrs
 *
 * @return the number of samples summed
 */
uint32_t OrbitController::sumAdcSamples(double* sums, AdcData** data) {
	uint32_t nthFrame,nthAdc,nthChannel;
	uint32_t numSamplesSummed=0;

	/* XXX -- assumes each ADC Data Segment has an equal # of frames and channels per frame */
	uint32_t nFrames = data[0]->getFrames();
	uint32_t frameOffset = data[0]->getChannelsPerFrame();
	for(nthFrame=0; nthFrame<nFrames; nthFrame++) { /* for each ADC frame... */
		int nthFrameOffset = nthFrame*frameOffset;

		for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
			int nthAdcOffset = nthAdc*frameOffset;
			int32_t *buf = data[nthAdc]->getBuffer();

			for(nthChannel=0; nthChannel<frameOffset; nthChannel++) { /* for each channel of this frame... */
				sums[nthAdcOffset+nthChannel] += (double)buf[nthFrameOffset+nthChannel];
				//sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
			}

		}

	numSamplesSummed++;
	}
	return numSamplesSummed;
}
