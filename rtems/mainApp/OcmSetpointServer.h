#ifndef OCMSETPOINTSERVER_H_
#define OCMSETPOINTSERVER_H_

#include <rtems.h>

#define OcmSetpointServerName			rtems_build_name('O','C','M','r')
#define OcmSetpointServerPort			24745

#define OcmSetpointQueueName			rtems_build_name('O','C','M','q')

void  StartOcmSetpointServer(void *arg);
void DestroyOcmSetpointServer(void);

#endif /*OCMSETPOINTSERVER_H_*/
