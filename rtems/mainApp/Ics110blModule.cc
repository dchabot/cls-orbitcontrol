/*
 * Ics110blModule.cc
 *
 *  Created on: Dec 8, 2008
 *      Author: djc
 */

#include <OrbitControlException.h>
#include <Ics110blModule.h>
#include <rtems.h>
#include <sis1100_api.h>
#include <stdlib.h> //for abs(),etc
#include <string>
#include <syslog.h>

using std::string;

//FIXME -- remove dependence on OrbitControlException. Use std::runtime_error instead.

Ics110blModule::Ics110blModule(VmeCrate* c, uint32_t vmeAddr) :
	//constructor-initializer list
	VmeModule(c,vmeAddr)
{
	type = "ICS-110BL";
	initialized = acquiring = false;
	irqLevel = ICS110B_DEFAULT_IRQ_LEVEL;
	irqVector = ICS110B_DEFAULT_IRQ_VECTOR;
	trueFrameRate = 0;
	channelsPerFrame = ICS110B_DEFAULT_CHANNELS_PER_FRAME;
	overSamplingRate = ICS110B_X128;
}

Ics110blModule::~Ics110blModule() {
	if(isAcquiring()) {
		stopAcquisition();
		disableInterrupt();
		resetFifo();
	}
}

/** Assumes Ics110blModule::channelsPerFrame has already been set */
void Ics110blModule::initialize(double requestedFrameRate,
								int clockingSelect,
								int acquireSelect) {
	double  calib_smpl_rate = 10.0; /* kHz */
	int oversampleRate;

	if(requestedFrameRate < crossoverFrameRate){
		oversampleRate = ICS110B_X128;
	} else {
		oversampleRate = ICS110B_X64;
	}

	/* Board Reset CLS-110B */
	reset();

	/* Calibrating the ADC and setting the sampling rate */
	calibrateAdc(oversampleRate, &calib_smpl_rate);

	/* Configuring the onboard filtering*/
	setHPF(oversampleRate, ICS110B_DISABLED);

	/* Set CLS-110B as Sampling Master */
	setMaster(ICS110B_ENABLED);

	/* Select internal or external triggering */
	setAcquisitionTriggerSource(acquireSelect);

	/* Select path and packing for output */
	setOutputMode(ICS110B_VME_UNPACKED);

	/* add ability to select internal or external clocking source */
	if(clockingSelect == INTERNAL_CLOCK) {
		setClockSource(ICS110B_INTERNAL);
	} else {
		setClockSource(ICS110B_EXTERNAL);
	}

	/* Setting the amount of oversampling that will be done */
	setOversamplingRate(oversampleRate);

	/* Program number of channels to be used */
	setChannelsPerFrame(channelsPerFrame);

	setFrameRate(requestedFrameRate,oversampleRate);

	initialized = true;
}

void Ics110blModule::programFoxRate(uint32_t foxWord) {
	uint32_t ctrlWord = (0x38001e05| (foxWord & 0x80000000));

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, ctrlWord); 	/* First control word write out */
#else
	writeA24D32(ICS110B_CLOCK_OFFSET,ctrlWord);
#endif
	Ics110blModule::sleep(0.170);

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, foxWord); 	/* Writting out the word */
#else
	writeA24D32(ICS110B_CLOCK_OFFSET,foxWord);
#endif
	Ics110blModule::sleep(0.170);

	ctrlWord = (0x38001e04 | (foxWord & 0x80000000));
#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, ctrlWord);	/* Second control word write out */
#else
	writeA24D32(ICS110B_CLOCK_OFFSET,ctrlWord);
#endif
	Ics110blModule::sleep(0.170);

	ctrlWord = (0x38001e00| (foxWord & 0x80000000));
#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, ctrlWord);	/* Third control word write out */
#else
	writeA24D32(ICS110B_CLOCK_OFFSET,ctrlWord);
#endif
	Ics110blModule::sleep(0.170);
}

