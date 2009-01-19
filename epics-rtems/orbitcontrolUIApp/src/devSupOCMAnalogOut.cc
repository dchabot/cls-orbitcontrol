/*
 * devSupOCMAnalogOut.cc
 *
 *  Created on: Jan 15, 2009
 *      Author: chabotd
 */

#include <aoRecord.h>
#include <longoutRecord.h>
#include <dbCommon.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <devSup.h>

#include <dbDefs.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>
#include <epicsExport.h>
#include <syslog.h>
#include <OrbitController.h>
#include <string>
#include <stdexcept>
using std::runtime_error;
using std::string;

/* some routines called from "C": avoid name-mangling */
#ifdef __cplusplus
	extern "C" {
#endif

static long init_record(void* aor);
static long write_ao(void* aor);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   write_ao;
    DEVSUPFUN	special_linconv;
} devSupOCMAnalogOut={
    6,
    NULL,
    NULL,
    init_record,
    NULL,
    write_ao,
    NULL
};

epicsExportAddress(dset,devSupOCMAnalogOut);

static long init_record(void* aor) {
	return 0;
}
static long write_ao(void* aor) {
	return 0;
}

#ifdef __cplusplus
}
#endif
