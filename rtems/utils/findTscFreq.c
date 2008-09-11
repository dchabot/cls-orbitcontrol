#include <tscDefs.h>

#include <stdint.h>
#include <stdlib.h>
#include <rtems.h>
#include <rtems/error.h>
#include <rtems/rtems/timer.h>
#include <syslog.h>

double tscTicksPerSecond=0;
double tscSdevTicksPerSecond=0;
double tscSdomTicksPerSecond=0;

typedef struct  {
	ssize_t bufsize;
	uint32_t *buf;
	rtems_id taskID;
}TscTestData;

static rtems_id createTimer(void) {
	rtems_status_code rc;
	rtems_id timerID = 0;
	rtems_name timerName = rtems_build_name('T','M','R','1');
	
	rc = rtems_timer_create(timerName, &timerID);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "rtems_timer_create(): %s\n", rtems_status_text(rc));
	}
	return timerID;
}

static void startTimer(rtems_id timerID,
						rtems_interval fireWhen,
						rtems_timer_service_routine_entry userFunc,
						void* userArg) {
	rtems_status_code rc;
	
	rc = rtems_timer_fire_after(timerID, fireWhen, userFunc, userArg);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "rtems_timer_fire_after(): %s\n", rtems_status_text(rc));
	}
}

static void destroyTimer(rtems_id timerID) {
	rtems_status_code rc;
	
	rc = rtems_timer_delete(timerID);
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "rtems_timer_delete(): %s\n", rtems_status_text(rc));
	}
}


static rtems_timer_service_routine 
timerService(rtems_id id, void* arg) {
	static int n;
	static uint64_t then;
	uint64_t now;
	TscTestData *argp = (TscTestData *)arg;
	
	rdtscll(now);
	argp->buf[n++] = (uint32_t)(now-then);
	then = now;
	if(n < argp->bufsize) {
		rtems_timer_reset(id);
	}
	else {
		rtems_event_send(argp->taskID, RTEMS_EVENT_13);
		destroyTimer(id);
	}
}

static void 
getTimingInfo(void * arg) {
	TscTestData *argp = (TscTestData *)arg;
	rtems_interval tps;
	int i;
	double avg = 0.0;
	double avgSqrs = 0.0;
	double stdDev = 0.0;
	double sdom = 0.0;
	/*extern double tscTicksPerSecond; 
	extern double tscSdevTicksPerSecond;
	extern double tscSdomTicksPerSecond;*/
	
	/* skip the first delta: it'll be garbage anyway... */
	for(i=1; i<argp->bufsize; i++) {
		double tmp = (double)argp->buf[i];
		avg += tmp;
		avgSqrs += tmp*tmp;
	}
#include <math.h>
	stdDev = (1.0/(double)(i-2))*(avgSqrs - (1.0/(double)(i-1))*(avg*avg));
	stdDev = sqrt(stdDev);
	avg /= (double)(i-1);
	sdom = stdDev/sqrt((double)(i-1));
	
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND,&tps);
	syslog(LOG_INFO,"CPU frequency: avg=%.3f sdom=%.3f sigma=%.3f\n", avg*tps, sdom*tps, stdDev*tps);
	/* set the globally accessible value in timeDefs.h */
	tscTicksPerSecond = avg*tps;
	tscSdevTicksPerSecond = stdDev*tps;
	tscSdomTicksPerSecond = sdom*tps;
}

void FindTscTicksPerSecond(uint32_t numSamples) {
	TscTestData tscTestData = {0};
	rtems_id timerID;
	
	/* begin... */
	syslog(LOG_INFO, "Determining TSC ticks per second...\n");
	
	rtems_task_ident(RTEMS_SELF, RTEMS_LOCAL, &tscTestData.taskID);
	timerID = createTimer();
	tscTestData.buf = (uint32_t *)malloc(numSamples*sizeof(uint32_t));
	if(tscTestData.buf == NULL) {
		syslog(LOG_INFO, "Can't allocate bmData.buf !!");
		return;
	}
	tscTestData.bufsize = numSamples;
	startTimer(timerID, 1/*  ticks*/,  timerService, (void *)&tscTestData);
	
	rtems_event_set eventSet = 0;
	rtems_event_receive(RTEMS_EVENT_13, RTEMS_WAIT|RTEMS_EVENT_ANY, 0, &eventSet);
	//dump timing info to syslog... 
	getTimingInfo(&tscTestData);
	free(tscTestData.buf);
}