void Ics110blModule::programAdcSpi(ICS110B_ADC_SPI *arg) {
	int                     regAddr, j;
	uint16_t               ctrlWord;
	ICS110B_ADC_SPI         *spiStruct;

	spiStruct = arg;
	ctrlWord = 0x8000; /* select device(s) */

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_write(cardFD, cardBase + ICS110B_ADC_SPI_OFFSET, ctrlWord);
#else
	writeA24D16(ICS110B_ADC_SPI_OFFSET,ctrlWord);
#endif
	Ics110blModule::sleep(0.5);

	ctrlWord = (spiStruct->startAddr) | 0x8000; /* set chip sel bit */
#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD, cardBase + ICS110B_ADC_SPI_OFFSET, ctrlWord);
#else
	writeA24D16(ICS110B_ADC_SPI_OFFSET,ctrlWord);
#endif
	Ics110blModule::sleep(0.5);

	regAddr = spiStruct->startAddr;
	for(j = 0; j < spiStruct->nBytes; j++) {
		if(regAddr == 0x6) {
			ctrlWord = 0x8000; /* don't clobber chip SPI address */
		}
		else {
			ctrlWord = (spiStruct->regData[j] | 0x8000);
		}
#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
		status |= vme_A24D16_write(cardFD, cardBase + ICS110B_ADC_SPI_OFFSET, ctrlWord);
#else
		writeA24D16(ICS110B_ADC_SPI_OFFSET,ctrlWord);
#endif
		Ics110blModule::sleep(0.5);
		regAddr++;
	}

	ctrlWord = 0x0000; /* set chip sel bit low; ie de-select */
#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD, cardBase + ICS110B_ADC_SPI_OFFSET, ctrlWord);
#else
	writeA24D16(ICS110B_ADC_SPI_OFFSET,ctrlWord);
#endif
	Ics110blModule::sleep(0.034);
}

void Ics110blModule::calibrateAdc(int overSamplingRate, double *pSampleRate) {
	double          slowRate;
	ICS110B_ADC_SPI spiStruct;
	slowRate = 10.00;

	setFrameRate(slowRate,overSamplingRate);

	spiStruct.nBytes = 2;
	spiStruct.startAddr = 0x01;
	spiStruct.regData[0] = (char)0xC0;

	if(overSamplingRate == ICS110B_X64) {
		spiStruct.regData[1] = (char)0x44;
	} else {
		spiStruct.regData[1] = (char)0xC4 ;
	}

	/* set cal bit in mode register, automatically cleared */
	programAdcSpi(&spiStruct);
	Ics110blModule::sleep(0.333);

	spiStruct.nBytes = 1;
	spiStruct.startAddr = 0x01;
	spiStruct.regData[0] = (char)0x0;

	programAdcSpi(&spiStruct);

	setFrameRate(*pSampleRate, overSamplingRate);
}

/** will set Ics110blModule::trueFrameRate !! */
void Ics110blModule::setFrameRate(double requestRate, uint8_t overSamplingRate)
{
	int     bitx, oneCount, bitCount;
	long    foxWord, tempWord, i;
	double  request;

	if(overSamplingRate == ICS110B_X64) {
		request = (requestRate) * (double)0.256;
	} else {
		request = (requestRate) * (double)0.512;
	}

	//Ics110blModule::trueFrameRate is modified in calcFoxWord
	calcFoxWord(&request,&tempWord);

	foxWord = 0x0;
	bitCount = 0x0;
	oneCount = 0x0;
	for(i = 0; i < 22; i++) {
		bitx = tempWord &0x01L;
		foxWord |= (bitx << bitCount);
		if(bitx == 1) {
			oneCount++;
		} else {
			oneCount = 0;
		}

		if(oneCount == 3) { /* must bit-stuff a zero after ANY sequence of three ones */
			oneCount = 0;
			bitCount++;
		}
		bitCount++;
		tempWord >>= 1;
	}

	foxWord |= ((bitCount - 22) << 27);

	if(overSamplingRate == ICS110B_X64) {
		trueFrameRate /= (double).256;
	} else {
		trueFrameRate /= (double).512;
	}

	programFoxRate((uint32_t)foxWord);

}

void Ics110blModule::resetDemodulator() {
	uint32_t temp = 	(3<<23) /* This sets channel number to 4th pair, i.e. we are using signal /CS_0708 */
			| 0x40000000;   /* This routes serial clock to daughter card rather than to other destinations*/

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, temp);
#else
	writeA24D32(ICS110B_CLOCK_OFFSET,temp);
#endif
	Ics110blModule::sleep(0.175);
}

