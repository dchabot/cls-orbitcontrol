#ifndef DATAHANDLER_H_
#define DATAHANDLER_H_

#include <rtems.h>
#include <stdint.h>

#define DataHandlerThreadName 		rtems_build_name('D','H','T','N')
#define RawDataQueueName 			rtems_build_name('R','A','W','q')

#define SamplesPerAvg	5000 /* 0.5 seconds */
#define ShiftFactor		256.0 /* (num>>8)==num/256 */

rtems_id StartDataHandler(uint32_t numReaderThreads);
uint32_t getMaxMsgs(void);

#endif /*DATAHANDLER_H_*/
