/*
 * PowerSupplyBulk.h
 *
 *  Created on: Dec 22, 2008
 *      Author: chabotd
 */

#ifndef POWERSUPPLYBULK_H_
#define POWERSUPPLYBULK_H_

#include <Ocm.h>

class PowerSupplyBulk {
public:
	PowerSupplyBulk(Ocm* ch);
	virtual ~PowerSupplyBulk();
	/**
	 * activateSetpoint() will toggle the UPDATE bit of a power-supply bulk,
	 * causing all (current?) setpoints to take effect.
	 */
	void activateSetpoint() const;

private:
	PowerSupplyBulk();
	PowerSupplyBulk(const PowerSupplyBulk&);
	const PowerSupplyBulk& operator=(const PowerSupplyBulk&);

	Ocm* psch;
};

#endif /* POWERSUPPLYBULK_H_ */
