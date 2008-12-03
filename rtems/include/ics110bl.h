#ifndef ICS110BL_H_
#define ICS110BL_H_

#include <stdint.h>
#include <vmeDefs.h>

#define ICS110B_DEFAULT_BASEADDR	0x00D00000

#define ICS110B_DEFAULT_IRQ_LEVEL	3 /* factory default */
#define ICS110B_DEFAULT_IRQ_VECTOR	123

#define ICS110B_IRQ_ENABLE	1
#define ICS110B_IRQ_DISABLE	0

#define ICS110B_FREQ_TOO_LOW     -200
#define ICS110B_FREQ_TOO_HIGH    -201


#define ADC_PER_VOLT 419430.3

/* XXX -- fifo length is 1024*128 Bytes (128 kB), NOT 1024*64 words.
 * Unless, they mean 64 kWords where a "word" is 2 bytes... marketing @ssh0lz...*/
#define HALF_FIFO_LENGTH 16256 /* 16 kWords; word-size is 4 bytes */

#define OK 0
#define ERROR 1

#define ICS110B_ID 0

#define MAX_NUM_ADC_CHANNELS 32

/*
        CLS-110 Status register masks
        These bits are active high.
*/
#define INTERNAL_CLOCK 0
#define EXTERNAL_CLOCK 1

/*
    Bit positions in Status/Reset Register
*/
#define ICS110B_FIFO_EMPTY      	1
#define ICS110B_FIFO_HALF_FULL  	2
#define ICS110B_FIFO_FULL       	4
#define ICS110B_ACQUIRE_ENABLED 	8
#define ICS110B_FIFO_RESET      	(1<<6)
#define ICS110B_BOARD_RESET			(1<<7)
/*
    Bit positions in Control Register
*/
#define ICS110B_ACQUIRE				1
#define ICS110B_MASTER				2
#define ICS110B_ACQUIRE_SOURCE  	4
#define ICS110B_OUTMODE_MODE1   	0x0008
#define ICS110B_OUTMODE_MODE2   	0x0010
#define ICS110B_OUTMODE_MODE3   	0x0020
#define ICS110B_SYNC_ENABLE			0x0040
#define ICS110B_CLOCK_SEL			0x0080
#define ICS110B_OVERSAMPLING_RATIO	0x0100
#define ICS110B_VSB_INT_ENABLE		0x4000
#define ICS110B_VME_INT_ENABLE		0x8000

/*
    Bit positions in ADC Configuration Register
*/
#define ICS110B_CHIP_SEL_0		0x0001
#define ICS110B_CHIP_SEL_1		0x0002
#define ICS110B_CHIP_SEL_2		0x0004
#define ICS110B_CAL_ENABLE		0x0040
#define ICS110B_CLOCK_SEL		0x0080

/*
    Bit positions in VSB Status Register
*/
#define ICS110B_GLOBAL_ADDRESS1		0x01000000
#define ICS110B_GLOBAL_ADDRESS2		0x02000000
#define ICS110B_GLOBAL_ADDRESS3		0x04000000
#define ICS110B_VSB_FIFO_EMPTY		0x10000000
#define ICS110B_VSB_FIFO_HALF_FULL	0x20000000
#define ICS110B_VSB_FIFO_FULL		0x40000000
#define ICS110B_VSB_SB_INT			0x80000000


/*
        CLS-110B Control register settings
*/
#define ICS110B_DISABLED        0
#define ICS110B_ENABLED         1
#define ICS110B_INTERNAL        0
#define ICS110B_EXTERNAL        1
#define ICS110B_VME_PACKED      0
#define ICS110B_VSB_PACKED      1
#define ICS110B_VME_UNPACKED    2
#define ICS110B_VSB_UNPACKED    3
#define ICS110B_FPDP_MASTER     4
#define ICS110B_FPDP_SLAVE      5
#define ICS110B_MASTER_ON		1
#define ICS110B_MASTER_OFF		0
#define ICS110B_X64             0
#define ICS110B_X128            1

/*
    Location of registers in memoryspace of ADC relative to the card base
*/
#define	ICS110B_FIFO_OFFSET                 0x00000
#define ICS110B_STAT_RESET_OFFSET           0x0FE00
#define ICS110B_CTRL_OFFSET                 0x0FE02
#define ICS110B_IVECT_OFFSET                0x0FE04
#define ICS110B_NUM_CHANS_OFFSET            0x0FE06
#define ICS110B_DECIMATION_OFFSET           0x0FE08
#define ICS110B_ADC_SPI_OFFSET              0x0FE0A
#define ICS110B_IP_SEL_OFFSET               0x0FE0C
#define ICS110B_FPDP_ADDR_OFFSET            0x0FE0E
#define ICS110B_FPDP_CHANS_OFFSET           0x0FE10
#define ICS110B_BLOCK_COUNT_OFFSET          0x0FE12
#define ICS110B_VSB_BASE_OFFSET             0x0FE14
#define ICS110B_VSB_SPACE_OFFSET            0x0FE16
#define ICS110B_SYNC_WORD_OFFSET            0x0FE18
#define ICS110B_CLOCK_OFFSET                0x0FE1C

