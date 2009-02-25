/*
 * AdcIsr.cc
 *
 *  Created on: Dec 16, 2008
 *      Author: chabotd
 */

#include <AdcIsr.h>
#include <sis1100_api.h>
#include <syslog.h>
#include <OrbitControlException.h>


AdcIsr::AdcIsr(Ics110blModule* mod, rtems_id id) :
	//ctor-initializer list
	bid(id),adc(mod)
{
	int rc;
	rc = vme_set_isr(adc->getCrate()->getFd(),
						ICS110B_DEFAULT_IRQ_VECTOR/*vector*/,
						isr/*handler*/,
						(void*)this/*handler arg*/);
	if(rc<0) {
		syslog(LOG_INFO, "Failed to set ADC Isr, crate# %d\n",adc->getCrate()->getId());
		throw OrbitControlException("AdcIsr: vme_set_isr() failed!!",rc);
	}
	/* arm the appropriate VME interrupt level at each sis3100 & ADC... */
	adc->setIrqVector(ICS110B_DEFAULT_IRQ_VECTOR);
	rc = vme_enable_irq_level(adc->getCrate()->getFd(), adc->getIrqLevel());
	if(rc<0)
		throw OrbitControlException("AdcIsr: vme_enable_irq_level() failed!!",rc);
	adc->enableInterrupt();
}

AdcIsr::~AdcIsr() {
	int rc = vme_disable_irq_level(adc->getCrate()->getFd(), adc->getIrqLevel());
	rc |= vme_clr_isr(adc->getCrate()->getFd(), adc->getIrqVector());
	if(rc) {
		//DO NOT THROW EXCEPTIONS FROM DTORS !!!!!!!
		syslog(LOG_INFO, "Failed to remove ADC Isr\n");
	}
}

/***************************** private interface ***********************************/
/*
 * NOTE: isr(void*,uint8_t) will be called from a C-func ptr
 * 		 in a high priority thread!
 */
void AdcIsr::isr(void *arg, uint8_t vector) {
	AdcIsr *parg = (AdcIsr*)arg;
	rtems_status_code rc;

	/* disable irq on the board*/
	parg->adc->disableInterrupt();
	/* inform the OrbitController of this event*/
	rc = rtems_barrier_wait(parg->bid,RTEMS_NO_TIMEOUT);
	//don't throw() from here (interrupt context):
	if(rc != RTEMS_SUCCESSFUL) {
		syslog(LOG_INFO,"AdcIsr -- rtems_barrier_wait() failure--%s",rtems_status_text(rc));
	}
}


