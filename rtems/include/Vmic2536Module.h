/*
 * Vmic2536Module.h
 *
 *  Created on: Dec 8, 2008
 *      Author: djc
 */

#ifndef VMIC2536MODULE_H_
#define VMIC2536MODULE_H_

#include <VmeModule.h>
#include <stdint.h>

class Vmic2536Module : public VmeModule {
public:
	Vmic2536Module(VmeCrate& c, uint32_t vmeAddr);
	virtual ~Vmic2536Module();
};

#endif /* VMIC2536MODULE_H_ */