/*
    Error codes
*/
#define  ICS110B_ALLOCATE_DEVICE_FAILED      0xFF000002
#define  ICS110B_INVALID_INT_VECTOR          0xFF000003
#define  ICS110B_INVALID_INT_REQUEST_LEVEL   0xFF000004
#define  ICS110B_INVALID_BOARD               0xFF000005
#define  ICS110B_DEVICE_ALREADY_CREATED      0xFF000006
#define  ICS110B_CREATE_SEMAPHORE_FAILED     0xFF000007
#define  ICS110B_HARDWARE_INIT_FAILED        0xFF000008
#define  ICS110B_INT_HANDLER_INSTALL_FAILED  0xFF000009
#define  ICS110B_ADD_DEVICE_FAILED           0xFF00000A
#define  ICS110B_DEVICE_NOT_CREATED          0xFF00000B
#define  ICS110B_DEVICE_IN_USE               0xFF00000C
#define  ICS110B_DEVICE_NOT_IN_USE           0xFF00000D

/*
    Various structures that appear in the ADC calculations that are not 16-bit or 32-bit registers
*/
typedef struct
        {
        char    regData[0x18];
        int     startAddr;
        int     nBytes;
}ICS110B_ADC_SPI;

typedef struct
        {
        double  chanGain[0x20];
}ICS110B_ADC_GAIN;

typedef struct
        {
        unsigned long   code;
        int     index;

}ICS110B_ADC_GAIN_CODE;

typedef struct
{
    uint16_t	stat_reset;
    uint16_t	ctrl;
    uint16_t	ivect;
    uint16_t	num_chans;
    uint16_t	decimation;
    uint16_t	adc_spi;
    uint16_t	ip_sel;
    uint16_t	fpdp_addr;
    uint16_t	fpdp_chans;
    uint16_t	block_count;
    uint16_t	VSB_base;
    uint16_t	VSB_space;
    uint32_t	sync_word;
    uint32_t	clock;
}ICS110B_ADC_REG;

typedef struct
{
    int				cardFD;
    int 			cardBase;
    ICS110B_ADC_REG		reg;
    ICS110B_ADC_SPI 		adc_spi;
    ICS110B_ADC_GAIN		adc_gain;
    ICS110B_ADC_GAIN_CODE	adc_gain_code;

}ICS110B_ADC_DEV;

/*
    ===========================================================================
    Prototypes
    ===========================================================================
*/
int InitICS110BL(VmeModule *module, double reqSampleRate, double *trueSampleRate, int clockingSelect, int AcquireSelect, /*uint8_t irqVector,*/ int numChannels);
int programFoxRate(VmeModule *module, int foxWordPtr);
int ICS110BProgramAdcSpi(VmeModule *module, int arg);
int ICS110BCalibrateAdc(VmeModule *module, int sampleRatio, double *pSampleRate);
int ICS110BSampleRateSet(VmeModule *module, double reqRate, double *actRate, int sampleRatio);
int ICS110BDemodReset(VmeModule *module);
int ICS110BCalcFoxWord(double *reqFreq, double *actFreq, long *progWord);
int ICS110BMasterSet(VmeModule *module, int master);
int ICS110BAcquireSourceSet(VmeModule *module, int source);
int ICS110BOutputModeSet(VmeModule *module, uint16_t outMode);
int ICS110BClockSelectSet(VmeModule *module, int clksel);
int ICS110BOversamplingSet(VmeModule *module, int ratio);
int ICS110BNoChannelsSet(VmeModule *module, int numChan);
int ICS110BFiltersSetADC(VmeModule *module, int sampleRatio, int filters);
int ICS110BFifoRead(VmeModule *module, uint32_t *buffer, uint32_t wordsRequested, uint32_t *wordsRead);
int ICS110BDeviceReset(VmeModule *module);
int ICS110BFIFOReset(VmeModule *module);
void ICS110BTaskDelay(double seconds);
int ICS110BSetIrqVector(VmeModule *module, uint8_t vector);
int ICS110BInterruptControl(VmeModule *module, int enable);

int ICS110BStartAcquisition(VmeModule *module);
int ICS110BStopAcquisition(VmeModule *module);
int ICS110BGetStatus(VmeModule *module, uint16_t *status);
int ICS110BIsEmpty(VmeModule *module);
int ICS110BIsFull(VmeModule *module);
int ICS110BIsHalfFull(VmeModule *module);
#endif /*ICS110BL_H_*/
