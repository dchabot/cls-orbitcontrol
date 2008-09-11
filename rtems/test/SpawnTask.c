#include <rtems.h>
#include <utils.h>
#include <syslog.h>


rtems_task dudThread(rtems_task_argument arg) {
	
	syslog(LOG_INFO, "dudThread: started...\n");
	rtems_task_wake_after(2000);
	syslog(LOG_INFO, "dudThread: exiting...\n");
	rtems_task_delete(RTEMS_SELF);
}

void SpawnThread(void) {
	rtems_name threadName = rtems_build_name('D','u','d','1');
	rtems_id tid;
	rtems_status_code rc;
	
	rc = rtems_task_create(threadName,
							200 /*priority*/,
							RTEMS_MINIMUM_STACK_SIZE*32,
							RTEMS_FLOATING_POINT|RTEMS_LOCAL,/*XXX floating point context req'd ??*/
							RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0),
							&tid);
	TestDirective(rc, "rtems_task_create()");
	rc = rtems_task_start(tid, dudThread, 0);
	TestDirective(rc, "rtems_task_start()");
}
