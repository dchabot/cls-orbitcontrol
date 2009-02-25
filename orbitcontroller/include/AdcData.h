/*
 * AdcData.h
 *
 *  Created on: Dec 12, 2008
 *      Author: chabotd
 */

#ifndef ADCDATASEGMENT_H_
#define ADCDATASEGMENT_H_

#include <stdint.h>
#include <Ics110blModule.h>

/**
 * Data-type returned by Ics110blModules.
 */
class AdcData {
public:
	AdcData(Ics110blModule* adc, uint32_t frames) :
		//ctor-initializer list
		numFrames(frames),
		channelsPerFrame(adc->getChannelsPerFrame()),
		bufSize(adc->getChannelsPerFrame()*numFrames)
	{
		buf = new int32_t[bufSize];
	}
	~AdcData() { delete []buf; }
	int32_t* getBuffer() const { return buf; }
	uint32_t getFrames() const { return numFrames; }
	uint32_t getChannelsPerFrame() const { return channelsPerFrame; }
	uint32_t getBufferSize() const { return bufSize; }

private:
	AdcData();
	AdcData(const AdcData&);
	const uint32_t numFrames;
	const uint32_t channelsPerFrame;
	const uint32_t bufSize;
	int32_t *buf;
};

#endif /* ADCDATASEGMENT_H_ */
