#ifndef TESTS_H_
#define TESTS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include <vmeDefs.h>
#include <utils.h>

#define numVmeCrates 4
#define numAdcModules 4
static VmeCrate *VmeCrates[numVmeCrates];
static VmeModule *AdcModules[numAdcModules];

#include "../mainApp/DaqController.h"
static DioConfig dioConfig[] = {
		{VMIC_2536_DEFAULT_BASE_ADDR,0},
		{VMIC_2536_DEFAULT_BASE_ADDR,1},
		{VMIC_2536_DEFAULT_BASE_ADDR,2},
		{VMIC_2536_DEFAULT_BASE_ADDR,3}
#if NumDioModules==5
		,{VMIC_2536_DEFAULT_BASE_ADDR+0x10,3}
#endif
};

#if 0
static void InitializeVmeCrates(void) {
	int i;

	for(i=0; i<numVmeCrates; i++) {
		int fd;
		char devName[64];

		VmeCrates[i].id = i;
		sprintf(devName, "/dev/sis1100_%d",i);
		fd = open(devName, O_RDWR, 0);
		if(fd < 0) {
			syslog(LOG_INFO, "Failed to open %s: %s\n",devName, strerror(fd));
			FatalErrorHandler(0);
		}
		VmeCrates[i].fd = fd;
	}
}

static void ShutdownVmeCrates(void) {
	int i;

	for(i=0; i<numVmeCrates; i++) {
		VmeCrates[i].id = 0;
		close(VmeCrates[i].fd);
	}
}
#endif
#endif /*TESTS_H_*/
