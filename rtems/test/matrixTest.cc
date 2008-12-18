#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <math.h>

#include <tscDefs.h>
#include <utils.h>


extern "C" void StartMatrixTest(int numIters) {
	extern double tscTicksPerSecond;
	void usecSpinDelay(uint32_t);
	//uint32_t dims[] = {10,20,30,40,50,60,70,80,90,100};
	uint64_t now,then,tmp;
	double sum,sumSqrs,avg,stdDev, max;
	int i,j,k,l,numTests;
	rtems_interrupt_level level;

	now=then=tmp=0;
	sum=sumSqrs=avg=stdDev=max=0.0;
	numTests = 1;//sizeof(dims)/sizeof(dims[0]);

	//rtems_interrupt_disable(level);

	for(i=0; i<numTests; i++) {
		static double r[100][100];
		static double b[100];
		static double a[100];
		for(j=0; j<numIters; j++) {

			//populate the matrix,r and vector,b
			for(k=0; k<100; k++) {
				for(l=0; l<100; l++) {
					r[k][l] = drand48()*1000;
				}
				b[k] = drand48()*1000;
				a[k] = 0;
			}
			rdtscll(then);

			/* do the matrix multiplication */
			for(k=0; k<100; k++) {
				for(l=0; l<100; l++) {
					a[k] += r[k][l]*b[l];
				}
			}

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

}
