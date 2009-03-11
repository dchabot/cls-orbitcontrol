/*
 * devSupOrbitControllerMbbio.cc
 *
 *  Created on: Jan 14, 2009
 *      Author: chabotd
 */

#include <mbboRecord.h>
#include <mbbiRecord.h>
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
#include <Command.h>
#include <string>
using std::string;

/* some routines called from "C": avoid name-mangling */
#ifdef __cplusplus
	extern "C" {
#endif

static long init_mbbi_record(void*);
static long read_mbbi(void*);
static long init_mbbo_record(void*);
static long write_mbbo(void*);

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   read_mbbi;
} devSupOrbitControllerMbbi={
    5,
    NULL,
    NULL,
    init_mbbi_record,
    NULL,
    read_mbbi,
};

struct {
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   write_mbbo;
} devSupOrbitControllerMbbo={
    5,
    NULL,
    NULL,
    init_mbbo_record,
    NULL,
    write_mbbo,
};

epicsExportAddress(dset,devSupOrbitControllerMbbi);
epicsExportAddress(dset,devSupOrbitControllerMbbo);

static void onOrbitControllerModeChange(void* arg) {
	int ev = (int)arg;
	post_event(ev);
}

static long init_mbbi_record(void* mbbir) {
	mbbiRecord *mbbip = (mbbiRecord*)mbbir;

	if (mbbip->inp.type != INST_IO) {
		syslog(LOG_INFO, "%s: INP field type should be INST_IO\n", mbbip->name);
		return(S_db_badField);
	}

	if(mbbip->evnt != 0) {
		OrbitController *oc = OrbitController::getInstance();
		Command* cmd = new Command(onOrbitControllerModeChange,(void*)mbbip->evnt);
		oc->registerForModeEvents(cmd);
	}

	return 0;
}

static long read_mbbi(void* mbbir) {
	mbbiRecord *mbbip = (mbbiRecord*)mbbir;
	OrbitController *oc = OrbitController::getInstance();

	mbbip->rval = (epicsUInt32)oc->getMode();
	syslog(LOG_INFO, "%s: OrbitController state is now %s\n",mbbip->name,
						(char*)((mbbip->zrst)+sizeof(mbbip->zrst)*mbbip->rval));
	return 0;
}

/********* mbbo record definitions *****************************************/

static long init_mbbo_record(void* mbbor) {
	mbboRecord *mbbop = (mbboRecord*)mbbor;

	if (mbbop->out.type != INST_IO) {
		syslog(LOG_INFO, "%s: OUT field type should be INST_IO\n", mbbop->name);
		return(S_db_badField);
	}

	return 0;
}

static long write_mbbo(void* mbbor) {
	mbboRecord *mbbop = (mbboRecord*)mbbor;
	OrbitController *oc = OrbitController::getInstance();

	syslog(LOG_INFO, "%s: setting OrbitController mode to %s\n",mbbop->name,
			(char*)((mbbop->zrst)+sizeof(mbbop->zrst)*mbbop->rval));
	oc->setMode((OrbitControllerMode)mbbop->rval);
	mbbop->rbv = mbbop->rval;
	return 0;
}

#ifdef __cplusplus
}
#endif
