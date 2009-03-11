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
#include <cmath> //fabs(double)
#include <tscDefs.h>

//BPM data-conversion factors:
static const int mmPerMeter = 1000;
// accounts for 24-bits of adc-data stuffed into the 3 MSB of a 32-bit word
static const int ShiftFactor = (1<<8);
// accounts for the voltage-divider losses of an LPF aft of each Bergoz unit
static const double LPF_Factor = 1.015;
// AdcPerVolt==(1<<23)/(20 Volts)
static const double AdcPerVolt = 419430.4;
static const double BpmFactor = LPF_Factor/(ShiftFactor*AdcPerVolt*mmPerMeter);

static State *states[TESTING+1];

/* THE singleton instance of this class */
OrbitController* OrbitController::instance = 0;

OrbitController::OrbitController() :
	//ctor-initializer list
	modeChangePublisher(0),
	mutexId(0),ocTID(0),ocThreadName(0),ocThreadArg(0),
	ocThreadPriority(OrbitControllerPriority),
	rtemsTicksPerSecond(0),adcFramesPerTick(0),
	adcFrameRateSetpoint(0),adcFrameRateFeedback(0),
	isrBarrierId(0),isrBarrierName(0),
	rdrBarrierId(0),rdrBarrierName(0),
	bufPoolId(0),bufPoolName(0),bufPool(0),
	state(NULL),stateQueueId(0),stateQueueName(0),
	initialized(false),mode(INITIALIZING),
	spQueueId(0),spQueueName(0),
	hResponseInitialized(false),vResponseInitialized(false),
	dispInitialized(false),samplesPerAvg(5000),
	bpmMsgSize(sizeof(rdSegments)),bpmMaxMsgs(10),
	bpmTID(0),bpmThreadName(0),
	bpmThreadArg(0),bpmThreadPriority(OrbitControllerPriority+3),
	bpmQueueId(0),bpmQueueName(0),bpmEventPublisher(0)

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
	delete bpmEventPublisher;
	delete modeChangePublisher;
	set<Ocm*>::iterator ocmit;
	for(ocmit=hOcmSet.begin(); ocmit!=hOcmSet.end(); ocmit++) { delete *ocmit; }
	hOcmSet.clear();
	for(ocmit=vOcmSet.begin(); ocmit!=vOcmSet.end(); ocmit++) { delete *ocmit; }
	vOcmSet.clear();
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
	for(uint32_t i=0; i<TESTING; i++) { delete states[i]; }
	if(bufPoolId) { rtems_region_delete(bufPoolId); delete []bufPool; }
	instance = 0;
}

OrbitController* OrbitController::getInstance() {
	//FIXME -- not thread-safe!!
	if(instance==0) {
		instance = new OrbitController();
		states[INITIALIZING] = Initializing::getInstance();
		states[STANDBY] = Standby::getInstance();
		states[ASSISTED] = Assisted::getInstance();
		states[AUTONOMOUS] = Autonomous::getInstance();
		states[TIMED] = Timed::getInstance();
		states[TESTING] = Testing::getInstance();
		instance->state = states[INITIALIZING];
	}
	return instance;
}

void OrbitController::destroyInstance() { delete instance; }

