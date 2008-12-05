#include <stdlib.h> /* for abs() */
#include <syslog.h>

#include "ics110bl.h"
#include <sis1100_api.h>

/* NOTE: all methods here return 0 on success and non-zero on error */

/*	---------------------------------------------------------------------------
	This function is used for setting all of the settings on a ICS110Bl ADC. It
	will also configure the various onboard electronics and reset the FIFOs
	leaving the board ready for operation. All that is needed after this point
	to run the board is the setting of the start aquisition bit.
*/
int InitICS110BL(VmeModule *module, double reqSampleRate, double *trueSampleRate, int clockingSelect, int AcquireSelect, int numChannels) {
	const double CrossoverFrequency = 54.0; /* kHz. Where oversampling rate is cut in half. */
	double  calib_smpl_rate = 10.0; /* kHz */
	int oversampleRate;
	int rc;

	if(reqSampleRate < CrossoverFrequency){
		oversampleRate = ICS110B_X128;
	} else {
		oversampleRate = ICS110B_X64;
	}

    /* Board Reset CLS-110B */
    if((rc=ICS110BDeviceReset(module)) != OK) {
        syslog(LOG_INFO, "\nUnable to reset CLS-110B board: rc=%#x\n",rc);
        return ERROR;
    }

	/* Calibrating the ADC and setting the sampling rate */
    if((rc=ICS110BCalibrateAdc(module, oversampleRate, &calib_smpl_rate)) != OK) {
		syslog(LOG_INFO, "\nUnable to calibrate CLS-110B board: rc=%#x\n",rc);
		return ERROR;
    }

	/* Configuring the onboard filtering*/
    if((rc=ICS110BFiltersSetADC(module,oversampleRate, ICS110B_DISABLED)) != OK) {
		syslog(LOG_INFO, "\nICS110BFiltersSetADC: unable to set on CLS-110B board: rc=%#x\n",rc);
		return ERROR;
    }

    /* Set CLS-110B as Sampling Master */
    if((rc=ICS110BMasterSet(module, ICS110B_ENABLED )) != OK) {
		syslog(LOG_INFO, "\nUnable to set CLS-110B as Master: rc=%#x\n",rc);
		return ERROR;
    }

    /* Select internal or external triggering */
    if((rc=ICS110BAcquireSourceSet(module, AcquireSelect)) != OK) {
		syslog(LOG_INFO, "\nUnable to set CLS-110B Acquire Source: rc=%#x\n",rc);
		return ERROR;
    }

    /* Select path and packing for output */
    if((rc=ICS110BOutputModeSet(module, ICS110B_VME_UNPACKED)) != OK) {
		syslog(LOG_INFO, "\nUnable to set CLS-110B Output Mode: rc=%#x\n",rc);
		return ERROR;
    }

    /* add ability to select internal or external clocking source */
    if(clockingSelect == INTERNAL_CLOCK) {
		if((rc=ICS110BClockSelectSet(module, ICS110B_INTERNAL)) != OK) {
	    	syslog(LOG_INFO, "\nUnable to set CLS-110B Clock Source: rc=%#x\n",rc);
	    	return ERROR;
		}
    } else {
		if((rc=ICS110BClockSelectSet(module, ICS110B_EXTERNAL)) != OK) {
			syslog(LOG_INFO, "\nUnable to set CLS-110B Clock Source\n");
			return ERROR;
		}
    }

	/* Setting the amount of oversampling that will be done */
    if((rc=ICS110BOversamplingSet(module, oversampleRate)) != OK) {
		syslog(LOG_INFO, "\nUnable to set CLS-110B %dX Oversampling Mode: rc=%#x\n",oversampleRate,rc);
		return ERROR;
    }

    /* Program number of channels to be used */
    if((rc=ICS110BNoChannelsSet(module, numChannels)) != OK) {
		syslog(LOG_INFO, "\nUnable to set number of channels on CLS-110B board: rc=%#x\n",rc);
		return ERROR;
    }

    if((rc=ICS110BSampleRateSet(module, reqSampleRate, trueSampleRate, oversampleRate)) != OK) {
		syslog(LOG_INFO, "\nUnable to set sample rate on CLS-110B board: rc=%#x\n",rc);
		return(ERROR);
	}

    return OK;
}


