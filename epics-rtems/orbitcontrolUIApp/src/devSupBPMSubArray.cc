/*
 * devSupBPMSubArray.c
 *
 *  Created on: Nov 24, 2008
 *      Author: chabotd
 */

#include <subArrayRecord.h>
#include <waveformRecord.h>
#include <dbCommon.h>
#include <stdlib.h>
#include <stdio.h>
#include <devSup.h>

#include <dbDefs.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <epicsExport.h>

#include <syslog.h>

#ifdef __cplusplus
	extern "C" {
#endif

static long init_record(void* sar);
static long read_sa(void* sar);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   read_sa;
} devSupBPMSubArray={
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    read_sa,
};
epicsExportAddress(dset,devSupBPMSubArray);

typedef struct {
	waveformRecord *wfr;
	epicsUInt32 indx;
} SubArrayPvt;

static long init_record(void* sar) {
	struct subArrayRecord* psar = (subArrayRecord*)sar;
	struct dbAddr *pr;
	SubArrayPvt *pvt = NULL;

	pr = dbGetPdbAddrFromLink(&psar->inp);
	if(pr == NULL) {
		syslog(LOG_INFO,"Can't get dbAddr ptr from dbGetPdbAddrFromLink(&psar->inp)!!\n");
		return(-1);
	}

	pvt = (SubArrayPvt*)calloc(1, sizeof(SubArrayPvt));
	if(pvt == NULL) {
		printf("Can't alloc-mem for SubArrayPvt!!!\n");
		return -1;
	}
	pvt->wfr = (struct waveformRecord*)(pr->precord);
	pvt->indx = psar->indx;
	psar->dpvt = pvt;

	return 0;
}

/*
 * NOTE -- from subArrayRecord.c, readValue():
 * if (sar->indx >= sar->melm)
 * 		sar->indx = sar->malm - 1
 *
 * This is NOT what we want!
 * We just want a single value, like this:
 * 		*(sar->bptr) = waveformRecord->bptr[sar->indx]
 *
 * So, since readValue() is going to overwrite our record's indx value,
 * we hack around it by recording the originally assigned indx
 * in a struct SubArrayPvt, along with the wfr pointer... :-(
 */
static long read_sa(void* sar) {
	struct subArrayRecord* psar = (subArrayRecord*)sar;
	SubArrayPvt *pvt = (SubArrayPvt*)psar->dpvt;
	double *psarBuf = (double*)psar->bptr;
	double *wfrBuf = (double*)pvt->wfr->bptr;

	/*copy value into our buffer*/
	*psarBuf = wfrBuf[pvt->indx];

	psar->nord = psar->nelm;
	return 0;
}

#ifdef __cplusplus
}
#endif
