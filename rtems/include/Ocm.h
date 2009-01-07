/*
 * Ocm.h
 *
 *  Created on: Dec 22, 2008
 *      Author: chabotd
 */

#ifndef OCM_H_
#define OCM_H_

#include <stdint.h>
#include <Vmic2536Module.h>

/*#define DAC_AMP_CONV_FACTOR 42234.31*/
#define DAC_AMP_CONV_FACTOR 1

#define OUTPUT_REG_OFFSET 0x08
#define INPUT_REG_OFFSET 0x04
#define CONTROL_REG_OFFSET 0x02

#define PS_CHANNEL_OFFSET 24
#define PS_CHANNEL_MASK 0xF

#define PS_LATCH 0x20000000
//#define DROP_PS_LATCH 0xDFFFFFFF

#define UPDATE 0x80000000
//#define DROP_UPDATE 0x7FFFFFFF

const int NumOcm=48;

class Ocm {
public:
	Ocm(const char* pvPrefix, Vmic2536Module* module, uint8_t chan);
	virtual ~Ocm();
	bool isInCorrection() const;
	void includeInCorrection();
	void omitFromCorrection();
	void setSetpoint(int32_t sp);
	void activateSetpoint();
	int32_t getSetpoint() const;
	const char* getId() const;
	uint8_t getDelay() const;
	void setDelay(uint8_t usec);

private:
	Ocm();
	Ocm(const Ocm&);
	const Ocm& operator=(const Ocm&);

	Vmic2536Module* mod;
	uint8_t channel;
	const char* id;
	bool inCorrection;
	int32_t setpoint;
	int32_t feedback;
	uint8_t delay; //opto-isolator on/off delay
};

inline bool Ocm::isInCorrection() const {
	return inCorrection;
}

inline void Ocm::includeInCorrection() {
	inCorrection = true;
}

inline void Ocm::omitFromCorrection() {
	inCorrection = false;
}

inline int32_t Ocm::getSetpoint() const {
	return setpoint;
}

inline const char* Ocm::getId() const {
	return id;
}

inline uint8_t Ocm::getDelay() const {
	return delay;
}

inline void Ocm::setDelay(uint8_t usec) {
	delay=usec;
}
#endif /* OCM_H_ */