/*
    ===========================================================================
    Code
    ===========================================================================
*/

/*
	int programFoxRate(int board_map_index, int foxWordPtr)
	---------------------------------------------------------------------------
	This function is used to directly write a word to the Fox F6053 on the ADC.
	It takes in the the pointer to the base address structure of the board and
	a pointer to the word you wish to write out. It is assumed that the caller
	has verified the validity of the word before calling this function. The
	function currently catuches the return of the writes, but does not error
	check them. This needs to be added in. In addition, I am not sure what each
	of the control words does explicitly other then togheter they set the board
	up for writing the fox word and then return the board to a normal mode of
	operation. This should be detailed what each write does.

	Algorithm
	---------------------------------------------------------------------------
	Get adddressing informaiton for the board
	Pull in the pointer in the stack of the function incase we get context switched
	Set up the control word based off of the word to write out
	Write out the control word
	Delay letting the board react to the write (The manual explicitly states this step)
	Write out the word as desired to the board
	Delay letting the board react to the write (The manual explicitly states this step)
	Set up the control word based off of the word to write out
	Write out the control word
	Delay letting the board react to the write (The manual explicitly states this step)
	Set up the control word based off of the word to write out
	Write out the control word
	Delay letting the board react to the write (The manual explicitly states this step)
*/
int programFoxRate(VmeModule *module, int foxWordPtr) {
	u_int32_t       ctrlWord;
	u_int32_t       foxWord;
	u_int32_t cardBase;
	int cardFD;
	int status = 0;

	cardBase = module->vmeBaseAddr;
	cardFD = module->crate->fd;

	foxWord = *(u_int32_t *)foxWordPtr;

	ctrlWord = (0x38001e05| (foxWord & 0x80000000));
#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, ctrlWord); 	/* First control word write out */
#else
	writeD32(module,ICS110B_CLOCK_OFFSET,ctrlWord);
#endif
	ICS110BTaskDelay(0.170);

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, foxWord); 	/* Writting out the word */
#else
	writeD32(module,ICS110B_CLOCK_OFFSET,foxWord);
#endif
	ICS110BTaskDelay(0.170);

	ctrlWord = (0x38001e04 | (foxWord & 0x80000000));
#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, ctrlWord);	/* Second control word write out */
#else
	writeD32(module,ICS110B_CLOCK_OFFSET,ctrlWord);
#endif
	ICS110BTaskDelay(0.170);

	ctrlWord = (0x38001e00| (foxWord & 0x80000000));
#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, ctrlWord);	/* Third control word write out */
#else
	writeD32(module,ICS110B_CLOCK_OFFSET,ctrlWord);
#endif
	ICS110BTaskDelay(0.170);

	return status;
}

