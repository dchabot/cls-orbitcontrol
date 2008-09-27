#include <stdlib.h>
#include <syslog.h>

#include <bsp.h> /* rtemsReboot() */
#include <utils.h>
#include <rtems-gdb-stub.h> /*rtems_gdb_breakpoint()*/

void FatalErrorHandler(FatalErrorCallback cb) {
	rtems_interval tps;

	if(cb) {
		(*cb)();
	}
	else {
		rtems_gdb_breakpoint();
		//exit(1);
		syslog(LOG_INFO, "FatalErrorHandler()... rebooting system\n");
		/* give the syslogger a chance to send the msg... */
		rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &tps);
		rtems_task_wake_after(5*tps);
		rtemsReboot();
	}
}

int TestDirective(rtems_status_code rc, const char* msg) {
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO, "Directive, %s, failed: %s\n",msg,rtems_status_text(rc));
		return -1;
	}
	return 0;
}
