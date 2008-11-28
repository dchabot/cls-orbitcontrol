#ifndef ORBITCONTROLLER_H_
#define ORBITCONTROLLER_H_

#include <stdint.h>
#include <rtems.h>
#include <vmeDefs.h>
#include <sis1100_api.h>

#define DefaultPriority 50
#define OrbitControllerThreadPriority DefaultPriority
#define OrbitControllerThreadName		rtems_build_name('D','A','Q','C')

/*
#define NumDaqStates 4
enum DaqStates {STOPPED,INITIALIZING,ACQUIRING,CORRECTING};
static char *DaqStateNames[] = {"STOPPED","INITIALIZING","ACQUIRING","CORRECTING"};
typedef struct {
	int state;
	char *stateName;
} DaqState;
*/

#define NumVmeCrates 4
#define NumAdcModules 4

#define AdcChannelsPerFrame 32
#define HzPerkHz 1000.0

#define AdcDefaultFrequency 10.0


/* prototypes */
void InitializeVmeCrates(VmeCrate *crateArray[], int numCrates);
void ShutdownVmeCrates(VmeCrate *crateArray[], int numCrates);
VmeModule* InitializeAdcModule(VmeCrate* vmeCrate,uint32_t baseAddr,double targetFrameRate,int numChannelsPerFrame,double *trueFrameRate);
void ShutdownAdcModules(VmeModule *modArray[], int numModules);
void AdcStartAcquisition(VmeModule *modArray[], int numModules);
void AdcStopAcquisition(VmeModule *modArray[], int numModules);
void AdcInstallIsr(VmeModule *mod, sis1100VmeISR isr, void *isrArg);
void AdcRemoveIsr(VmeModule *mod);
void StartOrbitController(rtems_task_entry entryPoint);

rtems_task OrbitControllerIrq(rtems_task_argument arg);

#endif /*ORBITCONTROLLER_H_*/
