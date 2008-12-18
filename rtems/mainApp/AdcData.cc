/*
 * AdcData.cc
 *
 *  Created on: Dec 17, 2008
 *      Author: chabotd
 */

#include <AdcData.h>

AdcData::AdcData(Ics110blModule* adc, uint32_t frames) :
	//ctor-initializer list
	numFrames(frames),bufSize(adc->getChannelsPerFrame()*numFrames)
{
	buf = new int32_t[bufSize];
}

AdcData::~AdcData() {
	delete []buf;
}
