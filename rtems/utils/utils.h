#ifndef UTILS_H_
#define UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <rtems.h>
#include <rtems/error.h>

void usecSpinDelay(uint32_t usecDelay);
void FindTscTicksPerSecond(uint32_t numSamples);

typedef void (*FatalErrorCallback)(void);

void FatalErrorHandler(FatalErrorCallback cb);
int TestDirective(rtems_status_code rc, const char* msg);

#ifdef __cplusplus
}
#endif

#endif /*UTILS_H_*/