void OrbitController::start(rtems_task_argument ocThreadArg,
							rtems_task_argument bpmThreadArg) {
	//FIXME--
	if(initialized==false) {
		changeState(states[INITIALIZING]);
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
	rtems_status_code rc = rtems_message_queue_send(stateQueueId,&mode,sizeof(OrbitControllerMode));
	TestDirective(rc,"OrbitController: stateQ msg_send failure");
	unlock();
}

/* FIXME
 * Refactor the BPM and OCM callback mechanisms using standard
 * Observer && Command patterns (i.e. vectors of observers and their cmds,publish,etc)
 */
void OrbitController::registerForModeEvents(Command* cmd) {
	modeChangePublisher->subscribe(cmd);
}
/********************** OcmController public interface *********************/
/**
 *
 * @param id
 * @return 1 if horizontal, 0 if vertical, -1 if id is ill-formed (error).
 */
static ocmType getOcmType(const string& id) {
    if(id.find("OCH") != string::npos) { return HORIZONTAL; }
    else if(id.find("OCV") != string::npos) { return VERTICAL; }
    else if(id.find("SOA") != string::npos) {
        size_t pos = id.find_first_of(":");
        if(id.compare(pos+1,1,"X")==0) { return HORIZONTAL; }
        else if(id.compare(pos+1,1,"Y")==0) { return VERTICAL; }
    }
    //screwed up id: bail out
    syslog(LOG_INFO, "Can't identify type with id=%s\n",id.c_str());
    return UNKNOWN;
}

Ocm* OrbitController::registerOcm(const string& id,
									uint32_t crateId,
									uint32_t vmeAddr,
									uint8_t ch,
									uint32_t pos) {
	Ocm *ocm = NULL;
	//find the matching VME crate/DIO module pair controlling this OCM:
	for(uint32_t i=0; i<dioArray.size(); i++) {
		if(crateId==dioArray[i]->getCrate()->getId()
				&& vmeAddr==dioArray[i]->getVmeBaseAddr()) {
			//found a matching crate/module pair
			ocm = getOcmById(id);
			if(ocm == NULL) {
				lock();
				ocm = new Ocm(id,dioArray[i],ch);
				ocm->setPosition(pos);
				//stuff this OCM into hOcmSet or vOcmSet:
				ocmType ocmtype = getOcmType(id);
				pair<set<Ocm*>::iterator,bool> ret;
				if(ocmtype==HORIZONTAL) { ret = hOcmSet.insert(ocm); }
				else if(ocmtype==VERTICAL) { ret = vOcmSet.insert(ocm); 	}
				if(ret.second != false) {
					syslog(LOG_INFO, "OcmController: added %s to %s OCM set.\n",
										ocm->getId().c_str(),
										getOcmType(ocm->getId())==HORIZONTAL?"horizontal":"vertical");
				}
				else {
					//the OCM already exists in hOcmSet or vOcmSet.
					//Destroy new instance and return ptr to std::set member
					delete ocm;
					ocm = *(ret.first);
				}
				unlock();
			}
			break;
		}
	}
	if(ocm==NULL) {
		//FIXME -- this should be fatal. It's a resolvable configuration issue.
		syslog(LOG_INFO, "OcmController: couldn't match DIO module with Ocm id=%s addr=%#x!!\n",
						  id.c_str(),vmeAddr);
	}
	return ocm;
}

void OrbitController::unregisterOcm(Ocm* ocm) {
	set<Ocm*>::iterator it;
	ocmType ocmtype = getOcmType(ocm->getId());
	if(ocmtype==HORIZONTAL) {
		it = hOcmSet.find(ocm);
		if(it != hOcmSet.end()) {
			hOcmSet.erase(it);
			delete ocm;
		}
	}
	else if(ocmtype==VERTICAL) {
		it = vOcmSet.find(ocm);
		if(it != vOcmSet.end()) {
			vOcmSet.erase(it);
			delete ocm;
		}
	}
	else {
		syslog(LOG_INFO, "Failed to remove %s from OCM maps; it doesn't exist!\n",
				ocm->getId().c_str());
	}
}

Ocm* OrbitController::getOcmById(const string& id) {
	lock();
	Ocm *ocm = NULL;
	set<Ocm*>::iterator it;
	ocmType ocmtype = getOcmType(id);
	if(ocmtype==HORIZONTAL) {
		for(it=hOcmSet.begin(); it!=hOcmSet.end(); it++) {
			if(id.compare((*it)->getId())==0) { ocm=*it; break; }
		}
	}
	else if(ocmtype==VERTICAL) {
		for(it=vOcmSet.begin(); it!=vOcmSet.end(); it++) {
			if(id.compare((*it)->getId())==0) { ocm=*it; break; }
		}
	}
	unlock();
	return ocm;
}

static void printOcmInfo(Ocm* ocm) {
	syslog(LOG_INFO, "%s: position=%i setpoint=%i inCorrection=%s delay=%u us\n",
					ocm->getId().c_str(),
					ocm->getPosition(),
					ocm->getSetpoint(),
					ocm->isEnabled()?"true":"false",
					ocm->getDelay());
}

void OrbitController::showAllOcms() {
	set<Ocm*>::iterator it;
	for(it=hOcmSet.begin(); it!=hOcmSet.end(); it++) { printOcmInfo(*it); }
	syslog(LOG_INFO, "\n\n\n");
	for(it=vOcmSet.begin(); it!=vOcmSet.end(); it++) { printOcmInfo(*it); }
	syslog(LOG_INFO, "\n\n\n");
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
			vmat[row][col] = v[col*NumVOcm+row];
		}
	}
	vResponseInitialized=true;
	unlock();
