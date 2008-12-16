/*
 * AdcDataSegment.h
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
class AdcDataSegment {
public:
	AdcDataSegment(Ics110blModule& adc, uint32_t numFrames);
	~AdcDataSegment();
	int32_t* getBuffer() const;
	uint32_t getFrames() const;
	uint32_t getBufferSize() const;

private:
	AdcDataSegment();
	AdcDataSegment(const AdcDataSegment&);
	uint32_t numFrames;
	const uint32_t bufSize;
	int32_t *buf;
};

AdcDataSegment::AdcDataSegment(Ics110blModule& adc, uint32_t frames) :
	//ctor-initializer list
	numFrames(frames),bufSize(adc.getChannelsPerFrame()*numFrames)
{
	buf = new int32_t[bufSize];
}

AdcDataSegment::~AdcDataSegment() {
	delete []buf;
}

/**
 * @return the number of ADC frames this segment is sized for
 */
inline uint32_t AdcDataSegment::getFrames() const {
	return numFrames;
}
/**
 * @return buffer size in number of channels may contain
 */
inline uint32_t AdcDataSegment::getBufferSize() const {
	return bufSize;
}
/**
 * @return a pointer to the buffer
 */
inline int32_t* AdcDataSegment::getBuffer() const {
	return buf;
}
#endif /* ADCDATASEGMENT_H_ */