/*
	int ICS110BProgramAdcSpi(int board_map_index, int arg)
	---------------------------------------------------------------------------
	This function is used to write out a word to directly set up the ADC
	convertors The logic for this function has been pulled directly from the
	VxWorks driver. The only major change has been to conver the memory map uses
	to calls to the SIS1100 api. For further information on this function see
	the VxWorks manual.
*/
int ICS110BProgramAdcSpi(VmeModule *module, int arg) {
	int                     regAddr, j;
	uint16_t               ctrlWord;
	ICS110B_ADC_SPI         *spiStruct;
	int cardBase;
	int cardFD;
	int status = 0;

	cardBase = module->vmeBaseAddr;
	cardFD = module->crate->fd;

	spiStruct = (ICS110B_ADC_SPI *)arg;

	ctrlWord = 0x8000; /* select device(s) */
#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_write(cardFD, cardBase + ICS110B_ADC_SPI_OFFSET, ctrlWord);
#else
	writeD16(module,ICS110B_ADC_SPI_OFFSET,ctrlWord);
#endif
	ICS110BTaskDelay(0.5);

	ctrlWord = (spiStruct->startAddr) | 0x8000; /* set chip sel bit */
#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD, cardBase + ICS110B_ADC_SPI_OFFSET, ctrlWord);
#else
	writeD16(module,ICS110B_ADC_SPI_OFFSET,ctrlWord);
#endif
	ICS110BTaskDelay(0.5);

	regAddr = spiStruct->startAddr;
	for(j = 0; j < spiStruct->nBytes; j++) {
		if(regAddr == 0x6) {
	    	ctrlWord = 0x8000; /* don't clobber chip SPI address */
		}
		else {
			ctrlWord = (spiStruct->regData[j] | 0x8000);
		}
#ifndef USE_MACRO_VME_ACCESSORS
		status |= vme_A24D16_write(cardFD, cardBase + ICS110B_ADC_SPI_OFFSET, ctrlWord);
#else
		writeD16(module,ICS110B_ADC_SPI_OFFSET,ctrlWord);
#endif
		ICS110BTaskDelay(0.5);
		regAddr++;
    }

	ctrlWord = 0x0000; /* set chip sel bit low; ie de-select */
#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD, cardBase + ICS110B_ADC_SPI_OFFSET, ctrlWord);
#else
	writeD16(module,ICS110B_ADC_SPI_OFFSET,ctrlWord);
#endif
	ICS110BTaskDelay(0.034);
	return status;
}

/*
	int ICS110BCalibrateAdc(VmeModule *module, int sampleRatio, double *pSampleRate)
	---------------------------------------------------------------------------
	This function allows one to configure the ADC. It takes in a  point to the
	base address of the board you wish to configure, the sampleRatio you want
	to use and pointer to the sampling rate you wish to run the board at. It
	returns either OK or ERROR and the actualy sampling rate the board is set
	to in pSampleRate. The variance between the desired and actual rate is due
	to the internal set up of the ADC, which only allows certian values to be
	specified. For more information on this read the operating manual for the
	board. The bulk of the logic for this function has been pulled from the
	VxWorks driver and modified for accessing the SIS1100.
*/
int ICS110BCalibrateAdc(VmeModule *module, int sampleRatio, double *pSampleRate) {
	double          slowRate;
	ICS110B_ADC_SPI spiStruct;
	slowRate = 10.00;

	if(ICS110BSampleRateSet(module, slowRate, &slowRate, sampleRatio) != OK) {
		return ERROR;
	}

	spiStruct.nBytes = 2;
	spiStruct.startAddr = 0x01;
	spiStruct.regData[0] = (char)0xC0;

	if(sampleRatio == ICS110B_X64) {
		spiStruct.regData[1] = (char)0x44;
	} else {
		spiStruct.regData[1] = (char)0xC4 ;
	}

	/* set cal bit in mode register, automatically cleared */
	if (ICS110BProgramAdcSpi(module, (int) &spiStruct) != OK) {
		return ERROR;
	}
	ICS110BTaskDelay(0.333);

	spiStruct.nBytes = 1;
	spiStruct.startAddr = 0x01;
	spiStruct.regData[0] = (char)0x0;

	if (ICS110BProgramAdcSpi(module, (int) &spiStruct) != OK) {
		return ERROR;
	}

	if(ICS110BSampleRateSet(module, *pSampleRate, &slowRate, sampleRatio) != OK) {
		return ERROR;
	}

	return OK;
}

/*
	int ICS110BSampleRateSet(VmeModule *module, double reqRate, double *actRate, int sampleRatio)
	---------------------------------------------------------------------------
	This function is a high level function used to set the sampling rate. It
	takes in a pointer to the board base address that will be manipulated. The
	requested sampling a rate, a pointer for  avariable to return what the board
	was actually set to and the desired sample ratio. It will then do the calls
	that are need for calculating the Fox word and then send out that word to
	the ADC. This function is called as apart of the ADC calibrate process.

	Algorithm
	---------------------------------------------------------------------------
	Calculate the true rate to be used by the board from the requested sampling rate and the sample ratio.
	Calculate the correct word to send to the Fox chip
	Verfiy and shape the Fox word
	Calculate what the actual sampling rate will be set
	Write out the fox word to the ADC

*/
int ICS110BSampleRateSet(VmeModule *module, double reqRate, double *actRate, int sampleRatio) {
	int     bitx, oneCount, bitCount;
	long    foxWord, tempWord, i;
	double  request, actual;

	if(sampleRatio == ICS110B_X64) {
		request = (reqRate) * (double)0.256;
	} else {
		request = (reqRate) * (double)0.512;
	}

	if(ICS110BCalcFoxWord(&request, &actual, &tempWord) != OK) {
		return ERROR;
	}

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

	if(sampleRatio == ICS110B_X64) {
		*actRate = actual / (double).256;
	} else {
		*actRate = actual / (double).512;
	}

	if (programFoxRate(module, (int)&foxWord) != OK) {
		return ERROR;
	}
	return OK;
}