void Ics110blModule::calcFoxWord(double *reqRate, long  *progWord) {
	short   m, i, p, pp ,q, qp, ri, rm, rp, rq, notdone;
	double  fvco, fout, err;
	static double freqRange [3] = { 50.0,  80.0,  150.0};
	static short index[2] = { 0, 8};        /* ICD2053B legal index values */
	short  tabLen = 2;      /* Length of freqRange table less one */
	double  fRef = 14.31818;        /* Reference crystal frequency (MHz) */

	ri = 4;
	rm = 4;
	rp = 4;
	rq = 4;

	m = 0;
	fvco = *reqRate;

	while ((fvco < freqRange[0]) && (m < 8)) {
		fvco = fvco * 2;
		m++;
	}

	if (m == 8) {
		throw OrbitControlException("ICS110BL: Frame rate too low!!");
	} else {
		if (fvco > freqRange[tabLen]) {
			throw OrbitControlException("ICS110BL: Frame rate too high!!");
		}
	}

	err = 10.0;        /* Starting value for error (fvco-fout) */
	notdone = TRUE;
	while (notdone == TRUE) {
		i = 0;

		while ((freqRange[i+1] < fvco) && (i < (tabLen - 1)))  {
			i++;
		}

		for (pp = 4; pp < 131; pp++) {
			qp = (int) (fRef * 2.0 * (double)pp / fvco + (double)0.5);
			fout =(fRef * 2.0 * (double)pp / (double)qp);
			if ((abs(fvco - fout) < err) && (qp >= 3) && (qp < 129)) {
				err = abs(fvco - fout);
				rm = m;
				ri = i;
				rp = pp;
				rq = qp;
			}
		}

		if (((fvco * 2.0) > freqRange[tabLen]) || (m == 7)) {
			notdone = FALSE;
		} else {
			fvco *= 2.0;
			m++;
		}
	}

	trueFrameRate = ((fRef * 2.0 * (double)rp) / ((double)rq * (double)(1 << rm)));
	p = rp - 3;
	q = rq - 2;
	*progWord = ((p & 0x7f) << 15) | ((rm & 0x7) << 11) | ((q & 0x7f) << 4) | (index[ri] & 0xf);

}

void Ics110blModule::setMaster(int master) {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif

	if (master) {
		data |= ICS110B_MASTER;
	} else {
		data &=	~ICS110B_MASTER;
	}

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif

}

void Ics110blModule::setAcquisitionTriggerSource(int src) {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif

	if(src == ICS110B_INTERNAL) {
		data &= ~ICS110B_ACQUIRE_SOURCE;
	} else {
		data |= ICS110B_ACQUIRE_SOURCE;
	}

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif

}

void Ics110blModule::setClockSource(int src) {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif

	if(src == ICS110B_INTERNAL) {
		data &= ~ICS110B_CLOCK_SEL;
	} else {
		data |= ICS110B_CLOCK_SEL;
	}

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif

}

void Ics110blModule::setOutputMode(uint16_t mode) {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif

	mode &= 0x7;
	mode <<= 3;

	data &= 0xFFC7;
	data |= mode;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif

}

void Ics110blModule::setClockSelect(int clk) {
	uint16_t data = 0;


#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif

	if(clk == ICS110B_INTERNAL) {
		data &= ~ICS110B_CLOCK_SEL;
	} else {
		data |= ICS110B_CLOCK_SEL;
	}

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif

}

void Ics110blModule::setOversamplingRate(uint8_t rate) {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif

	if(rate == ICS110B_X64) {
		data &= ~ICS110B_OVERSAMPLING_RATIO;
	} else {
		data |= ICS110B_OVERSAMPLING_RATIO;
	}

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif

}
/** This sets Ics110blModule::channelsPerFrame */
void Ics110blModule::setChannelsPerFrame(uint8_t ch) {
	uint16_t data = (uint16_t)ch;

	if((data%2 != 0) || (data > 32) || (data < 2)) {
		string err("2 <= numChannels <= 32 and divisible by 2\n");
		syslog(LOG_INFO, "%s",err.c_str());
		throw OrbitControlException(err.c_str());
	}
	data -= 1;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_write(cardFD,cardBase + ICS110B_NUM_CHANS_OFFSET,data);
#else
	writeA24D16(ICS110B_NUM_CHANS_OFFSET,data);
#endif

	channelsPerFrame = ch;
}

void Ics110blModule::setHPF(int overSamplingRate, bool isEnabled) {
	ICS110B_ADC_SPI spiStruct;

	spiStruct.nBytes = 1;
	spiStruct.startAddr = 0x02;

	switch(overSamplingRate) {
		case ICS110B_X64: {
			if(isEnabled==ICS110B_ENABLED) { spiStruct.regData[0] = (char)0x04; }
			else { spiStruct.regData[0] = (char)0x0C; }
			break;
		}
		case ICS110B_X128: {
			if(isEnabled==ICS110B_ENABLED) { spiStruct.regData[0] = (char)0x84; }
			else { spiStruct.regData[0] = (char)0x8C; }
			break;
		}
		default: {
			syslog(LOG_INFO,"Ics110blModule::enableHPF: invalid oversampling option=%i\n",overSamplingRate);
			throw OrbitControlException("Ics110blModule::enableHPF: invalid overSamplingRate");
		}
	} /* end switch() */

	programAdcSpi(&spiStruct);

}

