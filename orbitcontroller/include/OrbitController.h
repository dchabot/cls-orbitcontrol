/*
 * OrbitController.h
 *
 *  Created on: Dec 11, 2008
 *      Author: chabotd
 */

#ifndef ORBITCONTROLLER_H_
#define ORBITCONTROLLER_H_

#include <BpmController.h>
#include <OcmController.h>
#include <VmeCrate.h>
#include <Ics110blModule.h>
#include <Vmic2536Module.h>
#include <AdcReader.h>
#include <AdcIsr.h>
#include <AdcData.h>
#include <Ocm.h>
#include <PowerSupplyBulk.h>
#include <Initializing.h>
#include <Standby.h>
#include <Assisted.h>
#include <Autonomous.h>
#include <Timed.h>
#include <Testing.h>

#include <vector>
#include <map>
#include <set>
#include <rtems.h>
#include <stdint.h>

using std::vector;
using std::string;
using std::map;
using std::set;
using std::iterator;
using std::pair;


const rtems_task_priority OrbitControllerPriority=50;
const uint32_t NumAdcModules = 4;
const uint32_t NumAdcReaders = 4;
const uint32_t NumVmeCrates = 4;

//FIXME -- this should be implemented as a class, 'cause c++ enums suck ass :-(
enum OrbitControllerMode {INITIALIZING,STANDBY,ASSISTED,AUTONOMOUS,TIMED,TESTING};
typedef void (*OrbitControllerModeChangeCallback)(void*);

#define OC_DEBUG
#ifdef OC_DEBUG
	const uint32_t barrierTimeout=RTEMS_NO_TIMEOUT;
#else
	const uint32_t barrierTimeout=5000; //rtems "ticks"
#endif

/**
 * A Singleton class for managing the storage-ring orbit control system. Based on
 * GoF pattern (pg 127).
 *
 * After obtaining THE object instance via OrbitController::getInstance(),
 * the initialize(dbl rate) method should be invoked prior to calling start().
 *
 * If initialize() is NOT called prior to start(), the ICS-110BL ADCz will be configured
 * with a default sample-rate (aka frame-rate) of 10 kHz.
 *
 * NOTE: don't forget to call destroyInstance() to release all resources !!!
 *
 */
class OrbitController : public OcmController,public BpmController {
public:
	static OrbitController* getInstance();
	void start(rtems_task_argument, rtems_task_argument);
	void destroyInstance();
	double getAdcFrameRateSetpoint() const { return adcFrameRateSetpoint; }
	double getAdcFrameRateFeedback() const { return adcFrameRateFeedback; }
	OrbitControllerMode getMode();
	void setMode(OrbitControllerMode mode);
	void setModeChangeCallback(OrbitControllerModeChangeCallback cb, void* cbArg);

	//virtual methods inherited from abstract base-class OcmController:
	Ocm* registerOcm(const string& str,uint32_t crateId,
						uint32_t vmeAddr,uint8_t ch,uint32_t pos);
	void unregisterOcm(Ocm* ch);
	Ocm* getOcmById(const string& id);
	void showAllOcms();
	void setOcmSetpoint(Ocm* ch, int32_t val);
	void setVerticalResponseMatrix(double v[NumVOcm*NumBpm]);
	void setHorizontalResponseMatrix(double h[NumHOcm*NumBpm]);
	void setDispersionVector(double d[NumBpm]);
	void setMaxHorizontalStep(int32_t step) { maxHStep = step; }
	int32_t getMaxHorizontalStep() const { return maxHStep; }
	void setMaxVerticalStep(int32_t step) { maxVStep = step; }
	int32_t getMaxVerticalStep() const { return maxVStep; }
	void setMaxHorizontalFraction(double f) { lock(); maxHFrac = f; unlock(); }
	double getMaxHorizontalFraction() const { return maxHFrac; }
	void setMaxVerticalFraction(double f) { lock(); maxVFrac = f; unlock(); }
	double getMaxVerticalFraction() const { return maxVFrac; }

	//virtual methods inherited from abstract base-class BpmController
	void registerBpm(Bpm* bpm);
	void unregisterBpm(Bpm* bpm);
	Bpm* getBpmById(const string& id);
	void setBpmValueChangeCallback(BpmValueChangeCallback cb, void* cbArg);
	uint32_t getSamplesPerAvg() const { return samplesPerAvg; }
	void setSamplesPerAvg(uint32_t num) { samplesPerAvg = num; }
	void showAllBpms();

private:
	OrbitController();
	OrbitController(const OrbitController&);
	const OrbitController& operator=(const OrbitController&);
	~OrbitController();

	friend class Initializing;
	friend class Standby;
	friend class Assisted;
	friend class Autonomous;
	friend class Timed;
	friend class Testing;

	friend void fastAlgorithm(OrbitController*);

	void changeState(State*);
	void lock();
	void unlock();
	void startAdcAcquisition();
	void stopAdcAcquisition();
	void resetAdcFifos();
	void enableAdcInterrupts();
	void disableAdcInterrupts();
	void rendezvousWithIsr();
	void rendezvousWithAdcReaders();
	void activateAdcReaders(rtems_id,uint32_t);
	void enqueueAdcData();

	static rtems_task ocThreadStart(rtems_task_argument arg);
	rtems_task ocThreadBody(rtems_task_argument arg);
	static rtems_task bpmThreadStart(rtems_task_argument arg);
	rtems_task bpmThreadBody(rtems_task_argument arg);

