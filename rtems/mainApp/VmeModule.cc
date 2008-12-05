/*
 * VmeModule.cpp
 *
 *  Created on: Dec 5, 2008
 *      Author: chabotd
 */

#include <VmeModule.h>
#include <cstdlib>

VmeModule::VmeModule(VmeCrate& c, uint32_t vmeAddr) :
	//constructor-initializer list
	crate(c),name(NULL),type(NULL),
	slot(0),vmeBaseAddr(vmeAddr),
	pcA16BaseAddr(NULL), pcA24BaseAddr(NULL),pcA32BaseAddr(NULL),
	irqLevel(0),irqVector(0)
{
	pcA16BaseAddr = crate.getA16BaseAddr();
	pcA24BaseAddr = crate.getA24BaseAddr();
	pcA32BaseAddr = crate.getA32BaseAddr();
}

VmeModule::~VmeModule() {
	crate=NULL;
}

//class-attribute accessors
VmeCrate& VmeModule::getCrate() const
{
   return crate;
}

void VmeModule::setCrate(VmeCrate & crate) {
   this->crate = crate;
}

char* VmeModule::getName() const {
   return name;
}

void VmeModule::setName(char *name) {
   this->name = name;
}

char* VmeModule::getType() const {
   return type;
}

void VmeModule::setType(char *type) {
   this->type = type;
}

uint8_t VmeModule::getSlot() const {
   return slot;
}

void VmeModule::setSlot(uint8_t slot) {
   this->slot = slot;
}

uint32_t VmeModule::getVmeBaseAddr() const {
   return vmeBaseAddr;
}

void VmeModule::setVmeBaseAddr(uint32_t vmeBaseAddr) {
   this->vmeBaseAddr = vmeBaseAddr;
}

uint32_t VmeModule::getPcA16BaseAddr() const {
   return pcA16BaseAddr;
}

void VmeModule::setPcA16BaseAddr(uint32_t pcA16BaseAddr) {
   this->pcA16BaseAddr = pcA16BaseAddr;
}

uint32_t VmeModule::getPcA24BaseAddr() const {
   return pcA24BaseAddr;
}

void VmeModule::setPcA24BaseAddr(uint32_t pcA24BaseAddr) {
   this->pcA24BaseAddr = pcA24BaseAddr;
}

uint32_t VmeModule::getPcA32BaseAddr() const {
   return pcA32BaseAddr;
}

void VmeModule::setPcA32BaseAddr(uint32_t pcA32BaseAddr) {
   this->pcA32BaseAddr = pcA32BaseAddr;
}

uint8_t VmeModule::getIrqLevel() const {
   return irqLevel;
}

void VmeModule::setIrqLevel(uint8_t irqLevel) {
   this->irqLevel = irqLevel;
}

uint8_t VmeModule::getIrqVector() const {
   return irqVector;
}

void VmeModule::setIrqVector(uint8_t irqVector) {
   this->irqVector = irqVector;
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
