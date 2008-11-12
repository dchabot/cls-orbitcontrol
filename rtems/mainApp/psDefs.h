/*
 * psDefs.h
 *
 *  Created on: Sep 15, 2008
 *      Author: chabotd
 */

#ifndef PSDEFS_H_
#define PSDEFS_H_

/*#define DAC_AMP_CONV_FACTOR 42234.31*/
#define DAC_AMP_CONV_FACTOR 1

#define OUTPUT_REG_OFFSET 0x08
#define INPUT_REG_OFFSET 0x04
#define CONTROL_REG_OFFSET 0x02

#define PS_CHANNEL_OFFSET 24
#define PS_CHANNEL_MASK 0xF

#define PS_LATCH 0x20000000
#define DROP_PS_LATCH 0xDFFFFFFF

#define UPDATE 0x80000000
#define DROP_UPDATE 0x7FFFFFFF

#define ISO_DELAY 50 /* i.e. delay in microseconds for writes to optically isolated circuits */

#endif /* PSDEFS_H_ */