#ifdef OC_DEBUG
	for(col=0; col<2; col++) {
		for(row=0; row<NumVOcm; row++) {
			syslog(LOG_INFO, "vmat[%i][%i]=%.3e\n",row,col,vmat[row][col]);
		}
	}
#endif
}

void OrbitController::setHorizontalResponseMatrix(double h[NumHOcm*NumBpm]) {
	uint32_t row,col;
	lock();
	for(col=0; col<NumBpm; col++) {
		for(row=0; row<NumHOcm; row++) {
			hmat[row][col] = h[col*NumHOcm+row];
		}
	}
	hResponseInitialized=true;
	unlock();
#ifdef OC_DEBUG
	for(col=0; col<2; col++) {
		for(row=0; row<NumHOcm; row++) {
			syslog(LOG_INFO, "hmat[%i][%i]=%.3e\n",row,col,hmat[row][col]);
		}
	}
#endif
}

void OrbitController::setDispersionVector(double d[NumBpm]) {
	lock();
	for(uint32_t col=0; col<NumBpm; col++) {
		dmat[col] = d[col];
	}
	dispInitialized=true;
	unlock();
#ifdef OC_DEBUG
	for(uint32_t col=0; col<NumBpm; col++) {
		syslog(LOG_INFO, "dmat[%i]=%.3e\n",col,dmat[col]);
	}
#endif
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
		syslog(LOG_INFO, "%s: pos=%i x=%.3e y=%.3e enabled=%s\n",
						it->second->getId().c_str(),
						it->second->getPosition(),
						it->second->getX(),
						it->second->getY(),
						it->second->isEnabled()?"yes":"no");
	}
}

void OrbitController::registerForBpmEvents(Command* cmd) {
	bpmEventPublisher->subscribe(cmd);
}

/*********************** private methods *****************************************/
void OrbitController::changeState(State* aState) {
	state->changeState(aState);
	state=aState;
}

//FIXME -- refactor lock()/unlock() to signatures like lock(id:rtems_id)
// 		   This will permit finer-grained locking by admitting BPM,OCM,
//		   and OrbitController locks.
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

static uint64_t now,then,tmp,numIters;
static double sum,sumSqrs,avg,stdDev,maxTime;
extern double tscTicksPerSecond;

