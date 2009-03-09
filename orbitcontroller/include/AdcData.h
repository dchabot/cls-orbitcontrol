/*
 * AdcData.h
 *
 *  Created on: Dec 12, 2008
 *      Author: chabotd
 */

#ifndef ADCDATASEGMENT_H_
#define ADCDATASEGMENT_H_

#include <rtems.h>
#include <stdint.h>
#include <OrbitControlException.h>


/**
 * Data-type returned by AdcReaders.
 */
class AdcData {
public:
	AdcData(rtems_id id, uint32_t chPerFrame, uint32_t frames) :
		//ctor-initializer list
		bufId(id),
		numFrames(frames),
		channelsPerFrame(chPerFrame),
		bufSize(channelsPerFrame*numFrames)
	{
		rtems_status_code rc = rtems_region_get_segment(bufId,bufSize*sizeof(int32_t),
									RTEMS_NO_WAIT,RTEMS_NO_TIMEOUT,(void**)&buf);
		TestDirective(rc,"AdcData: failure obtaining buffer");
	}
	~AdcData() {
		rtems_status_code rc = rtems_region_return_segment(bufId,buf);
		TestDirective(rc, "AdcData: failure returning buffer");
	}
	int32_t* getBuffer() const { return buf; }
	uint32_t getFrames() const { return numFrames; }
	uint32_t getChannelsPerFrame() const { return channelsPerFrame; }
	uint32_t getBufferSize() const { return bufSize; }

private:
	AdcData();
	AdcData(const AdcData&);

	const rtems_id bufId;
	const uint32_t numFrames;
	const uint32_t channelsPerFrame;
	const uint32_t bufSize;
	int32_t *buf;
};

#endif /* ADCDATASEGMENT_H_ */
