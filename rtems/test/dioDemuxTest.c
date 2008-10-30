/*
 * DioDemuxTest.c
 *
 *  Created on: Sep 24, 2008
 *      Author: chabotd
 */
#include <stdlib.h> /*for mrand48, at least...*/
#include <math.h>

#include <vmic2536.h>
#include <tscDefs.h>

#include "tests.h"
#include "../mainApp/PSController.h"
#include "../mainApp/DaqController.h"

static VmeModule *dioArray[NumDioModules];

static DioConfig dioConfig[] = {
			{VMIC_2536_DEFAULT_BASE_ADDR,0},
			{VMIC_2536_DEFAULT_BASE_ADDR,1},
			{VMIC_2536_DEFAULT_BASE_ADDR,2},
			{VMIC_2536_DEFAULT_BASE_ADDR,3}
	#if NumDioModules==5
			,{VMIC_2536_DEFAULT_BASE_ADDR+0x10,3}
	#endif
	};

static PSController psCtlrArray[] = {
	/* id, setpoint, feedback, channel, inCorrection, crateId, modAddr, VmeModule*   */
	/* first, the horizontal controllers (OCH14xx-xx): */
	{"OCH1401-01",0,0,9,1,0,0x00700000,NULL},
//	{"SOA1401-01:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1401-02:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1401-02",0,0,12,1,0,0x00700000,NULL},
	{"OCH1402-01",0,0,13,1,0,0x00700000,NULL},
//	{"SOA1402-01:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1402-02:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1402-02",0,0,14,1,0,0x00700000,NULL},
	{"OCH1403-01",0,0,1,1,1,0x00700000,NULL},
//	{"SOA1403-01:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1403-02:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1403-02",0,0,2,1,1,0x00700000,NULL},
	{"OCH1404-01",0,0,3,1,1,0x00700000,NULL},
//	{"SOA1404-01:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1404-02:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1404-02",0,0,4,1,1,0x00700000,NULL},
	{"OCH1405-01",0,0,5,1,1,0x00700000,NULL},
//	{"SOA1405-01:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1405-02:X",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1405-02",0,0,6,1,1,0x00700000,NULL},
	{"OCH1406-01",0,0,7,1,2,0x00700000,NULL},
//	{"SOA1406-01:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1406-02:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1406-02",0,0,8,1,2,0x00700000,NULL},
	{"OCH1407-01",0,0,9,1,2,0x00700000,NULL},
//	{"SOA1407-01:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1407-02:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1407-02",0,0,12,1,2,0x00700000,NULL},
	{"OCH1408-01",0,0,13,1,2,0x00700000,NULL},
//	{"SOA1408-01:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1408-02:X",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1408-02",0,0,14,1,2,0x00700000,NULL},
	{"OCH1409-01",0,0,7,1,3,0x00700000,NULL},
//	{"SOA1409-01:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1409-02:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1409-02",0,0,8,1,3,0x00700000,NULL},
	{"OCH1410-01",0,0,9,1,3,0x00700000,NULL},
//	{"SOA1410-01:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1410-02:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1410-02",0,0,12,1,3,0x00700000,NULL},
	{"OCH1411-01",0,0,13,1,3,0x00700000,NULL},
//	{"SOA1411-01:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1411-02:X",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1411-02",0,0,14,1,3,0x00700000,NULL},
	{"OCH1412-01",0,0,7,1,0,0x00700000,NULL},
//	{"SOA1412-01:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1412-02:X",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCH1412-02",0,0,8,1,0,0x00700000,NULL},

	/* next, the vertical correctors (OCVxx-xx): */
	{"OCV1401-01",0,0,3,1,0,0x00700000,NULL},
//	{"SOA1401-01:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1401-02:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1401-02",0,0,4,1,0,0x00700000,NULL},
	{"OCV1402-01",0,0,5,1,0,0x00700000,NULL},
//	{"SOA1402-01:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1402-02:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1402-02",0,0,6,1,0,0x00700000,NULL},
	{"OCV1403-01",0,0,1,1,1,0x00700000,NULL},
//	{"SOA1403-01:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1403-02:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1403-02",0,0,2,1,1,0x00700000,NULL},
	{"OCV1404-01",0,0,3,1,1,0x00700000,NULL},
//	{"SOA1404-01:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1404-02:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1404-02",0,0,4,1,1,0x00700000,NULL},
	{"OCV1405-01",0,0,5,1,1,0x00700000,NULL},
//	{"SOA1405-01:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1405-02:Y",0,0,9,1,1,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1405-02",0,0,6,1,1,0x00700000,NULL},
	{"OCV1406-01",0,0,1,1,2,0x00700000,NULL},
//	{"SOA1406-01:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1406-02:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1406-02",0,0,2,1,2,0x00700000,NULL},
	{"OCV1407-01",0,0,3,1,2,0x00700000,NULL},
//	{"SOA1407-01:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1407-02:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1407-02",0,0,4,1,2,0x00700000,NULL},
	{"OCV1408-01",0,0,5,1,2,0x00700000,NULL},
//	{"SOA1408-01:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1408-02:Y",0,0,9,1,2,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1408-02",0,0,6,1,2,0x00700000,NULL},
	{"OCV1409-01",0,0,1,1,3,0x00700000,NULL},
//	{"SOA1409-01:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1409-02:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1409-02",0,0,2,1,3,0x00700000,NULL},
	{"OCV1410-01",0,0,3,1,3,0x00700000,NULL},
//	{"SOA1410-01:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1410-02:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1410-02",0,0,4,1,3,0x00700000,NULL},
	{"OCV1411-01",0,0,5,1,3,0x00700000,NULL},
//	{"SOA1411-01:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1411-02:Y",0,0,9,1,3,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1411-02",0,0,6,1,3,0x00700000,NULL},
	{"OCV1412-01",0,0,1,1,0,0x00700000,NULL},
//	{"SOA1412-01:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
//	{"SOA1412-02:Y",0,0,9,1,0,0x00700000,NULL},/*added SOA magnets to orbit ctl */
	{"OCV1412-02",0,0,2,1,0,0x00700000,NULL}
};