rtems_task OrbitController::ocThreadBody(rtems_task_argument arg) {
	OrbitControllerMode lmode;
	size_t msgSize;

	syslog(LOG_INFO, "OrbitController: entering main processing loop\n");
	//goto Assisted mode as our first state *after* INITIALIZING
	changeState(states[ASSISTED]);
	modeChangePublisher->publish();
	for(;;) {
		msgSize = 0; // MUST reset this per iteration!!
		rtems_message_queue_receive(stateQueueId,&lmode,&msgSize,RTEMS_NO_WAIT,RTEMS_NO_TIMEOUT);
		//FIXME -- TestDirective(rc,"OrbitController--stateQ msq_rcv problem");
		if(msgSize!=sizeof(OrbitControllerMode)) {
			//no msg: transition to self (current state)
			changeState(state);
		}
		else {
			//transition to new State
			changeState(states[lmode]);
			modeChangePublisher->publish();
#ifdef OC_DEBUG
			stdDev = (1.0/(double)(numIters))*sumSqrs - (1.0/(double)(numIters*numIters))*(sum*sum);
			stdDev = sqrt(stdDev);
			stdDev /= tscTicksPerSecond;
			avg = sum/((double)numIters);
			avg /= tscTicksPerSecond;
			maxTime /= tscTicksPerSecond;

			syslog(LOG_INFO, "OrbitController - AdcData stats:\n\tAvg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",avg,stdDev,maxTime);
			/* zero the parameters for the next iteration...*/
			sum=sumSqrs=avg=stdDev=maxTime=0.0;
			numIters=0;
#endif
		}
	}
	//state exit: silence the ADC's
	stopAdcAcquisition();
	resetAdcFifos();
	//FIXME -- temporary!!!
	rtems_task_wake_after(1000);
	TestDirective(rtems_task_suspend(ocTID),"OrbitController: problem suspending ocThread");
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
	rtems_status_code rc = rtems_barrier_wait(isrBarrierId,barrierTimeout);
	TestDirective(rc, "OrbitController: ISR barrier_wait() failure");
}

void OrbitController::rendezvousWithAdcReaders() {
	/* block until the ReaderThreads are at their sync-point... */
	rtems_status_code rc = rtems_barrier_wait(rdrBarrierId, barrierTimeout);
	TestDirective(rc,"OrbitController: RDR barrier_wait() failure");
}

