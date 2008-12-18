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
#include <AdcReader.h>
#include <AdcIsr.h>
#include <AdcData.h>
#include <DataHandler.h>
//if we're using <vector>, in seems like we're req'd to drag in <iterator> as well :-(
#include <vector>
#include <iterator>
#include <rtems.h>
#include <stdint.h>




const rtems_task_priority OrbitControllerPriority=50;
const uint32_t NumAdcModules = 4;
const uint32_t NumVmeCrates = 4;


class OrbitController {
public:
	OrbitController();
	~OrbitController();
	void start();

private:
	/* we need a static method here 'cause we need to omit the "this"
	 * pointer from rtems_task_start(tid,threadStart,arg), a native c-function.
	 */
	static rtems_task threadStart(rtems_task_argument arg);
	rtems_task threadBody(rtems_task_argument arg);
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

	std::vector<VmeCrate*> crateArray;
	std::vector<Ics110blModule*> adcArray;
	std::vector<AdcIsr*> isrArray;
	std::vector<AdcReader*> rdrArray;
	std::vector<AdcData*> rdSegments;
};

#endif /* ORBITCONTROLLER_H_ */
