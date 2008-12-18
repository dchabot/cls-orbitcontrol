/*
 * VmeModule.cpp
 *
 *  Created on: Dec 5, 2008
 *      Author: chabotd
 */

#include <VmeModule.h>
#include <cstdlib>
#include <syslog.h>

VmeModule::VmeModule(VmeCrate* c, uint32_t vmeAddr) :
	//constructor-initializer list
	crate(c),name(NULL),type(NULL),
	slot(0),vmeBaseAddr(vmeAddr),
	pcA16BaseAddr(NULL), pcA24BaseAddr(NULL),pcA32BaseAddr(NULL),
	irqLevel(0),irqVector(0)
{
	pcA16BaseAddr = crate->getA16BaseAddr();
	pcA24BaseAddr = crate->getA24BaseAddr();
	pcA32BaseAddr = crate->getA32BaseAddr();
}

VmeModule::~VmeModule() {
	syslog(LOG_INFO,"Destroying VME module, type=%s",type);
}