/*
	int ICS110BDemodReset(VmeModule *module)
	---------------------------------------------------------------------------
	This function is used to reset the Demod counter on the ADC board. The logic
	that appears in the VxWorks driver is much simpler then what happens in this
	function, so it must be based off of the Linux driver. Consult the Linux
	driver or Russ to see why the specifc bits are set or why there is a short
	task delay
*/
int ICS110BDemodReset(VmeModule *module) {
	uint32_t temp = 0;
	int cardBase;
	int cardFD;
	int status = 0;

	cardBase = module->vmeBaseAddr;
	cardFD = module->crate->fd;


	temp = 	(3<<23) /* This sets channel number to 4th pair, i.e. we are using signal /CS_0708 */
			| 0x40000000;   /* This routes serial clock to daughter card rather than to other destinations*/

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D32_write(cardFD, cardBase + ICS110B_CLOCK_OFFSET, temp);
#else
	writeD32(module,ICS110B_CLOCK_OFFSET,temp);
#endif
	ICS110BTaskDelay(0.175);

	return status;
}

/*
	int ICS110BCalcFoxWord(double *reqFreq, double *actFreq, long *progWord)
	---------------------------------------------------------------------------
	This routine is used to calculate a programming word for a Fox F6053, but
	NOT including the zero stuffing after 3 consecutive ones. It takes in as
	parameters the desired frequency (MHz), the returned programming word ,and
	the actual frequency generated by progWord.
	*** This code was pulled from the Linux Driver/VxWorks Driver. For a
	discussion on how the word is actually calculated consult the manual ***
*/
int ICS110BCalcFoxWord(double *reqFreq, double *actFreq, long *progWord) {
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
	fvco = *reqFreq;

	while ((fvco < freqRange[0]) && (m < 8)) {
		fvco = fvco * 2;
		m++;
	}

	if (m == 8) {
		return (ICS110B_FREQ_TOO_LOW);
    } else {
		if (fvco > freqRange[tabLen]) {
			return(ICS110B_FREQ_TOO_HIGH);
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

	*actFreq = ((fRef * 2.0 * (double)rp) / ((double)rq * (double)(1 << rm)));
	p = rp - 3;
	q = rq - 2;
	*progWord = ((p & 0x7f) << 15) | ((rm & 0x7) << 11) | ((q & 0x7f) << 4) | (index[ri] & 0xf);

	return OK;
}

/*
	int ICS110BFiltersSetADC(VmeModule *module, int sampleRatio, int filters)
	---------------------------------------------------------------------------
	This function is used to enable/disable high-pass filters located on ADC
	converter chips. All channels are equally affected by this state change.
*/
int ICS110BFiltersSetADC(VmeModule *module, int sampleRatio, int filters)
{
	ICS110B_ADC_SPI spiStruct;

	spiStruct.nBytes = 1;
	spiStruct.startAddr = 0x02;

	switch(sampleRatio) {
		case ICS110B_X64: {
			if(filters==ICS110B_ENABLED) { spiStruct.regData[0] = (char)0x04; }
			else { spiStruct.regData[0] = (char)0x0C; }
			break;
		}
		case ICS110B_X128: {
			if(filters==ICS110B_ENABLED) { spiStruct.regData[0] = (char)0x84; }
			else { spiStruct.regData[0] = (char)0x8C; }
			break;
		}
		default: {
			syslog(LOG_INFO,"ICS110BFiltersSetADC: invalid oversampling option=%i\n",sampleRatio);
			return ERROR;
		}
	} /* end switch() */


	if (ICS110BProgramAdcSpi(module, (int)&spiStruct) != 0) {
		return	ERROR;
	}

	return OK;
}

/*
	int ICS110BMasterSet(VmeModule *module, int master)
	---------------------------------------------------------------------------
	This function is used to set the master bit in the control register. It is
	assumed that any non-zero value in the input variable means that you want
	to set the board as the acquisiton master. The VxWorks driver only accepts
	0 and 1 as valid inputs, so when porting between the system be aware of
	this. The read in of the control register isn't currently checked, but the
	return value of the write out is returned to the caller.

	Algorithm
	---------------------------------------------------------------------------
	Read in the current control register
	Set the master bit as needed
	Write out the control register to the board
*/
int ICS110BMasterSet(VmeModule *module, int master) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readD16(module,ICS110B_CTRL_OFFSET);
#endif

	if (master) {
		data |= ICS110B_MASTER;
	} else {
		data &=	~ICS110B_MASTER;
	}

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeD16(module,ICS110B_CTRL_OFFSET,data);
#endif

	return status;
}
/*
    int ICS110BAcquireSourceSet(VmeModule *module, int source)
    ---------------------------------------------------------------------------
	This function is used to set the aquistion source bit in the control
	register. It is assumed that any non-ICS110B_INTERNAL value in the input
	variable means that you want to set the board to use an external source. The
	VxWorks driver only accepts 0 and 1 as valid inputs, so when porting between
	the system be aware of this. The read in of the control register isn't
	currently checked, but the return value of the write out is returned to the
	caller.

	Algorithm
	---------------------------------------------------------------------------
	Read in the current control register
	Set the acquire source bit as needed
	Write out the control register to the board
*/
int ICS110BAcquireSourceSet(VmeModule *module, int source) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readD16(module,ICS110B_CTRL_OFFSET);
#endif

	if(source == ICS110B_INTERNAL) {
		data &= ~ICS110B_ACQUIRE_SOURCE;
	} else {
		data |= ICS110B_ACQUIRE_SOURCE;
	}

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeD16(module,ICS110B_CTRL_OFFSET,data);
#endif

	return status;
}

