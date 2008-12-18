/*
 * VmeCrate.cpp
 *
 *  Created on: Dec 4, 2008
 *      Author: chabotd
 */

#include <VmeCrate.h>
#include <OrbitControlException.h>

#include <stdio.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>

#include <sis1100_api.h>
#include <utils.h>
#include <string>

using std::string;
/**
 * Instantiate a new VmeCrate object. Based on the crateId provided,
 * a file descriptor is opened on "/dev/sis1100_crateId".
 *
 * Note, this object maps ALL of A24 VME space
 * into the PC's address space... and ONLY A24 space (for now!)
 *
 * Note also that this ctor will throw an exception on failure!
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
		string err("Failed to open ");
		err += devName;
		syslog(LOG_INFO, "%s: %s\n",err.c_str(),strerror(fd));
		err += ": ";
		err += strerror(fd);
		throw OrbitControlException(err.c_str());
	}

	this->fd = fd;
	//map A24 space into PC memory:
	int rc = vme_set_mmap_entry(fd, 0x00000000,
							0x39/*AM*/,0xFF010800/*hdr*/,
							(1<<24)-1,&a24BaseAddr);
	if(rc) {
		string err("vme_set_mmap_entry() failure!! ");
		syslog(LOG_INFO, "%scrate#=%d rc=%d\n",err.c_str(),crateId,rc);
		throw OrbitControlException(err.c_str());
	}
	syslog(LOG_INFO, "crate# %d, VME A24 base-address=%p",crateId, a24BaseAddr);
	/* do a VME System-Reset */
	vmesysreset(fd);
}

VmeCrate::~VmeCrate() {
	int rc = vme_clr_mmap_entry(fd, &a24BaseAddr, (1<<24)-1);
	if(rc) {
		syslog(LOG_INFO,"vme_clr_mmap_entry() failure!! crate#=%d rc=%d\n",id,rc);
	}
	close(fd);
	syslog(LOG_INFO,"VmeCrate %d dtor!!\n",id);
}
