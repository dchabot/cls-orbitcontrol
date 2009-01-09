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
#include <DataHandler.h>
#include <Ocm.h>
#include <vector>
#include <rtems.h>
#include <stdint.h>

using std::vector;


const rtems_task_priority OrbitControllerPriority=50;
const uint32_t NumAdcModules = 4;
const uint32_t NumAdcReaders = 4;
const uint32_t NumVmeCrates = 4;
//FIXME -- this should be implemented as a class, 'cause c++ enums suck ass :-(
enum OrbitControllerMode {ASSISTED=0,AUTONOMOUS=1};



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
class OrbitController : public OcmController {
public:
	static OrbitController* getInstance();
	void initialize(const double adcSampleRate=10.1);
	void start(rtems_task_argument arg);
	void destroyInstance();
	double getAdcFrameRateSetpoint() const;
	double getAdcFrameRateFeedback() const;
	OrbitControllerMode getMode() const;
	void setMode(OrbitControllerMode mode);

	//virtual methods inherited from abstract base-class OcmController:
	void setOcmSetpoint(Ocm* ch, int32_t val);
	int32_t getOcmSetpoint(Ocm *ch);
	void updateAllOcmSetpoints();
	void registerOcm(Ocm* ch);
	void unregisterOcm(Ocm* ch);
	void setVerticalResponseMatrix(double v[NumOcm][NumOcm]);
	void setHorizontalResponseMatrix(double h[NumOcm][NumOcm]);

	//interface for access to internal BpmController object
	BpmController* getBpmController() const;
	void setBpmController(BpmController* ctlr);

private:
	OrbitController();
	OrbitController(const OrbitController&);
	const OrbitController& operator=(const OrbitController&);
	~OrbitController();

	void startAdcAcquisition();
	void stopAdcAcquisition();
	void resetAdcFifos();
	void enableAdcInterrupts();
	void rendezvousWithIsr();
	void rendezvousWithAdcReaders();
	void activateAdcReaders();

	//private struct for enqueueing OCM setpoint updates (single)
	struct SetpointMsg {
		SetpointMsg(Ocm* ocm, int32_t setpoint):ocm(ocm),sp(setpoint){}
		~SetpointMsg(){}
		Ocm* ocm;
		int32_t sp;
	};

	static rtems_task threadStart(rtems_task_argument arg);
	rtems_task threadBody(rtems_task_argument arg);

	static OrbitController* instance;
	rtems_id tid;
	rtems_name threadName;
	rtems_task_argument arg;
	rtems_task_priority priority;

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
	AdcData* rdSegments[NumAdcModules];

	bool initialized;
	OrbitControllerMode mode;

	rtems_id spQueueId;
	rtems_name spQueueName;
};


inline double OrbitController::getAdcFrameRateSetpoint() const {
	return adcFrameRateSetpoint;
}

inline double OrbitController::getAdcFrameRateFeedback() const {
	return adcFrameRateFeedback;
}

inline OrbitControllerMode OrbitController::getMode() const {
	return mode;
}

inline void OrbitController::setMode(OrbitControllerMode mode) {
	this->mode = mode;
}
#endif /* ORBITCONTROLLER_H_ */
