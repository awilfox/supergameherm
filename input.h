#ifndef __INPUT_H_
#define __INPUT_H_

#include <stdint.h>	/* uint[XX]_t */


uint8_t joypad_read(emulator_state *, uint16_t);
void joypad_write(emulator_state *, uint16_t, uint8_t);

#endif /*!__INPUT_H_*/