/*
	int ICS110BOutputModeSet(VmeModule *module, int outMode)
	---------------------------------------------------------------------------
	This function is used to set the output mode value in the control register
	of the ADC. The status of the write out of the new value is returned by the
	function.

	Algorithm
	---------------------------------------------------------------------------
	Pull in the control register into the local board structure.
	Adjust the desired output mode so that any trailing bits are croped off.
	Shift the more value to the correct bit position.
	Zero the output mode in the local control register.
	Desired value for output mode ORed into the local register.
	Local register written out to the card.
*/
int ICS110BOutputModeSet(VmeModule *module, uint16_t outMode) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;
	uint16_t shortOutMode = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

	shortOutMode = (unsigned short)outMode;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readD16(module,ICS110B_CTRL_OFFSET);
#endif

	shortOutMode &= 0x7;
	shortOutMode <<= 3;

	data &= 0xFFC7;
	data |= shortOutMode;

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeD16(module,ICS110B_CTRL_OFFSET,data);
#endif

	return status;
}
/*
	int ICS110BClockSelectSet(VmeModule *module, int clksel)
	---------------------------------------------------------------------------
	This function is used to set the clock select bit in the control
	register. It is assumed that any non-ICS110B_INTERNAL value in the input
	variable means that you want to set the board to use an external select. The
	VxWorks driver only accepts 0 and 1 as valid inputs, so when porting between
	the system be aware of this. The read in of the control register isn't
	currently checked, but the return value of the write out is returned to the
	caller.

	Algorithm
	---------------------------------------------------------------------------
	Read in the current control register
	Set the clock select bit as needed
	Write out the control register to the board
*/
int ICS110BClockSelectSet(VmeModule *module, int clksel) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readD16(module,ICS110B_CTRL_OFFSET);
#endif

	if(clksel == ICS110B_INTERNAL) {
		data &= ~ICS110B_CLOCK_SEL;
	} else {
		data |= ICS110B_CLOCK_SEL;
	}

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeD16(module,ICS110B_CTRL_OFFSET,data);
#endif

	return status;
}


