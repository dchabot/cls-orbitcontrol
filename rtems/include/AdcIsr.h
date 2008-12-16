/*
 * AdcIsr.h
 *
 *  Created on: Dec 16, 2008
 *      Author: chabotd
 */

#ifndef ADCISR_H_
#define ADCISR_H_

#include <Ics110blModule.h>
#include <rtems.h>

class AdcIsr {
public:
	AdcIsr(Ics110blModule* mod, rtems_id id);
	~AdcIsr();
	rtems_id getBarrierId() const;
	Ics110blModule* getAdc() const;

private:
	AdcIsr();
	AdcIsr(const AdcIsr&);
	static void isr(void* arg, uint8_t vector);
	rtems_id bid;
	Ics110blModule* adc;
};

inline rtems_id AdcIsr::getBarrierId() const {
	return bid;
}

inline Ics110blModule* AdcIsr::getAdc() const {
	return adc;
}
#endif /* ADCISR_H_ */
