#ifndef DIOREADSERVER_H_
#define DIOREADSERVER_H_

#include <rtems.h>
#include "DataHandler.h"

#define BpmSamplesPerAvgServerName			rtems_build_name('B','P','M','s')
#define BpmSamplesPerAvgServerPort			24750

void  StartBpmSamplesPerAvgServer(void *arg);
void DestroyBpmSamplesPerAvgServer(void);

#endif /*DIOREADSERVER_H_*/
