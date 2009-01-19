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
#include <vector>
#include <map>
#include <rtems.h>
#include <stdint.h>

using std::vector;
using std::string;
using std::map;
using std::iterator;
using std::pair;


const rtems_task_priority OrbitControllerPriority=50;
const uint32_t NumAdcModules = 4;
const uint32_t NumAdcReaders = 4;
const uint32_t NumVmeCrates = 4;
//FIXME -- this should be implemented as a class, 'cause c++ enums suck ass :-(
enum OrbitControllerMode {INITIALIZING,STANDBY,ASSISTED,AUTONOMOUS};
typedef void (*OrbitControllerModeChangeCallback)(void*);


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
 * XXX -- don't forget to call destroyInstance() to release all resources !!!
 *
 */
class OrbitController : public OcmController,public BpmController {
public:
	static OrbitController* getInstance();
	void initialize(const double adcSampleRate=10.1);
	void start(rtems_task_argument arg1, rtems_task_argument arg2);
	void destroyInstance();
	double getAdcFrameRateSetpoint() const { return adcFrameRateSetpoint; }
	double getAdcFrameRateFeedback() const { return adcFrameRateFeedback; }
	OrbitControllerMode getMode() const { return mode; }
	void setMode(OrbitControllerMode mode) { this->mode = mode; }
	void setModeChangeCallback(OrbitControllerModeChangeCallback cb, void* cbArg);

	//virtual methods inherited from abstract base-class OcmController:
	Ocm* registerOcm(const string& str,uint32_t crateId,uint32_t vmeAddr,uint8_t ch);
	void unregisterOcm(Ocm* ch);
	Ocm* getOcmById(const string& id);
	void showAllOcms();
	void setOcmSetpoint(Ocm* ch, int32_t val);
	void setVerticalResponseMatrix(double v[NumOcm*NumOcm]);
	void setHorizontalResponseMatrix(double h[NumOcm*NumOcm]);
	void setDispersionVector(double d[NumOcm]);

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

	void startAdcAcquisition();
	void stopAdcAcquisition();
	void resetAdcFifos();
	void enableAdcInterrupts();
	void disableAdcInterrupts();
	void rendezvousWithIsr();
	void rendezvousWithAdcReaders();
	void activateAdcReaders();
	void enqueueAdcData(AdcData** rdSegments);

	static rtems_task ocThreadStart(rtems_task_argument arg);
	rtems_task ocThreadBody(rtems_task_argument arg);

	static OrbitController* instance;
	OrbitControllerModeChangeCallback mcCallback;
	void* mcCallbackArg;
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

	bool initialized;
	OrbitControllerMode mode;

	//OcmController attributes
	map<string,Ocm*> ocmMap;
	rtems_id spQueueId;
	rtems_name spQueueName;
	//private struct for enqueueing OCM setpoint updates (single)
	struct SetpointMsg {
		SetpointMsg(Ocm* ocm, int32_t setpoint):ocm(ocm),sp(setpoint){}
		~SetpointMsg(){}
		Ocm* ocm;
		int32_t sp;
	};

	//BpmController attributes
	static rtems_task bpmThreadStart(rtems_task_argument arg);
	rtems_task bpmThreadBody(rtems_task_argument arg);
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
};

/* here are the ADC channel mappings based on the current drawings*/
/* these channels may be moved around in the future */
/* structure is x,y,x,y,x,y... map positions indicate the X positions ONLY ! */
const int adc0ChMap_LENGTH=4;  /* 2404.1 */
const int adc0ChMap[adc0ChMap_LENGTH] = {9*2,10*2,11*2,12*2};

const int adc1ChMap_LENGTH=14; /* 2406.1 */
const int adc1ChMap[adc1ChMap_LENGTH] = {1*2,2*2,3*2,0*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,11*2,12*2,13*2};

const int adc2ChMap_LENGTH=14; /* 2406.3 */
const int adc2ChMap[adc2ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,6*2,7*2,8*2,9*2,5*2,10*2,11*2,12*2,13*2};

/* April 28/08 BPM1408-01 replaced with Libera unit */
/* May 1/08 BPM1410-01 replaced with Libera unit */
const int adc3ChMap_LENGTH=13; /* 2408.1 */
const int adc3ChMap[adc3ChMap_LENGTH] = {1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,11*2,12*2,13*2,14*2};

const int adc4ChMap_LENGTH=9;
const int adc4ChMap[adc4ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2};

const int TOTAL_BPMS=(adc0ChMap_LENGTH + adc1ChMap_LENGTH + adc2ChMap_LENGTH + adc3ChMap_LENGTH + adc4ChMap_LENGTH);
const int NumBpmChannels=2*TOTAL_BPMS;
#endif /* ORBITCONTROLLER_H_ */
