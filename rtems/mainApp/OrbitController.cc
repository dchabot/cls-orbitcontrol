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
	ocTID(0),ocThreadName(0),ocThreadArg(0),
	ocThreadPriority(OrbitControllerPriority),
	rtemsTicksPerSecond(0),adcFramesPerTick(0),
	adcFrameRateSetpoint(0),adcFrameRateFeedback(0),
	isrBarrierId(0),isrBarrierName(0),
	rdrBarrierId(0),rdrBarrierName(0),
	initialized(false),mode(ASSISTED),
	spQueueId(0),spQueueName(0),
	samplesPerAvg(5000),
	bpmMsgSize(sizeof(AdcData*)*NumAdcModules),bpmMaxMsgs(10),
	bpmTID(0),bpmThreadName(0),
	bpmThreadArg(0),bpmThreadPriority(OrbitControllerPriority+2),
	bpmQueueId(0),bpmQueueName(0),
	bpmCB(0),bpmCBArg(0)
{ }

OrbitController::~OrbitController() { }

OrbitController* OrbitController::getInstance() {
	//FIXME -- not thread-safe!!
	if(instance==0) {
		instance = new OrbitController();
	}
	return instance;
}

void OrbitController::destroyInstance() {
	syslog(LOG_INFO, "Destroying OrbitController instance!!\n");
	stopAdcAcquisition();
	resetAdcFifos();
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

void OrbitController::initialize(const double adcSampleRate) {
	rtems_status_code rc;
	syslog(LOG_INFO, "OrbitController: initializing...\n");
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
		}
	}
	if(ocm != NULL) {
		//stuff this OCM in our Map:
		pair<map<string,Ocm*>::iterator,bool> ret;
		ret = ocmMap.insert(pair<string,Ocm*>(ocm->getId(),ocm));
		if(ret.second == false) {
			syslog(LOG_INFO, "Failed to insert %s in ocmMap; it already exists!\n",
								ocm->getId().c_str());
		}
		syslog(LOG_INFO, "OcmController: added %s to ocmMap.\n",ocm->getId().c_str());
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
	if(it != ocmMap.end()) {
		return it->second;
	}
	else {
		syslog(LOG_INFO, "OcmController: Failed to find %s in ocmMap!!\n",
				id.c_str());
		return NULL;
	}
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

//FIXME -- need mutex protection around matrix manipulations!!!!!!!
void OrbitController::setVerticalResponseMatrix(double v[NumOcm*NumOcm]) {
	uint32_t i,j;
	for(i=0; i<NumOcm; i++) {
		for(j=0; j<NumOcm; j++) {
			vmat[i][j] = v[i*NumOcm+j];
		}
	}
}

void OrbitController::setHorizontalResponseMatrix(double h[NumOcm*NumOcm]) {
	uint32_t i,j;
	for(i=0; i<NumOcm; i++) {
		for(j=0; j<NumOcm; j++) {
			hmat[i][j] = h[i*NumOcm+j];
		}
	}
}

void OrbitController::setDispersionVector(double d[NumOcm]) {
	for(uint32_t i=0; i<NumOcm; i++) {
		dmat[i] = d[i];
	}
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
	if(it != bpmMap.end()) {
		return it->second;
	}
	else {
		return NULL;
	}

}

void OrbitController::showAllBpms() {
	map<string,Bpm*>::iterator it;
	for(it=bpmMap.begin(); it!=bpmMap.end(); it++) {
		syslog(LOG_INFO, "%s: x=%.9g\ty=%.9g\n",it->second->getId().c_str(),
												it->second->getX(),
												it->second->getY());
	}
}

void OrbitController::setBpmValueChangeCallback(BpmValueChangeCallback cb, void* cbArg) {
	bpmCB=cb;
	bpmCBArg=cbArg;
}

/*********************** private methods *****************************************/

rtems_task OrbitController::ocThreadStart(rtems_task_argument arg) {
	OrbitController *oc = (OrbitController*)arg;
	oc->ocThreadBody(oc->ocThreadArg);
}

rtems_task OrbitController::ocThreadBody(rtems_task_argument arg) {
	syslog(LOG_INFO, "OrbitController: entering main processing loop\n");
	//start on the "edge" of a clock-tick:
	rtems_task_wake_after(2);
	startAdcAcquisition();
	for(int j=0; j<1000000; j++) {
		//Wait for notification of ADC "fifo-half-full" event...
		rendezvousWithIsr();
		stopAdcAcquisition();
		activateAdcReaders();
		//Wait (block) 'til AdcReaders have completed their block-reads:
		rendezvousWithAdcReaders();
		resetAdcFifos();
		startAdcAcquisition();
		enableAdcInterrupts();
		//FIXME -- temporary debugging!!
		for(uint32_t i=0; i<NumAdcModules; i++) {
			delete rdSegments[i];
		}
	}
	stopAdcAcquisition();
	resetAdcFifos();
	//FIXME -- temporary!!!
	rtems_event_send((rtems_id)arg,1);
	rtems_task_delete(ocTID);
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
	rtems_status_code rc = rtems_barrier_wait(isrBarrierId,5000);/*FIXME--debugging timeouts*/
	TestDirective(rc, "OrbitController: ISR barrier_wait() failure");
}

void OrbitController::rendezvousWithAdcReaders() {
	/* block until the ReaderThreads are at their sync-point... */
	rtems_status_code rc = rtems_barrier_wait(rdrBarrierId, 5000);/*FIXME--debugging timeouts*/
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

void OrbitController::enqueueAdcData(AdcData** data) {
	rtems_status_code rc = rtems_message_queue_send(bpmQueueId,(void*)data,bpmMsgSize);
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
	static double sums[NumBpmChannels];
	static double sorted[NumBpmChannels];

	syslog(LOG_INFO, "BpmController: entering main loop...\n");
	for(;;) {
		uint32_t bytes;
		rtems_status_code rc = rtems_message_queue_receive(bpmQueueId,(void*)rdSegments,
															&bytes,RTEMS_LOCAL|RTEMS_WAIT,
															RTEMS_NO_TIMEOUT);
		TestDirective(rc,"BpmController: msg_q_rcv failure");
		if(bytes%bpmMsgSize) {
			syslog(LOG_INFO, "BpmController: received %u bytes in msg: was expecting %u\n",
								bytes,bpmMsgSize);
			//FIXME -- handle this error!!!
		}
		/* XXX -- assumes each rdSegment has an equal # of frames... */
		for(nthFrame=0; nthFrame<rdSegments[nthFrame]->getFrames(); nthFrame++) { /* for each ADC frame in rdSegment[].buf... */
			int nthFrameOffset = nthFrame*rdSegments[nthFrame]->getChannelsPerFrame();

			for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
				int nthAdcOffset = nthAdc*rdSegments[nthAdc]->getChannelsPerFrame();

				for(nthChannel=0; nthChannel<rdSegments[nthAdc]->getChannelsPerFrame(); nthChannel++) { /* for each channel of this frame... */
					sums[nthAdcOffset+nthChannel] += (double)(rdSegments[nthAdc]->getBuffer()[nthFrameOffset+nthChannel]);
					//sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
				}

			}
			numSamplesSummed++;

			if(numSamplesSummed==localSamplesPerAvg) {
				sortBPMData(sorted,sums,rdSegments[0]->getChannelsPerFrame());
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
				numSamplesSummed=0;
				/* XXX - SamplesPerAvg can be set via orbitcontrolUI app */
				if(localSamplesPerAvg != samplesPerAvg) {
					syslog(LOG_INFO, "BpmController: changing SamplesPerAvg=%d to %d\n",
									localSamplesPerAvg,samplesPerAvg);
					localSamplesPerAvg = samplesPerAvg;
				}
			}
		}
		/* release allocated memory */
		for(uint32_t i=0; i<NumAdcModules; i++) {
			delete rdSegments[i];
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
}

/**
 * FIXME -- refactor so we're no duplicating functionality!!!
 *
 * @param sums ptr to array of doubles; storage for running sums
 * @param ds input array of AdcData ptrs
 *
 * @return the number of samples summed
 */
uint32_t OrbitController::sumAdcSamples(double* sums, AdcData** data) {
	uint32_t nthFrame,nthAdc,nthChannel;
	uint32_t numSamplesSummed=0;

	/* XXX -- assumes each rdSegment has an equal # of frames... */
	for(nthFrame=0; nthFrame<data[nthFrame]->getFrames(); nthFrame++) { /* for each ADC frame in rdSegment[].buf... */
		int nthFrameOffset = nthFrame*data[nthFrame]->getChannelsPerFrame();

		for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
			int nthAdcOffset = nthAdc*data[nthAdc]->getChannelsPerFrame();

			for(nthChannel=0; nthChannel<data[nthAdc]->getChannelsPerFrame(); nthChannel++) { /* for each channel of this frame... */
				sums[nthAdcOffset+nthChannel] += (double)(data[nthAdc]->getBuffer()[nthFrameOffset+nthChannel]);
				//sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
			}

		}
		numSamplesSummed++;
	}
	return numSamplesSummed;
}