	static OrbitController* instance;
	OrbitControllerModeChangeCallback mcCallback;
	void* mcCallbackArg;
	rtems_id mutexId;
	rtems_id ocTID;
	rtems_name ocThreadName;
	rtems_task_argument ocThreadArg;
	rtems_task_priority ocThreadPriority;

	rtems_interval rtemsTicksPerSecond;
	uint32_t adcFramesPerTick;
	double adcFrameRateSetpoint;
	double adcFrameRateFeedback;

	rtems_id isrBarrierId;
	rtems_name isrBarrierName;
	rtems_id rdrBarrierId;
	rtems_name rdrBarrierName;

	vector<VmeCrate*> crateArray;
	vector<Ics110blModule*> adcArray;
	vector<Vmic2536Module*> dioArray;
	vector<AdcIsr*> isrArray;
	vector<AdcReader*> rdrArray;
	vector<PowerSupplyBulk*> psbArray;
	AdcData* rdSegments[NumAdcModules];

	State *state;
	rtems_id stateQueueId;
	rtems_name stateQueueName;

	bool initialized;
	OrbitControllerMode mode;

	//OcmController attributes
	//private struct for determining order in OCM sets
	struct OcmCompare {
		bool operator()(Ocm* lhs, Ocm* rhs) const {
			return (lhs->getPosition()<rhs->getPosition());
		}
	};
	set<Ocm*,OcmCompare> vOcmSet;//vertical OCM
	set<Ocm*,OcmCompare> hOcmSet;//horizontal OCM
	rtems_id spQueueId;
	rtems_name spQueueName;
	//private struct for enqueueing OCM setpoint changes (single)
	struct SetpointMsg {
		SetpointMsg(Ocm* ocm, int32_t setpoint):ocm(ocm),sp(setpoint){}
		~SetpointMsg(){}
		Ocm* ocm;
		int32_t sp;
	};
	int32_t maxHStep;
	int32_t maxVStep;
	double maxHFrac;
	double maxVFrac;
	/* FIXME -- replace static arrays with vector<vector<double>> && vector<double>
	 * NOTE  -- also replace const NumOcm/NumBpm,etc with static variables and let
	 * 			the UI inform *us* of how many OCM/BPM are required...
	 */
	double hmat[NumHOcm][NumBpm];
	double vmat[NumVOcm][NumBpm];
	double dmat[NumBpm];
	bool hResponseInitialized;
	bool vResponseInitialized;
	bool dispInitialized;

	//BpmController attributes
	uint32_t samplesPerAvg;
	map<string,Bpm*> bpmMap;
	const uint32_t bpmMsgSize;
	const uint32_t bpmMaxMsgs;
	rtems_id bpmTID;
	rtems_name bpmThreadName;
	rtems_task_argument bpmThreadArg;
	rtems_task_priority bpmThreadPriority;
	rtems_id bpmQueueId;
	rtems_name bpmQueueName;
	BpmValueChangeCallback bpmCB;
	void* bpmCBArg;
	//BpmController private methods
	uint32_t sumAdcSamples(double* sums, AdcData** data);
	void sortBPMData(double *sortedArray,double *rawArray,uint32_t adcChannelsPerFrame);
	double getBpmScaleFactor(uint32_t numSamples);
	double getBpmSNR(double sum, double sumSqr, uint32_t n);
};

/* here are the ADC channel mappings based on the current drawings*/
/* these channels may be moved around in the future */
/* structure is x,y,x,y,x,y... map positions indicate the X positions ONLY ! */
const int adc0ChMap_LENGTH=4;  /* 2404.1 */
/*BPM {1401-01,1401-02,1401-03,1401-04}*/
const int adc0ChMap[adc0ChMap_LENGTH] = {9*2,10*2,11*2,12*2};

const int adc1ChMap_LENGTH=14; /* 2406.1 */
/*BPM{1402-07,1402-08,1402-09,1402-10,1403-01,1403-02,1403-03,1403-04,1403-05,1404-01,1404-02,1404-03,1404-04,1404-05}*/
const int adc1ChMap[adc1ChMap_LENGTH] = {1*2,2*2,3*2,0*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,11*2,12*2,13*2};

const int adc2ChMap_LENGTH=14; /* 2406.3 */
/* BPM{1405-01,1405-02,1405-03,1405-04,1405-05,1407-01,1406-02,1406-03,1406-04,1406-05,1407-03,1407-04,1407-05,1407-06 }*/
const int adc2ChMap[adc2ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,6*2,7*2,8*2,9*2,5*2,10*2,11*2,12*2,13*2};

/* April 28/08 BPM1408-01 replaced with Libera unit */
/* May 1/08 BPM1410-01 replaced with Libera unit */
const int adc3ChMap_LENGTH=13; /* 2408.1 */
/* BPM{1408-01,1408-02,1408-03,1408-04,1408-05,1409-01,1409-02,1409-03,1409-04,1409-05,1410-02,1410-03,1410-04,1410-05} */
const int adc3ChMap[adc3ChMap_LENGTH] = {1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,11*2,12*2,13*2,14*2};

const int adc4ChMap_LENGTH=9;
/* BPM{1411-01,1411-02,1411-03,1411-04,1411-05,1412-02,1412-03,1412-04,1412-05 } */
const int adc4ChMap[adc4ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2};

const int TOTAL_BPMS=(adc0ChMap_LENGTH + adc1ChMap_LENGTH + adc2ChMap_LENGTH + adc3ChMap_LENGTH + adc4ChMap_LENGTH);
const int NumBpmChannels=2*TOTAL_BPMS;
#endif /* ORBITCONTROLLER_H_ */
