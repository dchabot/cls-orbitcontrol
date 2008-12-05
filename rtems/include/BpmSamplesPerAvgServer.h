#ifndef BPMSSAMPLESPERAVGSERVER_
#define BPMSSAMPLESPERAVGSERVER_

#ifdef __cplusplus
extern "C" {
#endif

#include <rtems.h>
#include "DataHandler.h"

#define BpmSamplesPerAvgServerName			rtems_build_name('B','P','M','s')
#define BpmSamplesPerAvgServerPort			24750

void  StartBpmSamplesPerAvgServer(void *arg);
void DestroyBpmSamplesPerAvgServer(void);

#ifdef __cplusplus
}
#endif

#endif /*BPMSSAMPLESPERAVGSERVER_*/
