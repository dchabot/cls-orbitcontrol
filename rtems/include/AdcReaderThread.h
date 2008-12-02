#ifndef ADCREADERTHREAD_H_
#define ADCREADERTHREAD_H_

#include <rtems.h>
#include <vmeDefs.h>

typedef struct {
	rtems_id readerTID;
	rtems_id barrierID;
	rtems_event_set syncEvent;
	rtems_id rawDataQID;
	VmeModule *adc;
}ReaderThreadArg;

#define NumReaderThreads 4

ReaderThreadArg*  startReaderThread(VmeModule *mod, rtems_event_set syncEvent);

#endif /*ADCREADERTHREAD_H_*/
