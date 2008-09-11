#ifndef DIOWRITESERVER_H_
#define DIOWRITESERVER_H_

#include <rtems.h>

#define DioWriteServerName			rtems_build_name('D','S','V','w')
#define DioWriteServerPort			24743

#define NumPowerSupplyControllers	48+3 /* 48 + 3 for the Chicane */

void StartDioWriteServer(void *arg);
void DestroyDioWriteServer(void);

#endif /*DIOWRITESERVER_H_*/
