/*
 * PSController.h
 *
 *  Created on: Sep 21, 2008
 *      Author: djc
 */

#ifndef PSCONTROLLER_H_
#define PSCONTROLLER_H_

#include <stdlib.h>
#include <stdint.h>

#include <vmeDefs.h>

#define PSCONTROLLER_ID_SIZE 40

typedef struct {
	char id[PSCONTROLLER_ID_SIZE]; /* eg: use the EPICS record instance id here */
	int32_t setpoint;
	int32_t feedback;
	uint8_t channel;
	uint8_t inCorrection;
	uint32_t crateId;
	VmeModule *mod;
}PSController;

typedef struct {
	uint32_t numsp;
	int32_t *buf;
}spMsg;

#define NumOCM 48*2

/*int getId(PSController* ctlr, char** id);
int setId(PSController* ctlr, char** id);

int setSetpoint(PSController* ctlr, int32_t sp);

int getFeedback(PSController* ctlr, int32_t* fbk);*/

int getChannel(PSController* ctlr, uint8_t* ch);
int setChannel(PSController* ctlr, uint8_t ch);

int isInCorrection(PSController* ctlr, uint8_t* answer);

void UpdateSetPoints(int32_t* spBuf);
void ToggleUpdateBit(VmeModule* mod);
void InitializePSControllers(VmeModule** modArray);
#endif /* PSCONTROLLER_H_ */
