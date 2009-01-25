/*
 * AdcReader.h
 *
 *  Created on: Dec 12, 2008
 *      Author: chabotd
 */

#ifndef ADCREADER_H_
#define ADCREADER_H_

#include <rtems.h>
#include <Ics110blModule.h>
#include <AdcData.h>


class AdcReader {
public:
	AdcReader(Ics110blModule* adc, rtems_id bid);
	virtual ~AdcReader();
	void start(rtems_task_argument);
	void read(AdcData* data);
	AdcData* getAdcData() const { return data;	}

private:
	AdcReader();
	AdcReader(const AdcReader&);
	AdcReader& operator=(const AdcReader&);

	/* we need a static method here 'cause we need to omit the "this"
	 * pointer from rtems_task_start(tid,threadStart,arg), a native c-function.
	 */
	static rtems_task threadStart(rtems_task_argument arg);
	rtems_task threadBody(rtems_task_argument arg);
	rtems_task_argument arg;
	rtems_task_priority priority;
	rtems_id tid;
	rtems_name threadName;
	rtems_id barrierId;
	int instance;
	Ics110blModule* adc;
	static const rtems_event_set readEvent=1;
	AdcData *data;
};

#endif /* ADCREADER_H_ */
