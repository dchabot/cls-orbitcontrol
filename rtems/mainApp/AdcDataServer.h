#ifndef ADCDATASERVER_H_
#define ADCDATASERVER_H_


#define AdcDataServerName			rtems_build_name('A','S','R','V')
#define ProcessedDataQueueName 		rtems_build_name('P','R','C','q')
#define AdcDataServerPort			24742

void StartAdcDataServer(void);
void DestroyAdcDataServer(void);

#endif /*ADCDATASERVER_H_*/
