/*
 * Ics110blModule.h
 *
 *  Created on: Dec 8, 2008
 *      Author: djc
 */

#ifndef ICS110BLMODULE_H_
#define ICS110BLMODULE_H_

#include <VmeModule.h>
#include <stdint.h>

class Ics110blModule : public VmeModule {
public:
	Ics110blModule(VmeCrate& c, uint32_t vmeAddr);
	virtual ~Ics110blModule();
};

#endif /* ICS110BLMODULE_H_ */
