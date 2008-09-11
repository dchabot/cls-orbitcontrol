#include <stdint.h>
#include <tscDefs.h>

/* XXX tcsTicksPerSecond varies with clock speed and must be set prior to using this fcn ! */
void usecSpinDelay(uint32_t usecDelay) {
	extern double tscTicksPerSecond;
	const double tscTicksPerUSec = tscTicksPerSecond/1000000.0;
	uint64_t tscDelay = (uint64_t)(usecDelay*tscTicksPerUSec);
	uint64_t now,then;
	
	rdtscll(then);
	now=then;
	/* spin... weeeeeeeeeeeeeeeee... */
	while((now-then)<tscDelay) {
		rdtscll(now);
	}
}
