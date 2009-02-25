/*
 * OrbitControlException.cc
 *
 *  Created on: Jan 5, 2009
 *      Author: chabotd
 */

#include <OrbitControlException.h>

void TestDirective(rtems_status_code rc, const char* str) {
	if(rc != RTEMS_SUCCESSFUL) {
		//fatal
		char msg[256];
		snprintf(msg,sizeof(msg),"%s -- %s",str,rtems_status_text(rc));
		throw OrbitControlException(msg);
	}
}
