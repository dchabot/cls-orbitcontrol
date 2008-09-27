/*
 * DioDemuxTest.c
 *
 *  Created on: Sep 24, 2008
 *      Author: chabotd
 */
#include <stdlib.h> /*for mrand48, at least...*/
#include <math.h>

#include <tscDefs.h>

#include "tests.h"
#include "../mainApp/PSController.h"
#include "../mainApp/DaqController.h"


void StartDioDemuxTest(int numIters) {
	VmeModule *dioArray[NumDioModules];
	extern double tscTicksPerSecond;
	uint64_t now, then, tmp;
	int i,j,k;
	double sum,sumSqrs,avg,stdDev, max;
	static int32_t sp[NumOCM];
	const int updatesPerIter = 100;
//	rtems_interrupt_level level;

	now=then=tmp=0;
	sum=sumSqrs=avg=stdDev=max=0.0;

	syslog(LOG_INFO, "StartAdcFifoTest() beginning...\n");
	InitializeVmeCrates(VmeCrates, numVmeCrates);

	/** Init the vmic2536 DIO modules */
	for(i=0; i<NumDioModules; i++) {
		dioArray[i] = InitializeDioModule(VmeCrates[dioConfig[i].vmeCrateID], dioConfig[i].baseAddr);
		syslog(LOG_INFO, "Initialized VMIC2536 DIO module[%d]\n",i);
	}

	InitializePSControllers(dioArray);

	for(j=0; j<numIters; j++) {
		for(k=0; k<updatesPerIter; k++) {
			/* get some random data: */
			for(i=0; i<NumOCM; i++) {
				sp[i] = mrand48()>>8;
			}
			rdtscll(then);
			/* update the global PSController psControllerArray[NumOCM] */
			/* Demux setpoints and toggle the LATCH of each ps-channel: */
			UpdateSetPoints(sp);
			/* Finally, toggle the UPDATE for each ps-controller (4 total) */
			for(i=0; i<NumDioModules; i++) {
				ToggleUpdateBit(dioArray[i]);
			}
			rdtscll(now);
			tmp = now-then;
			sum += (double)tmp;
			sumSqrs += (double)(tmp*tmp);
			if((double)tmp>max) {
				max = (double)tmp;
			}
		}
		stdDev = (1.0/(double)(updatesPerIter))*sumSqrs - (1.0/(double)(updatesPerIter*updatesPerIter))*(sum*sum);
		stdDev = sqrt(stdDev);
		stdDev /= tscTicksPerSecond;
		avg = sum/((double)updatesPerIter);
		avg /= tscTicksPerSecond;
		max /= tscTicksPerSecond;

		syslog(LOG_INFO, "Update %d: Avg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",j,avg,stdDev,max);
		/* zero the parameters for the next iteration...*/
		sum=sumSqrs=avg=stdDev=max=0.0;
	}
}

