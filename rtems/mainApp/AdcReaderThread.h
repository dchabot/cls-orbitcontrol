#ifndef ADCREADERTHREAD_H_
#define ADCREADERTHREAD_H_

#include <rtems.h>
#include <vmeDefs.h>

typedef struct {
	rtems_id readerTID;
	rtems_id controllerTID;
	rtems_event_set syncEvent;
	rtems_id rawDataQID;
	VmeModule *adc;
}ReaderThreadArg;

#define NumReaderThreads 4

rtems_task ReaderThread(rtems_task_argument arg);
ReaderThreadArg*  startReaderThread(VmeModule *mod, rtems_event_set syncEvent);

#endif /*ADCREADERTHREAD_H_*/
