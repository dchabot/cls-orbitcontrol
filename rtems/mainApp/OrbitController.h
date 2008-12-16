/*
 * OrbitController.h
 *
 *  Created on: Dec 11, 2008
 *      Author: chabotd
 */

#ifndef ORBITCONTROLLER_H_
#define ORBITCONTROLLER_H_

#include <Ics110blModule.h>
#include <rtems.h>
#include <stdint.h>


const rtems_task_priority OrbitControllerPriority=50;
const uint32_t NumAdcModules = 4;


class OrbitController {
public:
	OrbitController();
	virtual ~OrbitController();

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

	rtems_id isrBarrierId;
	rtems_name isrBarrierName;
	rtems_id rdrBarrierId;
	rtems_name rdrBarrierName;
};

#endif /* ORBITCONTROLLER_H_ */
