/*
 * VmeCrate.cpp
 *
 *  Created on: Dec 4, 2008
 *      Author: chabotd
 */

#include <VmeCrate.h>

#include <stdio.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>

#include <sis1100_api.h>
#include <utils.h>

/**
 * Instantiate a new VmeCrate object. Based on the crateId provided,
 * a file descriptor is opened on "/dev/sis1100_crateId".
 *
 * Note, this object maps ALL of A24 VME space
 * into the PC's address space... and ONLY A24 space (for now!)
 *
 * @param crateId
 * @return
 */
VmeCrate::VmeCrate(uint32_t crateId) :
	//constructor-initializer list
	id(crateId),fd(-1),
	a16BaseAddr(NULL),a24BaseAddr(NULL),a32BaseAddr(NULL)
{
	char devName[64];
	extern int errno;

	sprintf(devName, "/dev/sis1100_%d",(int)crateId);
	int fd = open(devName, O_RDWR, 0);
	if(fd < 0) {
		syslog(LOG_INFO, "Failed to open %s: %s\n",devName, strerror(fd));
		//FatalErrorHandler(0);
	}

	this->fd = fd;
#ifdef USE_MACRO_VME_ACCESSORS
	int rc = vme_set_mmap_entry(fd, 0x00000000,
							0x39/*AM*/,0xFF010800/*hdr*/,
							(1<<24)-1,&(this->a24BaseAddr));
	if(rc) {
		syslog(LOG_INFO, "vme_set_mmap_entry() failure!! crate#=%d rc=%d\n",crateId,rc);
	}
	syslog(LOG_INFO, "crate# %d, VME A24 base-address=%p",crateId, this->a24BaseAddr);
#endif
	/* do a VME System-Reset */
	vmesysreset(fd);
}

VmeCrate::~VmeCrate() {
#ifdef USE_MACRO_VME_ACCESSORS
	int rc = vme_clr_mmap_entry(this->fd, &this->a24BaseAddr, (1<<24)-1);
	if(rc) {
		syslog(LOG_INFO,"vme_clr_mmap_entry() failure!! crate#=%d rc=%d\n",this->id,rc);
	}
#endif
	close(this->fd);
}

inline uint32_t VmeCrate::getId() const {
	return id;
}

inline void VmeCrate::setId(uint32_t id) {
	this->id = id;
}

inline int VmeCrate::getFd() const {
	return fd;
}

inline uint32_t VmeCrate::getA16BaseAddr() const {
	return a16BaseAddr;
}

inline uint32_t VmeCrate::getA24BaseAddr() const {
	return a24BaseAddr;
}

inline uint32_t VmeCrate::getA32BaseAddr() const {
	return a32BaseAddr;
}

