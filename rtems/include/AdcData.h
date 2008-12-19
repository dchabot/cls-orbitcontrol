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
	AdcData(Ics110blModule* adc, uint32_t numFrames);
	virtual ~AdcData();
	int32_t* getBuffer() const;
	uint32_t getFrames() const;
	uint32_t getBufferSize() const;

private:
	AdcData();
	AdcData(const AdcData&);
	const uint32_t numFrames;
	const uint32_t bufSize;
	int32_t *buf;
};

/**
 * @return the number of ADC frames this segment is sized for
 */
inline uint32_t AdcData::getFrames() const {
	return numFrames;
}

/**
 * @return buffer size in number of channels contain (4 bytes/channel)
 */
inline uint32_t AdcData::getBufferSize() const {
	return bufSize;
}
/**
 * @return a pointer to the buffer
 */
inline int32_t* AdcData::getBuffer() const {
	return buf;
}
#endif /* ADCDATASEGMENT_H_ */
