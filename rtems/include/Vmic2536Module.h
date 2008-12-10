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

#define VMIC2536MODULE_USE_SINGLE_CYCLE_VME_ACCESS

class Vmic2536Module : public VmeModule {
public:
	Vmic2536Module(VmeCrate& c, uint32_t vmeAddr);
	virtual ~Vmic2536Module();
	void initialize() const;
	uint16_t getControl() const;
	void setControl(uint16_t val) const;
	uint32_t getOutput() const;
	void setOutput(uint32_t val) const;
	uint32_t getInput() const;
	void setInput(uint32_t val) const;
	uint16_t getId() const;
};

#endif /* VMIC2536MODULE_H_ */