int Ics110blModule::readFifo(uint32_t *buffer,
								uint32_t wordsRequested,
								uint32_t *wordsRead) const {
	int cardFD;
	uint32_t cardBase;

	cardBase = vmeBaseAddr;
	cardFD = crate->getFd();

	return vme_A24BLT32FIFO_read(cardFD,cardBase+ICS110B_FIFO_OFFSET,buffer,wordsRequested,wordsRead);

}

/*
	NOTE: this will place the board in a "default" state:
	i.e. non-acquiring, empty FIFO, etc...
*/
void Ics110blModule::reset() {
	uint16_t data = ICS110B_BOARD_RESET;
	/* don't perform the usual read-modify-write operations here
	 * 'cause we don't care what the current settings are:
	 * 		THIS IS A COMPLETELY DESTRUCTIVE RESET !!!
	 *
	 * 		mmmmm... destructive....
	 */

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_write(cardFD,cardBase + ICS110B_STAT_RESET_OFFSET,data);
#else
	writeA24D16(ICS110B_STAT_RESET_OFFSET,data);
#endif
}

void Ics110blModule::resetFifo() {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_STAT_RESET_OFFSET,&data);
#else
	data = readA24D16(ICS110B_STAT_RESET_OFFSET);
#endif

	data |= ICS110B_FIFO_RESET;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_STAT_RESET_OFFSET,data);
#else
	writeA24D16(ICS110B_STAT_RESET_OFFSET,data);
#endif

}

void Ics110blModule::setIrqVector(uint8_t vector) {
#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_write(cardFD,cardBase + ICS110B_IVECT_OFFSET,(uint16_t)vector);
#else
	writeA24D16(ICS110B_IVECT_OFFSET,(uint16_t)vector);
#endif
	irqVector = vector;
}

uint8_t Ics110blModule::getIrqVector() const {
	uint16_t vector;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_IVECT_OFFSET,&vector);
#else
	vector = readA24D16(ICS110B_IVECT_OFFSET);
#endif

	return (uint8_t)vector;
}

void Ics110blModule::enableInterrupt() {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif
	data |= ICS110B_VME_INT_ENABLE;
#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif
}

void Ics110blModule::disableInterrupt() {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif
	data &= ~ICS110B_VME_INT_ENABLE;
#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif
}

void Ics110blModule::startAcquisition() {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif

	data |= 1;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif
	acquiring = true;
}

void Ics110blModule::stopAcquisition() {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readA24D16(ICS110B_CTRL_OFFSET);
#endif

	data &= ~1;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeA24D16(ICS110B_CTRL_OFFSET,data);
#endif
	acquiring = false;
}

uint16_t Ics110blModule::getStatus() {
	uint16_t data;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD,cardBase + ICS110B_STAT_RESET_OFFSET,&data);
#else
	data = readA24D16(ICS110B_STAT_RESET_OFFSET);
#endif

	return data &= 0x00CF;
}

bool Ics110blModule::isFifoEmpty() {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD, cardBase + ICS110B_STAT_RESET_OFFSET, &data);
#else
	data = readA24D16(ICS110B_STAT_RESET_OFFSET);
#endif

	return ((data&ICS110B_FIFO_EMPTY)==ICS110B_FIFO_EMPTY);
}

bool Ics110blModule::isFifoFull() {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD, cardBase + ICS110B_STAT_RESET_OFFSET, &data);
#else
	data = readA24D16(ICS110B_STAT_RESET_OFFSET);
#endif

	return ((data&ICS110B_FIFO_FULL)==ICS110B_FIFO_FULL);
}

bool Ics110blModule::isFifoHalfFull() {
	uint16_t data = 0;

#ifndef ICS110BLMODULE_USE_SINGLE_CYCLE_VME_ACCESS
	uint32_t cardBase = vmeBaseAddr;
	int cardFD = crate->getFd();
	int status = vme_A24D16_read(cardFD, cardBase + ICS110B_STAT_RESET_OFFSET, &data);
#else
	data = readA24D16(ICS110B_STAT_RESET_OFFSET);
#endif

	return ((data&ICS110B_FIFO_HALF_FULL)==ICS110B_FIFO_HALF_FULL);
}

void Ics110blModule::sleep(double secs) {
	rtems_interval ticks_per_second;
	/* This needed to be defined, since some of the operations for this board need to be time sequenced for the hardware to work correctly */
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &ticks_per_second);
	rtems_task_wake_after((int)(secs*ticks_per_second));
}
