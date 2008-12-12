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

typedef struct {
	uint32_t numFrames;
	int32_t *buf;
}RawDataSegment;

class AdcReader {
public:
	AdcReader(Ics110blModule& adc);
	virtual ~AdcReader();
	void start(rtems_task_argument);
	rtems_id getQueueId() const;
	void read(RawDataSegment* ds);
	int getInstance() const;
	rtems_id getThreadId() const;

private:
	AdcReader();
	AdcReader(const AdcReader&);
	AdcReader& operator=(const AdcReader&);

	/* we need a static method here 'cause we need to omit the "this"
	 * pointer from rtems_task_start(), a native c-function.
	 */
	static rtems_task threadStart(rtems_task_argument arg);
	rtems_task threadBody(rtems_task_argument arg);
	rtems_task_argument arg;
	rtems_task_priority priority;
	rtems_id tid;
	rtems_name threadName;
	rtems_id qid;
	rtems_name queueName;
	rtems_id barrierId;
	int instance;
	Ics110blModule& adc;
};


inline rtems_id AdcReader::getQueueId() const {
	return qid;
}
#endif /* ADCREADER_H_ */