/*
	int ICS110BOversamplingSet(VmeModule *module, int ratio)
	---------------------------------------------------------------------------
	This function is used to set the oversampling select bit in the control
	register. It is assumed that any non-ICS110B_X64 value in the input
	variable means that you want to set the board to use ICS110B_X128. The
	VxWorks driver only accepts 0 and 1 as valid inputs, so when porting between
	the system be aware of this. The read in of the control register isn't
	currently checked, but the return value of the write out is returned to the
	caller.

	Algorithm
	---------------------------------------------------------------------------
	Read in the current control register
	Set the oversampling select bit as needed
	Write out the control register to the board

*/
int ICS110BOversamplingSet(VmeModule *module, int ratio) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readD16(module,ICS110B_CTRL_OFFSET);
#endif

	if(ratio == ICS110B_X64) {
		data &= ~ICS110B_OVERSAMPLING_RATIO;
	} else {
		data |= ICS110B_OVERSAMPLING_RATIO;
	}

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeD16(module,ICS110B_CTRL_OFFSET,data);
#endif

	return status;
}

/*
	Algorithm
	---------------------------------------------------------------------------
	Assign the number of channels to the local board structure.
	Write out the number of channels in the local board structure to the card.
*/
int ICS110BNoChannelsSet(VmeModule *module, int numChan) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

	data = (uint16_t)numChan;
	if((data%2 != 0) || (data > 32) || (data < 2)) {
		syslog(LOG_INFO, "2 < numChannels < 32 and divisible by 2\n");
		return -1;
	}
	data -= 1;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_write(cardFD,cardBase + ICS110B_NUM_CHANS_OFFSET,data);
#else
	writeD16(module,ICS110B_NUM_CHANS_OFFSET,data);
#endif
	return status;
}

int ICS110BFifoRead(VmeModule *module, uint32_t *buffer, uint32_t wordsRequested, uint32_t *wordsRead) {
	int cardFD;
	uint32_t cardBase;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

	status = vme_A24BLT32FIFO_read(cardFD,cardBase+ICS110B_FIFO_OFFSET,buffer,wordsRequested,wordsRead);

	return status;
}

/*
	NOTE: this will place the board in a "default" state:
	i.e. non-acquiring, empty FIFO, etc...
*/
int ICS110BDeviceReset(VmeModule *module) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

	/* don't perform the usual read-modify-write operations here
	 * 'cause we don't care what the current settings are:
	 * 		THIS IS A COMPLETELY DESTRUCTIVE RESET !!!
	 *
	 * 		mmmmm... destructive....
	 */
	data = ICS110B_BOARD_RESET;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_write(cardFD,cardBase + ICS110B_STAT_RESET_OFFSET,data);
#else
	writeD16(module,ICS110B_STAT_RESET_OFFSET,data);
#endif
	return status;
}

/*
	int ICS110BFIFOReset(VmeModule *module)
	---------------------------------------------------------------------------
	This function is used to reset the FIFO on the ADC. It only dumps the
	current content of the FIFO and does not effect the acquistion state of the
	card. It returns the status of the final write to the user.

	Algorithm
	---------------------------------------------------------------------------
	Read in the status register into the local card structure.
	Set the FIFO reset bit.
	Write out the local card structure to the card.
*/
int ICS110BFIFOReset(VmeModule *module) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_STAT_RESET_OFFSET,&data);
#else
	data = readD16(module,ICS110B_STAT_RESET_OFFSET);
#endif

	data |= ICS110B_FIFO_RESET;

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_STAT_RESET_OFFSET,data);
#else
	writeD16(module,ICS110B_STAT_RESET_OFFSET,data);
#endif

	return status;
}

