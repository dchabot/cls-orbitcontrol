/*
 * PSController.h
 *
 *  Created on: Sep 21, 2008
 *      Author: djc
 */

#ifndef PSCONTROLLER_H_
#define PSCONTROLLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <vmeDefs.h>

/*#define DAC_AMP_CONV_FACTOR 42234.31*/
#define DAC_AMP_CONV_FACTOR 1

#define OUTPUT_REG_OFFSET 0x08
#define INPUT_REG_OFFSET 0x04
#define CONTROL_REG_OFFSET 0x02

#define PS_CHANNEL_OFFSET 24
#define PS_CHANNEL_MASK 0xF

#define PS_LATCH 0x20000000
#define DROP_PS_LATCH 0xDFFFFFFF

#define UPDATE 0x80000000
#define DROP_UPDATE 0x7FFFFFFF

#define ISO_DELAY 30 /* i.e. delay in microseconds for writes to optically isolated circuits */

#define PSCONTROLLER_ID_SIZE 40

typedef struct {
	char id[PSCONTROLLER_ID_SIZE]; /* eg: use the EPICS record instance id here */
	int32_t setpoint;
	int32_t feedback;
	uint8_t channel;
	bool inCorrection;
	uint32_t crateId;
	uint32_t modAddr;
	VmeModule *mod;
}PSController;

#define OcmSetpointQueueCapacity	1
typedef struct {
	uint32_t numsp;
	int32_t *buf;
}spMsg;

#define NumOCM 48

/* number of DIO modules will vary:
 * 	experimental config uses 4,
 * 	production config uses 5
 */
#define NumDioModules 4

typedef struct {
	uint32_t baseAddr;
	uint32_t vmeCrateID;
} DioConfig;


/*int getId(PSController* ctlr, char** id);
int setId(PSController* ctlr, char** id);

int setSetpoint(PSController* ctlr, int32_t sp);

int getFeedback(PSController* ctlr, int32_t* fbk);*/

int getChannel(PSController* ctlr, uint8_t* ch);
int setChannel(PSController* ctlr, uint8_t ch);

bool isInCorrection(PSController* ctlr);

PSController* getPSControllerByName(char* ctlrName);
void DistributeSetpoints(int32_t* spBuf);
void ToggleUpdateBit(VmeModule* mod);
void ToggleUpdateBits();
void SimultaneousSetpointUpdate(int32_t *spBuf);
int SetSingleSetpoint(PSController* ctlr, int32_t setpoint);
VmeModule* InitializeDioModule(VmeCrate* vmeCrate, uint32_t baseAddr);
void ShutdownDioModules(VmeModule *DioModules[], int numModules);
void InitializePSControllers(VmeCrate** crateArray);

#ifdef __cplusplus
}
#endif

#endif /* PSCONTROLLER_H_ */
