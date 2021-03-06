#ifndef __SERIO_H_
#define __SERIO_H_

#include "config.h"	// macros,  uint[XX]_t
#include "typedefs.h"	// typedefs


struct ser_state_t
{
	uint16_t curr_clk;		//! ticks passed
	uint8_t out;			//! in / out value
	int8_t cur_bit;			//! the current bit
	bool enabled;			//! transfer active
	bool use_internal;		//! clock source
};


void serial_tick(emu_state *restrict, int);

#endif /*!__SERIO_H_*/
