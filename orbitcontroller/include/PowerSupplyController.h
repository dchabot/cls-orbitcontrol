/*
 * PowerSupplyController.h
 *
 *  Created on: Apr 20, 2009
 *      Author: chabotd
 */

#ifndef POWERSUPPLYCONTROLLER_H_
#define POWERSUPPLYCONTROLLER_H_

#include <OrbitController.h>
#include <vector>
using std::vector;

class OrbitController;

class PowerSupplyController {
public:
	PowerSupplyController(Vmic2536Module* m) : mod(m) { }
	~PowerSupplyController() { }
	void setChannel(Ocm*,int32_t);
	void raiseLatch(Ocm*);
	void lowerLatch(Ocm*);
	void raiseUpdate();
	void lowerUpdate();
	//sort hocm and vocm by ring-order (i.e. geographically)
	void sortOcm(vector<Ocm*>&);

private:
	PowerSupplyController();
	PowerSupplyController(const PowerSupplyController&);
	const PowerSupplyController& operator=(const PowerSupplyController&);

	friend class OrbitController;

	Vmic2536Module* mod;
	//the horizontal and vertical OCM channels associated with this controller
	vector<Ocm*> hocm;
	vector<Ocm*> vocm;
};

#endif /* POWERSUPPLYCONTROLLER_H_ */
