/*
 * OrbitController.h
 *
 *  Created on: Dec 11, 2008
 *      Author: chabotd
 */

#ifndef ORBITCONTROLLER_H_
#define ORBITCONTROLLER_H_

#include <rtems.h>

const rtems_task_priority OrbitControllerPriority=50;

class OrbitController {
public:
	OrbitController();
	virtual ~OrbitController();
};

#endif /* ORBITCONTROLLER_H_ */
