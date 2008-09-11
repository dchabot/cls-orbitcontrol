#ifndef DIOREADSERVER_H_
#define DIOREADSERVER_H_

#include <rtems.h>

#define DioReadServerName			rtems_build_name('D','S','V','r')
#define DioReadServerPort			24744

void  StartDioReadServer(void *arg);
void DestroyDioReadServer(void);

#endif /*DIOREADSERVER_H_*/
