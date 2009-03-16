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
#include <string>
using std::string;


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

//FIXME -- let the UI determine these #s!!! Ditto for # of BPMs.
const uint32_t NumOcm=48;
const uint32_t NumHOcm=NumOcm/2;
const uint32_t NumVOcm=NumOcm/2;

enum ocmType {HORIZONTAL=1,VERTICAL=0,UNKNOWN=-1};

class Ocm {
public:
	Ocm(const string& str,Vmic2536Module* module,uint8_t chan);
	~Ocm() { }
	bool isEnabled() const { return enabled; }
	void setEnabled(bool b) { enabled=b; }
	void setSetpoint(int32_t sp);
	void activateSetpoint();
	int32_t getSetpoint() const { return setpoint; }
	string getId() const { return id; }
	uint8_t getDelay() const { return delay; }
	void setDelay(uint8_t usec) { delay=usec; }
	uint32_t getPosition() const { return position; }
	void setPosition(uint32_t p) { position=p; }
	Vmic2536Module* getModule() const { return mod; }

private:
	Ocm();
	Ocm(const Ocm&);
	const Ocm& operator=(const Ocm&);

	Vmic2536Module* mod;
	uint8_t channel;
	const string id;
	bool enabled;
	int32_t setpoint;
	int32_t feedback;
	uint8_t delay; //opto-isolator on/off delay
	uint32_t position;
};

#endif /* OCM_H_ */
