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
//these will probably have to access hardware:
//thus, we force subclasses to override and forbid any VmeModule instances.
	virtual uint8_t getIrqLevel() const;
	virtual void setIrqLevel(uint8_t irqLevel);
	virtual uint8_t getIrqVector() const;
	virtual void setIrqVector(uint8_t irqVector);

//class-attribute accessors
	VmeCrate & getCrate() const;
	void setCrate(VmeCrate & crate);
	char *getName() const;
	void setName(char *name);
	const char *getType() const;
	void setType(const char *type);
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
	const char *type;
	uint8_t slot;
	uint32_t vmeBaseAddr;
	uint32_t pcA16BaseAddr;
	uint32_t pcA24BaseAddr;
	uint32_t pcA32BaseAddr;
	uint8_t irqLevel;
	uint8_t irqVector;
};


//class-attribute accessors
inline VmeCrate& VmeModule::getCrate() const {
   return crate;
}

inline void VmeModule::setCrate(VmeCrate& crate) {
   this->crate = crate;
}

inline char* VmeModule::getName() const {
   return name;
}

inline void VmeModule::setName(char *name) {
   this->name = name;
}

inline const char* VmeModule::getType() const {
   return type;
}

inline void VmeModule::setType(const char *type) {
   this->type = type;
}

inline uint8_t VmeModule::getSlot() const {
   return slot;
}

inline void VmeModule::setSlot(uint8_t slot) {
   this->slot = slot;
}

inline uint32_t VmeModule::getVmeBaseAddr() const {
   return vmeBaseAddr;
}

inline void VmeModule::setVmeBaseAddr(uint32_t vmeBaseAddr) {
   this->vmeBaseAddr = vmeBaseAddr;
}

inline uint32_t VmeModule::getPcA16BaseAddr() const {
   return pcA16BaseAddr;
}

inline void VmeModule::setPcA16BaseAddr(uint32_t pcA16BaseAddr) {
   this->pcA16BaseAddr = pcA16BaseAddr;
}

inline uint32_t VmeModule::getPcA24BaseAddr() const {
   return pcA24BaseAddr;
}

inline void VmeModule::setPcA24BaseAddr(uint32_t pcA24BaseAddr) {
   this->pcA24BaseAddr = pcA24BaseAddr;
}

inline uint32_t VmeModule::getPcA32BaseAddr() const {
   return pcA32BaseAddr;
}

inline void VmeModule::setPcA32BaseAddr(uint32_t pcA32BaseAddr) {
   this->pcA32BaseAddr = pcA32BaseAddr;
}

inline uint8_t VmeModule::getIrqLevel() const {
	return irqLevel;
}

inline void VmeModule::setIrqLevel(uint8_t level) {
	this->irqLevel = level;
}

inline uint8_t VmeModule::getIrqVector() const {
	return irqVector;
}

inline void VmeModule::setIrqVector(uint8_t vector) {
	irqVector = vector;
}

//VME A32 space single-cycle read/write definitions
inline void VmeModule::writeA32D32(uint32_t offset, uint32_t val) {
	*(volatile uint32_t *)(pcA32BaseAddr + vmeBaseAddr + offset) = (val);
}

inline void VmeModule::writeA32D16(uint32_t offset, uint16_t val) {
	*(volatile uint16_t *)(pcA32BaseAddr + vmeBaseAddr + offset) = (val);
}

inline void VmeModule::writeA32D8(uint32_t offset, uint8_t val) {
	*(volatile uint8_t *)(pcA32BaseAddr + vmeBaseAddr + offset) = (val);
}

inline uint32_t VmeModule::readA32D32(uint32_t offset) {
	return *(volatile uint32_t *)(pcA32BaseAddr + vmeBaseAddr + offset);
}

inline uint16_t VmeModule::readA32D16(uint32_t offset) {
	return *(volatile uint16_t *)(pcA32BaseAddr + vmeBaseAddr + offset);
}

inline uint8_t VmeModule::readA32D8(uint32_t offset) {
	return *(volatile uint8_t *)(pcA32BaseAddr + vmeBaseAddr + offset);
}

//VME A24 space single-cycle read/write definitions
inline void VmeModule::writeA24D32(uint32_t offset, uint32_t val) {
	*(volatile uint32_t *)(pcA24BaseAddr + vmeBaseAddr + offset) = (val);
}

inline void VmeModule::writeA24D16(uint32_t offset, uint16_t val) {
	*(volatile uint16_t *)(pcA24BaseAddr + vmeBaseAddr + offset) = (val);
}

inline void VmeModule::writeA24D8(uint32_t offset, uint8_t val) {
	*(volatile uint8_t *)(pcA24BaseAddr + vmeBaseAddr + offset) = (val);
}

inline uint32_t VmeModule::readA24D32(uint32_t offset) {
	return *(volatile uint32_t *)(pcA24BaseAddr + vmeBaseAddr + offset);
}

inline uint16_t VmeModule::readA24D16(uint32_t offset) {
	return *(volatile uint16_t *)(pcA24BaseAddr + vmeBaseAddr + offset);
}

inline uint8_t VmeModule::readA24D8(uint32_t offset) {
	return *(volatile uint8_t *)(pcA24BaseAddr + vmeBaseAddr + offset);
}

//VME A16 space single-cycle read/write definitions
inline void VmeModule::writeA16D32(uint32_t offset, uint32_t val) {
	*(volatile uint32_t *)(pcA16BaseAddr + vmeBaseAddr + offset) = (val);
}

inline void VmeModule::writeA16D16(uint32_t offset, uint16_t val) {
	*(volatile uint16_t *)(pcA16BaseAddr + vmeBaseAddr + offset) = (val);
}

inline void VmeModule::writeA16D8(uint32_t offset, uint8_t val) {
	*(volatile uint8_t *)(pcA16BaseAddr + vmeBaseAddr + offset) = (val);
}

inline uint32_t VmeModule::readA16D32(uint32_t offset) {
	return *(volatile uint32_t *)(pcA16BaseAddr + vmeBaseAddr + offset);
}

inline uint16_t VmeModule::readA16D16(uint32_t offset) {
	return *(volatile uint16_t *)(pcA16BaseAddr + vmeBaseAddr + offset);
}

inline uint8_t VmeModule::readA16D8(uint32_t offset) {
	return *(volatile uint8_t *)(pcA16BaseAddr + vmeBaseAddr + offset);
}

#endif /* VMEMODULE_H_ */
