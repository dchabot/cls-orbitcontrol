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
#include <ics110bl.h>

#define ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS

class Ics110blModule : public VmeModule {
public:
	Ics110blModule(VmeCrate* c, uint32_t vmeAddr);
	virtual ~Ics110blModule();

// FIXME -- make this a class attribute with accessor/mutator:
// something like void (*init)(void*), thereby permitting different initialization schemes...
	void initialize(double requestedFrameRate, int clockingSelect, int acquireSelect);
	void programFoxRate(uint32_t foxWord);
	void programAdcSpi(ICS110B_ADC_SPI * arg);
	void calibrateAdc(int overSamplingRate, double* pSampleRate);
//as a side-effect, sets Ics110blModule::trueFrameRate
	void setFrameRate(double requestRate,uint8_t overSamplingRate);
	void resetDemodulator();
	void calcFoxWord(double* reqRate, long* progWord);
	void setMaster(int master);
	void setAcquisitionTriggerSource(int src);
	void setClockSource(int src);
	void setOutputMode(uint16_t mode);
	void setClockSelect(int clk);
	void setOversamplingRate(uint8_t rate);
	void setChannelsPerFrame(uint8_t ch);
	uint8_t getChannelsPerFrame() const;
	double getFrameRate() const;
	void setHPF(int overSamplingRate, bool isEnabled);
	int readFifo(uint32_t* buffer, uint32_t wordsRequested, uint32_t* wordsRead) const;
	void reset();
	void resetFifo();
//these are impls of the virtual methods in VmeModule:
	void setIrqVector(uint8_t vector);
	uint8_t getIrqVector() const;
	void setIrqLevel(uint8_t level);
	uint8_t getIrqLevel() const;
//
	void enableInterrupt();
	void disableInterrupt();
	void startAcquisition();
	void stopAcquisition();
	uint16_t getStatus();
	bool isFifoEmpty();
	bool isFifoFull();
	bool isFifoHalfFull();
	bool isInitialized() const;
	bool isAcquiring() const;

private:
	Ics110blModule();
	Ics110blModule(const Ics110blModule&);

	static void sleep(double secs);
	double trueFrameRate;//in kHz
	static const double crossoverFrameRate = 54.0; /* kHz. Where oversampling rate is cut in half (128x->64x) */
	uint8_t channelsPerFrame;//[2,32] mod(2)
	uint8_t overSamplingRate; //either 64 or 128 times...
	bool initialized;
	bool acquiring;
};

/** The VME IRQ level is set via jumper and not accessible from software */
inline void Ics110blModule::setIrqLevel(uint8_t level) {
	irqLevel = level;
}

inline uint8_t Ics110blModule::getIrqLevel() const {
	return irqLevel;
}

inline uint8_t Ics110blModule::getChannelsPerFrame() const {
	return channelsPerFrame;
}

inline double Ics110blModule::getFrameRate() const {
	return trueFrameRate;
}

inline bool Ics110blModule::isInitialized() const {
	return initialized;
}

inline bool Ics110blModule::isAcquiring() const {
	return acquiring;
}
#endif /* ICS110BLMODULE_H_ */