static void __InitializePSControllers(VmeCrate** crateArray) {
	int i,j;
	VmeModule **modArray = dioArray;

	/* First, initialize the vmic2536 DIO modules */
	/** Init the vmic2536 DIO modules */
	for(i=0; i<NumDioModules; i++) {
		dioArray[i] = InitializeDioModule(crateArray[dioConfig[i].vmeCrateID], dioConfig[i].baseAddr);
		syslog(LOG_INFO, "Initialized VMIC2536 DIO module[%d]\n",i);
	}

	for(i=0; i<NumDioModules; i++) {
		for(j=0; j<NumOCM; j++) {
			if((psCtlrArray[j].crateId == modArray[i]->crate->id)
					&& (psCtlrArray[j].modAddr == modArray[i]->vmeBaseAddr)) {
				psCtlrArray[j].mod = modArray[i];
				syslog(LOG_INFO, "psCtlrArray[%d]: id=%s, crateId=%d\n",
						j,psCtlrArray[j].id,psCtlrArray[j].crateId);
			}
		}
	}
}

void StartDioDemuxTest(int numIters) {
	VmeModule *dioArray[NumDioModules];
	extern double tscTicksPerSecond;
	uint64_t now, then, tmp;
	int i,j,k;
	double sum,sumSqrs,avg,stdDev, max;
	static int32_t sp[NumOCM];
	const int updatesPerIter = 100;
//	rtems_interrupt_level level;

	now=then=tmp=0;
	sum=sumSqrs=avg=stdDev=max=0.0;

	syslog(LOG_INFO, "StartAdcFifoTest() beginning...\n");
	InitializeVmeCrates(VmeCrates, numVmeCrates);

	__InitializePSControllers(VmeCrates);

	for(j=0; j<numIters; j++) {
		for(k=0; k<updatesPerIter; k++) {
			/* get some random data: */
			for(i=0; i<NumOCM; i++) {
				sp[i] = mrand48()>>8;
			}
			rdtscll(then);
			/* update the global PSController psControllerArray[NumOCM] */
			/* Demux setpoints and toggle the LATCH of each ps-channel: */
			DistributeSetpoints(sp);
			/* Finally, toggle the UPDATE for each ps-controller (4 total) */
			for(i=0; i<NumDioModules; i++) {
				ToggleUpdateBit(dioArray[i]);
			}
			rdtscll(now);
			tmp = now-then;
			sum += (double)tmp;
			sumSqrs += (double)(tmp*tmp);
			if((double)tmp>max) {
				max = (double)tmp;
			}
		}
		stdDev = (1.0/(double)(updatesPerIter))*sumSqrs - (1.0/(double)(updatesPerIter*updatesPerIter))*(sum*sum);
		stdDev = sqrt(stdDev);
		stdDev /= tscTicksPerSecond;
		avg = sum/((double)updatesPerIter);
		avg /= tscTicksPerSecond;
		max /= tscTicksPerSecond;

		syslog(LOG_INFO, "Update %d: Avg = %0.9f +/- %0.9f [s], max=%0.9f [s]\n",j,avg,stdDev,max);
		/* zero the parameters for the next iteration...*/
		sum=sumSqrs=avg=stdDev=max=0.0;
	}
}

