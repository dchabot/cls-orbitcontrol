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

static PSController psCtlrArray[NumOCM] = {
	/* id, setpoint, feedback, channel, inCorrection, crateId, VmeModule*   */
	/* first, the horizontal controllers (OCH14xx-xx): */
	{"OCH1401-01",0,0,9/*channel*/,1,0,0},
	{"OCH1401-02",0,0,12/*channel*/,1,0,0},
	{"OCH1402-01",0,0,13/*channel*/,1,0,0},
	{"OCH1402-02",0,0,14/*channel*/,1,0,0},
	{"OCH1403-01",0,0,1/*channel*/,1,1,0},
	{"OCH1403-02",0,0,2/*channel*/,1,1,0},
	{"OCH1404-01",0,0,3/*channel*/,1,1,0},
	{"OCH1404-02",0,0,4/*channel*/,1,1,0},
	{"OCH1405-01",0,0,5/*channel*/,1,1,0},
	{"OCH1405-02",0,0,6/*channel*/,1,1,0},
	{"OCH1406-01",0,0,7/*channel*/,1,2,0},
	{"OCH1406-02",0,0,8/*channel*/,1,2,0},
	{"OCH1407-01",0,0,9/*channel*/,1,2,0},
	{"OCH1407-02",0,0,12/*channel*/,1,2,0},
	{"OCH1408-01",0,0,13/*channel*/,1,2,0},
	{"OCH1408-02",0,0,14/*channel*/,1,2,0},
	{"OCH1409-01",0,0,7/*channel*/,1,3,0},
	{"OCH1409-02",0,0,8/*channel*/,1,3,0},
	{"OCH1410-01",0,0,9/*channel*/,1,3,0},
	{"OCH1410-02",0,0,12/*channel*/,1,3,0},
	{"OCH1411-01",0,0,13/*channel*/,1,3,0},
	{"OCH1411-02",0,0,14/*channel*/,1,3,0},
	{"OCH1412-01",0,0,7/*channel*/,1,0,0},
	{"OCH1412-02",0,0,8/*channel*/,1,0,0},

	/* next, the vertical correctors (OCVxx-xx): */
	{"OCV1401-01",0,0,3/*channel*/,1,0,0},
	{"OCV1401-02",0,0,4/*channel*/,1,0,0},
	{"OCV1402-01",0,0,5/*channel*/,1,0,0},
	{"OCV1402-02",0,0,6/*channel*/,1,0,0},
	{"OCV1403-01",0,0,1/*channel*/,1,1,0},
	{"OCV1403-02",0,0,2/*channel*/,1,1,0},
	{"OCV1404-01",0,0,3/*channel*/,1,1,0},
	{"OCV1404-02",0,0,4/*channel*/,1,1,0},
	{"OCV1405-01",0,0,5/*channel*/,1,1,0},
	{"OCV1405-02",0,0,6/*channel*/,1,1,0},
	{"OCV1406-01",0,0,1/*channel*/,1,2,0},
	{"OCV1406-02",0,0,2/*channel*/,1,2,0},
	{"OCV1407-01",0,0,3/*channel*/,1,2,0},
	{"OCV1407-02",0,0,4/*channel*/,1,2,0},
	{"OCV1408-01",0,0,5/*channel*/,1,2,0},
	{"OCV1408-02",0,0,6/*channel*/,1,2,0},
	{"OCV1409-01",0,0,1/*channel*/,1,3,0},
	{"OCV1409-02",0,0,2/*channel*/,1,3,0},
	{"OCV1410-01",0,0,3/*channel*/,1,3,0},
	{"OCV1410-02",0,0,4/*channel*/,1,3,0},
	{"OCV1411-01",0,0,5/*channel*/,1,3,0},
	{"OCV1411-02",0,0,6/*channel*/,1,3,0},
	{"OCV1412-01",0,0,1/*channel*/,1,0,0},
	{"OCV1412-02",0,0,2/*channel*/,1,0,0}
};

#endif /* OCMDEFS_H_ */
