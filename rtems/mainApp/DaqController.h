#ifndef DAQCONTROLLER_H_
#define DAQCONTROLLER_H_

#include <stdint.h>
#include <rtems.h>
#include <vmeDefs.h>
#include <sis1100_api.h>
#include <vmic2536.h>

#define DefaultPriority 50
#define DaqThreadPriority DefaultPriority
#define DaqControllerThreadName		rtems_build_name('D','A','Q','C')

#define NumDaqStates 4
enum DaqStates {STOPPED,INITIALIZING,ACQUIRING,CORRECTING};
static char *DaqStateNames[] = {"STOPPED","INITIALIZING","ACQUIRING","CORRECTING"};
typedef struct {
	int state;
	char *stateName;
} DaqState;

#define NumVmeCrates 4
#define NumAdcModules 4

#define AdcChannelsPerFrame 32
#define HzPerkHz 1000.0

#define AdcDefaultFrequency 10.0

#define NumDioModules 5
/* number of DIO modules will vary:
 * 	experimental config uses 4,
 * 	production config uses 5
 */
typedef struct {
	uint32_t baseAddr;
	uint32_t vmeCrateID;
} DioConfig;

static DioConfig dioConfig[] = {
		{VMIC_2536_DEFAULT_BASE_ADDR,0},
		{VMIC_2536_DEFAULT_BASE_ADDR,1},
		{VMIC_2536_DEFAULT_BASE_ADDR,2},
		{VMIC_2536_DEFAULT_BASE_ADDR,3}
#if NumDioModules==5
		,{VMIC_2536_DEFAULT_BASE_ADDR+0x10,3}
#endif
};


/* prototypes */
void InitializeVmeCrates(VmeCrate *crateArray[], int numCrates);
void ShutdownVmeCrates(VmeCrate *crateArray[], int numCrates);
VmeModule* InitializeAdcModule(VmeCrate* vmeCrate,uint32_t baseAddr,double targetFrameRate,int numChannelsPerFrame,double *trueFrameRate);
void ShutdownAdcModules(VmeModule *modArray[], int numModules);
void AdcStartAcquisition(VmeModule *modArray[], int numModules);
void AdcStopAcquisition(VmeModule *modArray[], int numModules);
void AdcInstallIsr(VmeModule *mod, sis1100VmeISR isr, void *isrArg);
void AdcRemoveIsr(VmeModule *mod);
VmeModule* InitializeDioModule(VmeCrate* vmeCrate, uint32_t baseAddr);
void ShutdownDioModules(VmeModule *DioModules[], int numModules);
void StartDaqController(rtems_task_entry entryPoint);

/*rtems_task daqControllerPeriodic(rtems_task_argument arg);*/
rtems_task daqControllerIrq(rtems_task_argument arg);

#endif /*DAQCONTROLLER_H_*/
