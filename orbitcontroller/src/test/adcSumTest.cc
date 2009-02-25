#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <math.h>

#include <tscDefs.h>
#include <utils.h>

const uint32_t AdcChannelsPerFrame=32;

/* here are the ADC channel mappings based on the current drawings*/
/* these channels may be moved around in teh future */
/* structure is x,y,x,y,x,y... map positions indicate the X positions ONLY ! */
#define adc0ChMap_LENGTH 4  /* 2404.1 */
int adc0ChMap[adc0ChMap_LENGTH] = {9*2,10*2,11*2,12*2};

#define adc1ChMap_LENGTH 14 /* 2406.1 */
int adc1ChMap[adc1ChMap_LENGTH] = {1*2,2*2,3*2,0*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,11*2,12*2,13*2};

#define adc2ChMap_LENGTH 14 /* 2406.3 */
int adc2ChMap[adc2ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,6*2,7*2,8*2,9*2,5*2,10*2,11*2,12*2,13*2};

/* April 28/08 BPM1408-01 replaced with Libera unit */
/* May 1/08 BPM1410-01 replaced with Libera unit */
#define adc3ChMap_LENGTH 13 /* 2408.1 */
int adc3ChMap[adc3ChMap_LENGTH] = {1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,11*2,12*2,13*2,14*2};

#define adc4ChMap_LENGTH 9
int adc4ChMap[adc4ChMap_LENGTH] = {0*2,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2};

#define TOTAL_BPMS      (adc0ChMap_LENGTH + adc1ChMap_LENGTH + adc2ChMap_LENGTH + adc3ChMap_LENGTH + adc4ChMap_LENGTH)
#define NumBpmChannels  2*TOTAL_BPMS


//#define LOWPASS_FILTER_FACTOR   1.015
//#define mmPerMeter              1000
#include <ics110bl.h>   /* need the definition of ADC_PER_VOLT */
#include <bpmDefs.h>


static void scaleBPMAverages(double* buf, uint32_t numSamples)
{
	int i;
	const int mmPerMeter = 1000;
	/* accounts for 24-bits of adc-data stuffed into the 3 MSB of a 32-bit word*/
	const int ShiftFactor = (1<<8);
	/* accounts for the voltage-divider losses of an LPF aft of each Bergoz unit */
	const double LPF_Factor = 1.015;
	double mmScaleFactor = LPF_Factor/(numSamples*ShiftFactor*ADC_PER_VOLT*mmPerMeter);

    for(i=0; i<TOTAL_BPMS; i++) {
		buf[i] *= (mmScaleFactor/XBPM_convFactor[i]);
		buf[i+TOTAL_BPMS] *= (mmScaleFactor/YBPM_convFactor[i]);
	}
}

/*
 * This routine will sort the BPM data from the internal representation
 * to a single array in (storage ring) cell-order, with the top-half of
 * the array occupied by horizontal (x) BPM values and the bottom-half
 * occupied by vertical (y) values.
 */
static void sortBPMData(double *sortedArray, double *rawArray) {
	int i,j;
	int nthAdc = 0;

	for(i=0,j=0; j<adc0ChMap_LENGTH; i++,j++) {
		/* bpmX*/
		sortedArray[i] = rawArray[nthAdc+adc0ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc0ChMap[j]+1];
	}
    nthAdc += AdcChannelsPerFrame;
    for(j=0; j<adc1ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc1ChMap[j]];
		/* bpmY */
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc1ChMap[j]+1];
	}
    nthAdc += AdcChannelsPerFrame;
    for(j=0; j<adc2ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc2ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc2ChMap[j]+1];
	}
    nthAdc += AdcChannelsPerFrame;
    for(j=0; j<adc3ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc3ChMap[j]];
		/* bpmY*/
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc3ChMap[j]+1];
	}
    nthAdc = 0;
    for(j=0; j<adc4ChMap_LENGTH; i++,j++) {
		/* bpmX */
		sortedArray[i] = rawArray[nthAdc+adc4ChMap[j]];
		/* bpmY */
		sortedArray[i+TOTAL_BPMS] = rawArray[nthAdc+adc4ChMap[j]+1];
	}
}


