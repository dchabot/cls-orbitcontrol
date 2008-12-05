/*
 * VmeModule.h
 *
 *  Created on: Dec 5, 2008
 *      Author: chabotd
 */

#ifndef VMEMODULE_H_
#define VMEMODULE_H_

#include <VmeCrate.h>
#include <stdint.h>


class VmeModule {
public:
	VmeModule(VmeCrate& c, uint32_t vmeAddr);
	virtual ~VmeModule();
//class-attribute accessors
	VmeCrate & getCrate() const;
	void setCrate(VmeCrate & crate);
	char *getName() const;
	void setName(char *name);
	char *getType() const;
	void setType(char *type);
	uint8_t getSlot() const;
	void setSlot(uint8_t slot);
	uint32_t getVmeBaseAddr() const;
	void setVmeBaseAddr(uint32_t vmeBaseAddr);
	uint32_t getPcA16BaseAddr() const;
	void setPcA16BaseAddr(uint32_t pcA16BaseAddr);
	uint32_t getPcA24BaseAddr() const;
	void setPcA24BaseAddr(uint32_t pcA24BaseAddr);
	uint32_t getPcA32BaseAddr() const;
	void setPcA32BaseAddr(uint32_t pcA32BaseAddr);
	uint8_t getIrqLevel() const;
	void setIrqLevel(uint8_t irqLevel);
	uint8_t getIrqVector() const;
	void setIrqVector(uint8_t irqVector);
//VME A32 single-cycle accessors
	void writeA32D32(uint32_t offset, uint32_t val);
	void writeA32D16(uint32_t offset, uint16_t val);
	void writeA32D8(uint32_t offset, uint8_t val);
	uint32_t readA32D32(uint32_t offset);
	uint16_t readA32D16(uint32_t offset);
	uint8_t	 readA32D8(uint32_t offset);
//VME A24 single-cycle accessors
	void writeA24D32(uint32_t offset, uint32_t val);
	void writeA24D16(uint32_t offset, uint16_t val);
	void writeA24D8(uint32_t offset, uint8_t val);
	uint32_t readA24D32(uint32_t offset);
	uint16_t readA24D16(uint32_t offset);
	uint8_t	 readA24D8(uint32_t offset);
//VME A16 single-cycle accessors
	void writeA16D32(uint32_t offset, uint32_t val);
	void writeA16D16(uint32_t offset, uint16_t val);
	void writeA16D8(uint32_t offset, uint8_t val);
	uint32_t readA16D32(uint32_t offset);
	uint16_t readA16D16(uint32_t offset);
	uint8_t	 readA16D8(uint32_t offset);

private:
// default and copy-constructors are private
	VmeModule();
	VmeModule(const VmeModule&);
	VmeCrate& crate;
	char *name;
	char *type;
	uint8_t slot;
	uint32_t vmeBaseAddr;
	uint32_t pcA16BaseAddr;
	uint32_t pcA24BaseAddr;
	uint32_t pcA32BaseAddr;
	uint8_t irqLevel;
	uint8_t irqVector;
};

#endif /* VMEMODULE_H_ */
