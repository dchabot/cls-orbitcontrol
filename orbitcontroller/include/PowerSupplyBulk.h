/*
 * PowerSupplyBulk.h
 *
 *  Created on: Dec 22, 2008
 *      Author: chabotd
 */

#ifndef POWERSUPPLYBULK_H_
#define POWERSUPPLYBULK_H_

#include <stdint.h>
#include <Vmic2536Module.h>
#include <utils.h>

class PowerSupplyBulk {
public:
	PowerSupplyBulk(Vmic2536Module* m):mod(m),delay(35) { }
	~PowerSupplyBulk() { }
	void updateSetpoints() const;
	uint32_t getDelay() const { return delay; }
	void setDelay(uint32_t microsec) {  delay=microsec; }

private:
	PowerSupplyBulk();
	PowerSupplyBulk(const PowerSupplyBulk&);
	const PowerSupplyBulk& operator=(const PowerSupplyBulk&);

	Vmic2536Module* mod;
	uint32_t delay;
};

#endif /* POWERSUPPLYBULK_H_ */
