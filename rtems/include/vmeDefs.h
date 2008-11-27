#ifndef VMEDEFS_H_
#define VMEDEFS_H_

#include <stdint.h>

#define numSlotsPerCrate 21

typedef struct {
	uint32_t id;
	int fd;
	uint32_t a24BaseAddr;
	void *moduleList[numSlotsPerCrate];
} VmeCrate;

typedef struct {
	VmeCrate *crate;
	char *name;
	char *type;
	uint8_t slot;
	uint32_t vmeBaseAddr;
	uint32_t pcBaseAddr;
	uint8_t irqLevel;
	uint8_t irqVector;
} VmeModule;

/* XXX Using these macros in place of equivalent sis1100_api calls
 * involves a compromise: while you gain speed of execution (pointer dereference
 * vs. a complex function), you lose error detection and thread-safety... :-(
 *
 * i.e. use the macros only if you really know what you're doing...
 */
#define USE_MACRO_VME_ACCESSORS

#define VmeRead_32(mod, reg, ...)  *(volatile uint32_t *)(mod->pcBaseAddr + mod->vmeBaseAddr + reg)
#define VmeRead_16(mod, reg, ...)   *(volatile uint16_t *)(mod->pcBaseAddr + mod->vmeBaseAddr + reg)
#define VmeRead_8(mod, reg, ...)   *(volatile uint8_t *)(mod->pcBaseAddr + mod->vmeBaseAddr + reg)

#define VmeWrite_32(mod, reg, data, ...)  *(volatile uint32_t *)(mod->pcBaseAddr + mod->vmeBaseAddr + reg) = (data)
#define VmeWrite_16(mod, reg, data, ...)  *(volatile uint16_t *)(mod->pcBaseAddr + mod->vmeBaseAddr + reg) = (data)
#define VmeWrite_8(mod, reg, data, ...)  *(volatile uint8_t *)(mod->pcBaseAddr + mod->vmeBaseAddr + reg) = (data)

#endif /*VMEDEFS_H_*/
