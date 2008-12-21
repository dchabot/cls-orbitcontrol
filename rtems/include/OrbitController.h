/*
 * OrbitController.h
 *
 *  Created on: Dec 11, 2008
 *      Author: chabotd
 */

#ifndef ORBITCONTROLLER_H_
#define ORBITCONTROLLER_H_

#include <VmeCrate.h>
#include <Ics110blModule.h>
#include <Vmic2536Module.h>
#include <AdcReader.h>
#include <AdcIsr.h>
#include <AdcData.h>
#include <DataHandler.h>
//if we're using <vector>, in seems like we're req'd to drag in <iterator> as well :-(
#include <vector>
#include <iterator>
#include <rtems.h>
#include <stdint.h>

using std::vector;


const rtems_task_priority OrbitControllerPriority=50;
const uint32_t NumAdcModules = 4;
const uint32_t NumAdcReaders = 4;
const uint32_t NumVmeCrates = 4;

/**
 * A Singleton class for managing the storage-ring orbit control system.
 *
 * After obtaining an object instance via OrbitController::getInstance(),
 * the initialize() method should be invoked prior to calling start(dbl rate).
 *
 * If initialize() is NOT called prior to start(), the ICS-110BL ADCz will be configured
 * with a default sample-rate (aka frame-rate) of 10 kHz.
 *
 * Don't forget to call destroyInstance() to release all resources!!
 */
class OrbitController {
public:
	static OrbitController* getInstance();
	void initialize(const double adcSampleRate);
	void start(rtems_task_argument arg);
	void destroyInstance();

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

	static rtems_task threadStart(rtems_task_argument arg);
	rtems_task threadBody(rtems_task_argument arg);

	static OrbitController* instance;
	rtems_id tid;
	rtems_name threadName;
	rtems_task_argument arg;
	rtems_task_priority priority;

	rtems_interval rtemsTicksPerSecond;
	uint32_t adcFramesPerTick;

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
};

#endif /* ORBITCONTROLLER_H_ */
