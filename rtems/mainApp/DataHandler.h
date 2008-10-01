#ifndef DATAHANDLER_H_
#define DATAHANDLER_H_

#include <rtems.h>
#include <stdint.h>

#define DataHandlerThreadName 		rtems_build_name('D','H','T','N')
#define RawDataQueueName 			rtems_build_name('R','A','W','q')

rtems_id StartDataHandler(uint32_t numReaderThreads);
uint32_t getMaxMsgs(void);

#endif /*DATAHANDLER_H_*/
