#ifndef TSCDEFS_H_
#define TSCDEFS_H_

#include <stdint.h>

#ifdef __i386__
/*from linux includes (msr.h)... */
#define rdtsc(low,high) __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))
#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

#if 0
#ifdef __rtems__
#include <bsp.h>

/* 
 * General format of software interaction with the i8254 PIT is:
 * 1) write CONTROL byte to MODE port (base + 3)
 * 2) read/write data byte from COUNTER port
 */

static inline void Initialize8254Timer1(uint16_t usec) {
	/* i386 BSP uses TIMER_CNTR0, leaving 1 and 2 open for user apps */
	outport_byte(TIMER_MODE, TIMER_SEL1 | TIMER_16BIT | TIMER_INTTC); 
	outport_byte(TIMER_CNTR1, (US_TO_TICK(usec)>>0) & 0xFF); /* lsb */
	outport_byte(TIMER_CNTR1, (US_TO_TICK(usec)>>8) & 0xFF); /* msb */
}

/* NOTE: this function will delay for approximately usec + 6 microseconds */
static inline void Delay8254Timer1(uint16_t usec) {
	uint8_t status = 0;
	
	Initialize8254Timer1(usec);
	do {
		outport_byte(TIMER_MODE, TIMER_RD_BACK | RB_COUNT_1 | RB_NOT_COUNT);
		inport_byte(TIMER_CNTR1, status);
		if(status & RB_OUTPUT) { break; }
	} while(1);
	Initialize8254Timer1(0);
}

#endif /*__rtems__*/
#endif /* if 0 */

#endif //end ifdef __i386__

#endif /*TSCDEFS_H_*/