typedef struct {
	uint32_t numFrames;
	int32_t *buf;
}DataSegment;

extern "C" void adcSumTest(int numIters) {
	extern double tscTicksPerSecond;
	uint64_t now,then,tmp;
	double sum,sumSqrs,avg,stdDev, max;
	int i,j,k,numTests;
	rtems_interrupt_level level;
	const uint32_t NumAdcModules=4;
	static DataSegment rdSegments[NumAdcModules];
	static double sums[NumAdcModules*AdcChannelsPerFrame];
	static double sumsSqrd[NumAdcModules*AdcChannelsPerFrame];
	static double sorted[NumAdcModules*AdcChannelsPerFrame];

	now=then=tmp=0;
	sum=sumSqrs=avg=stdDev=max=0.0;
	numTests = 1;//sizeof(delays)/sizeof(delays[0]);

	for(i=0; i<NumAdcModules; i++) {
		rdSegments[i].buf = (int32_t*)calloc(1,sizeof(int32_t)*16*1024);
		if(rdSegments[i].buf == NULL) {
			syslog(LOG_INFO,"Couldn't allocate mem for buffer!!\n");
			return;
		}
		//populate the buffer
		for(j=0; j<16*1024; j++) {
			rdSegments[i].buf[j]=mrand48();
		}
		rdSegments[i].numFrames = (16*1024)/AdcChannelsPerFrame;
	}

	//rtems_interrupt_disable(level);

	for(i=0; i<numTests; i++) {
		for(j=0; j<numIters; j++) {
			/* test algorithm */
			uint32_t nthFrame,nthAdc,nthChannel;
			int32_t numSamplesSummed=0;
			memset(sums,0,sizeof(sums));
			rdtscll(then);
			/* XXX -- assumes each rdSegment has an equal # of frames... */
			for(nthFrame=0; nthFrame<rdSegments[0].numFrames; nthFrame++) { /* for each ADC frame in rdSegment[].buf... */
				int nthFrameOffset = nthFrame*AdcChannelsPerFrame;

				for(nthAdc=0; nthAdc<NumAdcModules; nthAdc++) { /* for each ADC... */
					int nthAdcOffset = nthAdc*AdcChannelsPerFrame;

					for(nthChannel=0; nthChannel<AdcChannelsPerFrame; nthChannel++) { /* for each channel of this frame... */
						sums[nthAdcOffset+nthChannel] += (double)(rdSegments[nthAdc].buf[nthFrameOffset+nthChannel]);
						sumsSqrd[nthAdcOffset+nthChannel] += pow(sums[nthAdcOffset+nthChannel],2);
					}

				}
				numSamplesSummed++;
			}
			sortBPMData(sorted, (double*)sums);
			scaleBPMAverages(sorted, numSamplesSummed);
			sortBPMData(sorted, (double*)sumsSqrd);
			scaleBPMAverages(sorted, numSamplesSummed);
			rdtscll(now);

			tmp = now-then;
			sum += (double)tmp;
			sumSqrs += (double)(tmp*tmp);
			if((double)tmp>max) {
				max = (double)tmp;
			}
		}
		stdDev = (1.0/(double)(numIters))*sumSqrs - (1.0/(double)(numIters*numIters))*(sum*sum);
		stdDev = sqrt(stdDev);
		stdDev /= tscTicksPerSecond;
		avg = sum/((double)numIters);
		avg /= tscTicksPerSecond;
		max /= tscTicksPerSecond;

		syslog(LOG_INFO, "Avg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",avg,stdDev,max);
		/* zero the parameters for the next iteration...*/
		sum=sumSqrs=avg=stdDev=max=0.0;
	}
	//rtems_interrupt_enable(level);

	for(i=0; i<NumAdcModules; i++) {
		free(rdSegments[i].buf);
	}
}
