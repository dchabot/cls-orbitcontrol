/*
 * OcmController.h
 *
 *  Created on: Jan 7, 2009
 *      Author: chabotd
 */

#ifndef OCMCONTROLLER_H_
#define OCMCONTROLLER_H_

#include <stdint.h>
#include <Ocm.h>
#include <Bpm.h>


class OcmController {
public:
	OcmController(){}
	virtual ~OcmController(){}
	virtual void setOcmSetpoint(Ocm* ch, int32_t val)=0;
	virtual Ocm* registerOcm(const string& str,uint32_t crateId,uint32_t vmeAddr,uint8_t ch)=0;
	virtual void unregisterOcm(Ocm* ch)=0;
	virtual Ocm* getOcmById(const string& id)=0;
	virtual void showAllOcms()=0;
	virtual void setVerticalResponseMatrix(double v[NumVOcm*NumBpm])=0;
	virtual void setHorizontalResponseMatrix(double h[NumHOcm*NumBpm])=0;
	virtual void setDispersionVector(double d[NumBpm])=0;
	virtual void setMaxHorizontalStep(int32_t step)=0;
	virtual int32_t getMaxHorizontalStep()const=0;
	virtual void setMaxVerticalStep(int32_t step)=0;
	virtual int32_t getMaxVerticalStep()const=0;
	virtual void setMaxHorizontalFraction(double f)=0;
	virtual double getMaxHorizontalFraction()const=0;
	virtual void setMaxVerticalFraction(double f)=0;
	virtual double getMaxVerticalFraction()const=0;
};

#endif /* OCMCONTROLLER_H_ */
