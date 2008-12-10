#ifndef VMIC2536_H_
#define VMIC2536_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#ifndef __cplusplus
#include <vmeDefs.h>
#endif

/* XXX -- these boards require only 0x00-0x0F of address space...
 * Why not just use A16 space instead of polluting the A24-space ???
 */
#define VMIC_2536_DEFAULT_BASE_ADDR	0x00700000

/* this setting will kill the fail-LED and take the board out of "test mode"... */
#define VMIC_2536_INIT 0xE000

#define VMIC_2536_BOARD_ID	0x2000

/* register offsets */
#define VMIC_2536_BOARDID_REG_OFFSET 0x00 /* should read-back as 0x2000 */
#define VMIC_2536_CONTROL_REG_OFFSET 0x02
#define VMIC_2536_INPUT_REG_OFFSET 0x04
#define VMIC_2536_OUTPUT_REG_OFFSET 0x08

#ifndef __cplusplus
int VMIC2536_Init(VmeModule *module);
int VMIC2536_setControl(VmeModule *module, uint16_t value);
int VMIC2536_getControl(VmeModule *module, uint16_t* value);
int VMIC2536_setOutput(VmeModule *module, uint32_t value);
int VMIC2536_getOutput(VmeModule *module, uint32_t* value);
int VMIC2536_getBoardID(VmeModule *module, uint16_t* value);
#endif

#ifdef __cplusplus
}
#endif

#endif /*VMIC2536_H_*/
