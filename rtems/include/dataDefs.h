#ifndef DATADEFS_H_
#define DATADEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <vmeDefs.h>

typedef struct {
	VmeModule *adc;
	uint32_t numChannelsPerFrame;
	uint32_t numFrames;
	int32_t *buf;
}RawDataSegment;

#define ProcessedDataQueueCapacity 10
typedef struct {
	uint32_t numElements;
	double *buf;
}ProcessedDataSegment;

typedef struct {
	uint32_t 	crateID;
	uint32_t 	vmeBaseAddr;
	uint32_t 	modOffset;
	uint32_t 	mask;
}DioRequestMsg;

typedef struct {
	int 		numBytes;
	uint32_t 	data;
}DioResponseMsg;

typedef struct {
	uint32_t 	crateID;
	uint32_t 	vmeBaseAddr;
	uint32_t 	modOffset;
	uint32_t 	pwrSupChan;
	int32_t  	pwrSupData;
}DioWriteMsg;

#ifdef __cplusplus
}
#endif

#endif /*DATADEFS_H_*/
