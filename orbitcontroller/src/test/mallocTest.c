/*
 * mallocTest.c
 *
 *  Created on: Mar 5, 2009
 *      Author: chabotd
 */
#include <tscDefs.h>
#include <rtems.h>
#include <stdlib.h>
#include <math.h>
#include <syslog.h>

void StartMallocTest(int numIters) {
	extern double tscTicksPerSecond;
	void usecSpinDelay(uint32_t);
	uint64_t now,then,tmp;
	double sum,sumSqrs,avg,stdDev, max;
	int i,j,k;
	int* ia[4];
//	rtems_interrupt_level level;

	now=then=tmp=0;
	sum=sumSqrs=avg=stdDev=max=0.0;

//	rtems_interrupt_disable(level);

	for(i=0; i<1; i++) {
		for(j=0; j<numIters; j++) {
			rdtscll(then);
			for(k=0; k<4; k++) {
				ia[k] = (int*)calloc(1,3200*sizeof(int));
			}
			rdtscll(now);
			for(k=0; k<4; k++) { free(ia[k]); }

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

		syslog(LOG_INFO, "Malloc Test -- Avg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",avg,stdDev,max);
		/* zero the parameters for the next iteration...*/
		sum=sumSqrs=avg=stdDev=max=0.0;
	}
//	rtems_interrupt_enable(level);
}
