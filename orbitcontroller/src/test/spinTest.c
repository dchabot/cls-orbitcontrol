#include <stdint.h>
#include <syslog.h>
#include <math.h>

#include <tscDefs.h>
#include <utils.h>


void StartSpinTest(int numIters) {
	extern double tscTicksPerSecond;
	void usecSpinDelay(uint32_t);
	uint32_t delays[] = {50000,100000,150000,200000,250000,300000,350000,400000,450000,500000};
	uint64_t now,then,tmp;
	double sum,sumSqrs,avg,stdDev, max;
	int i,j,k,numTests;
	rtems_interrupt_level level;
	
	now=then=tmp=0;
	sum=sumSqrs=avg=stdDev=max=0.0;
	numTests = sizeof(delays)/sizeof(delays[0]);
	
	rtems_interrupt_disable(level);
	
	for(i=0; i<numTests; i++) {
		for(j=0; j<numIters; j++) {
			rdtscll(then);

			/* spin... */
			for(k=0; k<delays[i]; k++);
			
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
		
		syslog(LOG_INFO, "loop count = %d, Avg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",delays[i],avg,stdDev,max);
		/* zero the parameters for the next iteration...*/
		sum=sumSqrs=avg=stdDev=max=0.0;
	}
	rtems_interrupt_enable(level);
	
}
