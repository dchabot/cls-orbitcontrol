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

class OcmController {
public:
	OcmController(){}
	virtual ~OcmController(){}
	virtual void setOcmSetpoint(Ocm* ch, int32_t val)=0;
	virtual Ocm* registerOcm(const string& str,uint32_t crateId,uint32_t vmeAddr,uint8_t ch)=0;
	virtual void unregisterOcm(Ocm* ch)=0;
	virtual Ocm* getOcmById(const string& id)=0;
	virtual void showAllOcms()=0;
	virtual void setVerticalResponseMatrix(double v[NumOcm*NumOcm])=0;
	virtual void setHorizontalResponseMatrix(double h[NumOcm*NumOcm])=0;
	virtual void setDispersionVector(double d[NumOcm])=0;

	void setMaxHorizontalStep(int32_t step);
	int32_t getMaxHorizontalStep() const;
	void setMaxVerticalStep(int32_t step);
	int32_t getMaxVerticalStep() const ;
	void setMaxHorizontalFraction(double f);
	double getMaxHorizontalFraction() const;
	void setMaxVerticalFraction(double f);
	double getMaxVerticalFraction() const;

protected:
	int32_t maxHStep;
	int32_t maxVStep;
	int32_t maxHFrac;
	int32_t maxVFrac;
	double hmat[NumOcm][NumOcm];
	double vmat[NumOcm][NumOcm];
	double dmat[NumOcm];
};

inline int32_t OcmController::getMaxHorizontalStep() const { return maxHStep; }
inline int32_t OcmController::getMaxVerticalStep() const { return maxVStep; }
inline double OcmController::getMaxHorizontalFraction() const { return maxHFrac; }
inline double OcmController::getMaxVerticalFraction() const { return maxVFrac; }

inline void OcmController::setMaxHorizontalStep(int32_t step) { maxHStep = step; }
inline void OcmController::setMaxVerticalStep(int32_t step) { maxVStep = step; }
inline void OcmController::setMaxHorizontalFraction(double f) { maxHFrac = f; }
inline void OcmController::setMaxVerticalFraction(double f) { maxVFrac = f; }

#endif /* OCMCONTROLLER_H_ */