/* NOTE:	the delay values utilized throughout this interface
 * 			are based on the values from the VxWorks driver ASSUMING
 * 			a system "clock-tick" of 60 Hz...
 *
 */
void ICS110BTaskDelay(double seconds) {
	rtems_interval ticks_per_second;
	/* This needed to be defined, since some of the operations for this board need to be time sequenced for the hardware to work correctly */
	rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &ticks_per_second);
	rtems_task_wake_after((int)(seconds*ticks_per_second));
}

/*	FIXME !!!
	---------------------------------------------------------------------------
	This function is used to set the VMEInterrupt enable bit in the control
	register. It is assumed that any non-zero value in the input variable means
	that you want to enable the interrupt. The VxWorks driver only accepts 0 and 1
	as valid inputs, so when porting between the system be aware of this.
*/
int ICS110BInterruptControl(VmeModule *module, int enable) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readD16(module,ICS110B_CTRL_OFFSET);
#endif

	if(enable) {
		data |= ICS110B_VME_INT_ENABLE;
	} else {
		data &= ~ICS110B_VME_INT_ENABLE;
	}

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeD16(module,ICS110B_CTRL_OFFSET,data);
#endif

	return status;
}

/*	FIXME !!! */
int ICS110BSetIrqVector(VmeModule *module, uint8_t vector) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

	data = vector;
	module->irqVector = vector;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_write(cardFD,cardBase + ICS110B_IVECT_OFFSET,data);
#else
	writeD16(module,ICS110B_IVECT_OFFSET,data);
#endif

    return status;

}

int ICS110BStartAcquisition(VmeModule *module) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readD16(module,ICS110B_CTRL_OFFSET);
#endif

	data |= 1;

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeD16(module,ICS110B_CTRL_OFFSET,data);
#endif

	return status;
}

int ICS110BStopAcquisition(VmeModule *module){
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;
	int status = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	status = vme_A24D16_read(cardFD,cardBase + ICS110B_CTRL_OFFSET,&data);
#else
	data = readD16(module,ICS110B_CTRL_OFFSET);
#endif

	data &= ~1;

#ifndef USE_MACRO_VME_ACCESSORS
	status |= vme_A24D16_write(cardFD,cardBase + ICS110B_CTRL_OFFSET,data);
#else
	writeD16(module,ICS110B_CTRL_OFFSET,data);
#endif

	return status;
}

int ICS110BGetStatus(VmeModule *module, uint16_t *status) {
	int cardFD;
	uint32_t cardBase;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	return vme_A24D16_read(cardFD,cardBase + ICS110B_STAT_RESET_OFFSET,status);
#else
	*status = readD16(module,ICS110B_STAT_RESET_OFFSET);
	*status &= 0x00CF;
	return OK;
#endif
}

int ICS110BIsEmpty(VmeModule *module) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	int status = 0;
    status = vme_A24D16_read(cardFD, cardBase + ICS110B_STAT_RESET_OFFSET, &data);
#else
    data = readD16(module,ICS110B_STAT_RESET_OFFSET);
#endif
    return ((data&ICS110B_FIFO_EMPTY)==ICS110B_FIFO_EMPTY);
}

int ICS110BIsFull(VmeModule *module) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	int status = 0;
	status = vme_A24D16_read(cardFD, cardBase + ICS110B_STAT_RESET_OFFSET, &data);
#else
    data = readD16(module,ICS110B_STAT_RESET_OFFSET);
#endif
    return ((data&ICS110B_FIFO_FULL)==ICS110B_FIFO_FULL);
}

int ICS110BIsHalfFull(VmeModule *module) {
	int cardFD;
	uint32_t cardBase;
	uint16_t data = 0;

	cardFD = module->crate->fd;
	cardBase = module->vmeBaseAddr;

#ifndef USE_MACRO_VME_ACCESSORS
	int status = 0;
	status = vme_A24D16_read(cardFD, cardBase + ICS110B_STAT_RESET_OFFSET, &data);
#else
    data = readD16(module,ICS110B_STAT_RESET_OFFSET);
#endif
    return ((data&ICS110B_FIFO_HALF_FULL)==ICS110B_FIFO_HALF_FULL);
}