void OrbitController::activateAdcReaders(uint32_t numFrames) {
	for(uint32_t i=0; i<NumAdcModules; i++) {
		rdtscll(then);
		rdSegments[i] = new AdcData(bufPoolId,adcArray[i]->getChannelsPerFrame(),numFrames);
		rdtscll(now);
		//this'll unblock the associated AdcReader thread:
		rdrArray[i]->read(rdSegments[i]);
	}
#ifdef OC_DEBUG
	tmp = now-then;
	sum += (double)tmp;
	sumSqrs += (double)(tmp*tmp);
	if((double)tmp>maxTime) {
		maxTime = (double)tmp;
	}
	++numIters;
#endif
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
	static double sums[NumAdcModules*32];
	static double sumsSqrd[NumAdcModules*32];
	static double sortedSums[NumBpmChannels];
	static double sortedSumsSqrd[NumBpmChannels];

	syslog(LOG_INFO, "BpmController: entering main loop...\n");
	for(;;) {
		uint32_t bytes;
		rtems_status_code rc = rtems_message_queue_receive(bpmQueueId,ds,&bytes,RTEMS_WAIT,RTEMS_NO_TIMEOUT);
		TestDirective(rc,"BpmController: msg_q_rcv failure");
		if(bytes != bpmMsgSize) {
			//FIXME -- handle this error!!!
			syslog(LOG_INFO, "BpmController: received %u bytes in msg: was expecting %u\n",
								bytes,bpmMsgSize);
		}
		/* XXX -- assumes each ADC Data Segment has an equal # of frames and channels per frame */
		uint32_t numFrames = ds[0]->getFrames();
        uint32_t chPerFrame = ds[0]->getChannelsPerFrame();
		for(nthFrame=0; nthFrame<numFrames; nthFrame++) { /* for each ADC frame... */
			int nthFrameOffset = nthFrame*chPerFrame;

			for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
				int nthAdcOffset = nthAdc*chPerFrame;
				int32_t *buf = ds[nthAdc]->getBuffer();
				double val;

				for(nthChannel=0; nthChannel<chPerFrame; nthChannel++) { /* for each channel of this frame... */
					val = (double)buf[nthFrameOffset+nthChannel];
					sums[nthAdcOffset+nthChannel] += val;
					sumsSqrd[nthAdcOffset+nthChannel] += (val*val);
				}
			}
			numSamplesSummed++;

			if(numSamplesSummed==localSamplesPerAvg) {
				sortBPMData(sortedSums,sums,ds[0]->getChannelsPerFrame());
				/* scale & update each BPM object's x&&y, then execute client's BpmValueChangeCallback */
				map<string,Bpm*>::iterator it;
				double cf = getBpmScaleFactor(numSamplesSummed);
				for(it=bpmMap.begin(); it!=bpmMap.end(); it++) {
					Bpm *bpm = it->second;
					uint32_t pos = bpm->getPosition();
					double x = sortedSums[2*pos]*cf/bpm->getXVoltsPerMilli();
					bpm->setX(x);
					double y = sortedSums[2*pos+1]*cf/bpm->getYVoltsPerMilli();
					bpm->setY(y);
				}
				/* also update each BPM instance's x&&y SNR: */
				sortBPMData(sortedSumsSqrd,sumsSqrd,ds[0]->getChannelsPerFrame());
				for(it=bpmMap.begin(); it!=bpmMap.end(); it++) {
					Bpm *bpm = it->second;
					uint32_t pos = bpm->getPosition();
					double xsnr = getBpmSigma(sortedSums[2*pos],sortedSumsSqrd[2*pos],numSamplesSummed);
					bpm->setXSigma(xsnr);
					double ysnr = getBpmSigma(sortedSums[2*pos+1],sortedSumsSqrd[2*pos+1],numSamplesSummed);
					bpm->setYSigma(ysnr);
				}
				bpmEventPublisher->publish();
				/* zero the array of running-sums,reset counter, update num pts in avg */
				memset(sums,0,sizeof(sums));
				memset(sortedSums,0,sizeof(sortedSums));
				memset(sumsSqrd,0,sizeof(sumsSqrd));
				memset(sortedSumsSqrd,0,sizeof(sortedSumsSqrd));
#if 0 //def OC_DEBUG
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
	return BpmFactor/numSamples;
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
		sortedArray[2*i] = rawArray[nthAdc+adc0ChMap[j]];
		sortedArray[2*i+1] = rawArray[nthAdc+adc0ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc1ChMap_LENGTH; i++,j++) {
		sortedArray[2*i] = rawArray[nthAdc+adc1ChMap[j]];
		sortedArray[2*i+1] = rawArray[nthAdc+adc1ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc2ChMap_LENGTH; i++,j++) {
		sortedArray[2*i] = rawArray[nthAdc+adc2ChMap[j]];
		sortedArray[2*i+1] = rawArray[nthAdc+adc2ChMap[j]+1];
	}
	nthAdc += adcChannelsPerFrame;
	for(j=0; j<adc3ChMap_LENGTH; i++,j++) {
		sortedArray[2*i] = rawArray[nthAdc+adc3ChMap[j]];
		sortedArray[2*i+1] = rawArray[nthAdc+adc3ChMap[j]+1];
	}
	nthAdc = 0;
	for(j=0; j<adc4ChMap_LENGTH; i++,j++) {
		sortedArray[2*i] = rawArray[nthAdc+adc4ChMap[j]];
		sortedArray[2*i+1] = rawArray[nthAdc+adc4ChMap[j]+1];
	}
}

/**
 * FIXME -- refactor so we're not duplicating functionality!!!
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
			}

		}

	numSamplesSummed++;
	}
	return numSamplesSummed;
}

double OrbitController::getBpmSigma(double sum, double sumSqr, uint32_t n) {
	// sigma = sqrt(((1/n)*sumSqr)-(sum/n)^2) [m]
	double a = sumSqr/n;
	double b = pow((sum/n),2);

	return sqrt(a-b)*BpmFactor;
}
