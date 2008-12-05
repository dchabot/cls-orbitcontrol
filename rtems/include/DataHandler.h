#ifndef DATAHANDLER_H_
#define DATAHANDLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rtems.h>
#include <stdint.h>

#define DataHandlerThreadName 		rtems_build_name('D','H','T','N')
#define RawDataQueueName 			rtems_build_name('R','A','W','q')

rtems_id StartDataHandler(uint32_t numReaderThreads);
uint32_t getMaxMsgs(void);

#ifdef __cplusplus
}
#endif

#endif /*DATAHANDLER_H_*/
