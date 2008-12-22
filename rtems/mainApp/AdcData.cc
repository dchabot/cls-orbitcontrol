/*
 * AdcData.cc
 *
 *  Created on: Dec 17, 2008
 *      Author: chabotd
 */

#include <AdcData.h>
#include <syslog.h>


AdcData::AdcData(Ics110blModule* adc, uint32_t frames) :
	//ctor-initializer list
	numFrames(frames),
	bufSize(adc->getChannelsPerFrame()*numFrames)
{
	buf = new int32_t[bufSize];
}

AdcData::~AdcData() {
	syslog(LOG_INFO, "AdcData dtor!!\n");
	delete []buf;
}
