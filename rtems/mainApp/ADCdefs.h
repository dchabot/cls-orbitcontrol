#ifndef ADCDEFS_H_
#define ADCDEFS_H_

#include <vmeDefs.h>
/*#include <110bl_utils.h>
#include <vmeMapMem.h>
*/
/* Addressing information */
//extern base_addrs_st *board_map[MAX_NUM_SIS1100_BOARDS][MAX_NUM_BOARDS];

#define ADCPORTNUM 24725 /* used in the messageSender for setting up the socket */

/*	These values are for the number of ADCS and associated workers in the system.*/
#define NUM_ADCS 				4		/* Value determined by configuration fo hardware */
#define NUM_ADCWorkers			NUM_ADCS
//#define NUM_ADC_CHANNELS		5
#define ADC_CHANNELS_PER_FRAME	10
#define ADC_SAMPLINGRATE		10.0	/* 10.0kHz */
#define ADC_BOARD_ID            0
#define ADC_BASE				0x00D00000 /* Value determined by configuration fo hardware */

#define VoltsPerMilliMeter		1.261
#define VoltsPerMeter			VoltsPerMilliMeter * 1000

/* defines used for doing halffull transfers */
#define SamplesPerHalfFIFO 580
#define HalfFIFOSize SamplesPerHalfFIFO * NUM_ADC_CHANNELS

/* values for the timing of acquisition. if changing these make sure your new values are valid, because they will make you life difficult if they are wrong */
#define SamplesPerPeriod			1000 /* num of adc samples ControllerPeriodTicks */
#define ControllerPeriodTicks		100 /* == 0.1 sec, if tickspersecond==1000... */

#endif /*ADCDEFS_H_*/
