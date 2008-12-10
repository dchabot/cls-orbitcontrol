/*
 * VmeCrate.h
 *
 *  Created on: Dec 4, 2008
 *      Author: chabotd
 */

#ifndef VMECRATE_H_
#define VMECRATE_H_

#include <stdint.h>

class VmeCrate {
public:
	VmeCrate(uint32_t crateId);
	virtual ~VmeCrate();

	uint32_t getId() const;
	void setId(uint32_t id);
	int getFd() const;
	uint32_t getA16BaseAddr() const;
	uint32_t getA24BaseAddr() const;
	uint32_t getA32BaseAddr() const;
private:
//default and copy-constructors are private
	VmeCrate();
	VmeCrate(const VmeCrate&);
	uint32_t id;
	int fd;
	uint32_t a16BaseAddr;
	uint32_t a24BaseAddr;
	uint32_t a32BaseAddr;
};

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

#endif /* VMECRATE_H_ */
