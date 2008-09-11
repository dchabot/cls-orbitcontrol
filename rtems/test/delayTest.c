#include <tscDefs.h>
#include <math.h>
#include <syslog.h>

void StartDelayTest(int numIters) {
	extern double tscTicksPerSecond;
	void usecSpinDelay(uint32_t);
	uint32_t delays[] = {1,2,4,5,7,10,13,15,20,25,30,35,40,45,50,60,70,80,90,100};
	uint64_t now,then,tmp;
	double sum,sumSqrs,avg,stdDev, max;
	int i,j,numTests;
//	rtems_interrupt_level level;
	
	now=then=tmp=0;
	sum=sumSqrs=avg=stdDev=max=0.0;
	numTests = sizeof(delays)/sizeof(delays[0]);
	
//	rtems_interrupt_disable(level);
	
	for(i=0; i<numTests; i++) {
		for(j=0; j<numIters; j++) {
			rdtscll(then);
			//Delay8254Timer1(delays[i]);
			usecSpinDelay(delays[i]);
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
		
		syslog(LOG_INFO, "Delay = %d [us], Avg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",delays[i],avg,stdDev,max);
		/* zero the parameters for the next iteration...*/
		sum=sumSqrs=avg=stdDev=max=0.0;
	}
//	rtems_interrupt_enable(level);
}
