/*
 * ocmDefs.h
 *
 *  Created on: Sep 21, 2008
 *      Author: djc
 */

#ifndef OCMDEFS_H_
#define OCMDEFS_H_

#include <PSController.h>

#define NumOCM 48
/* ocmSetpointArray is populated via the OcmSetPointServer */
static int32_t ocmSetpointArray[NumOCM];
/* FIXME -- unused */
static int32_t ocmFeedbackArray[NumOCM];

static PSController psCtlrArray[] = {
	/* first, the horizontal controllers (OCH14xx-xx): */
	{"OCH1401-01",0,0,/*channel*/,1,1},
	{"OCH1401-02",0,0,/*channel*/,1,1},
	{"OCH1402-01",0,0,/*channel*/,1,1},
	{"OCH1402-02",0,0,/*channel*/,1,1},
	{"OCH1403-01",0,0,/*channel*/,1,1},
	{"OCH1403-02",0,0,/*channel*/,1,1},
	{"OCH1404-01",0,0,/*channel*/,1,2},
	{"OCH1404-02",0,0,/*channel*/,1,2},
	{"OCH1405-01",0,0,/*channel*/,1,2},
	{"OCH1405-02",0,0,/*channel*/,1,2},
	{"OCH1406-01",0,0,/*channel*/,1,2},
	{"OCH1406-02",0,0,/*channel*/,1,2},
	{"OCH1407-01",0,0,/*channel*/,1,3},
	{"OCH1407-02",0,0,/*channel*/,1,3},
	{"OCH1408-01",0,0,/*channel*/,1,3},
	{"OCH1408-02",0,0,/*channel*/,1,3},
	{"OCH1409-01",0,0,/*channel*/,1,3},
	{"OCH1409-02",0,0,/*channel*/,1,3},
	{"OCH1410-01",0,0,/*channel*/,1,4},
	{"OCH1410-02",0,0,/*channel*/,1,4},
	{"OCH1411-01",0,0,/*channel*/,1,4},
	{"OCH1411-02",0,0,/*channel*/,1,4},
	{"OCH1412-01",0,0,/*channel*/,1,4},
	{"OCH1412-02",0,0,/*channel*/,1,4},

	/* next, the vertical correctors (OCVxx-xx): */
	{"OCV1401-01",0,0,/*channel*/,1,1},
	{"OCV1401-02",0,0,/*channel*/,1,1},
	{"OCV1402-01",0,0,/*channel*/,1,1},
	{"OCV1402-02",0,0,/*channel*/,1,1},
	{"OCV1403-01",0,0,/*channel*/,1,1},
	{"OCV1403-02",0,0,/*channel*/,1,1},
	{"OCV1404-01",0,0,/*channel*/,1,2},
	{"OCV1404-02",0,0,/*channel*/,1,2},
	{"OCV1405-01",0,0,/*channel*/,1,2},
	{"OCV1405-02",0,0,/*channel*/,1,2},
	{"OCV1406-01",0,0,/*channel*/,1,2},
	{"OCV1406-02",0,0,/*channel*/,1,2},
	{"OCV1407-01",0,0,/*channel*/,1,3},
	{"OCV1407-02",0,0,/*channel*/,1,3},
	{"OCV1408-01",0,0,/*channel*/,1,3},
	{"OCV1408-02",0,0,/*channel*/,1,3},
	{"OCV1409-01",0,0,/*channel*/,1,3},
	{"OCV1409-02",0,0,/*channel*/,1,3},
	{"OCV1410-01",0,0,/*channel*/,1,4},
	{"OCV1410-02",0,0,/*channel*/,1,4},
	{"OCV1411-01",0,0,/*channel*/,1,4},
	{"OCV1411-02",0,0,/*channel*/,1,4},
	{"OCV1412-01",0,0,/*channel*/,1,4},
	{"OCV1412-02",0,0,/*channel*/,1,4}
};

#endif /* OCMDEFS_H_ */
