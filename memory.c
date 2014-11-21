#include "config.h"	// macros
#include <stdint.h>	// uint[XX]_t

#include "print.h"	// fatal
#include "memory.h"	// Constants and what have you */
#include "lcdc.h"	// lcdc_read
#include "ctl_unit.h"	// int_flag_*
#include "input.h"	// joypad_*
#include "rom_read.h"	// OFF_CART_TYPE
#include "serio.h"	// serial_*
#include "timer.h"	// timer_*


/***********************************************************************
 * readers
 ***********************************************************************/
typedef uint8_t (*mem_read_fn)(emu_state *restrict, uint16_t);

/*!
 * @brief hardware read catch-all for no hardware present at a port
 * @result emulation stopped because some doofus read from a non-existant
 * 	   device.
 */
static inline uint8_t no_hardware(emu_state *restrict state, uint16_t location)
{
	printf("%X\n", state->registers.pc);
	fatal("no device present at %04X (emulator bug?  incompatible GB?)",
	      location);
	/* NOTREACHED */
	return -1;
}

static inline uint8_t ugh_sound(emu_state *restrict state unused, uint16_t location unused)
{
	fatal("ask greaser or aji to write a sound module for this thing");
	return -1;
}

/*! a table of hardware register read methods */
static mem_read_fn hw_reg_read[0x80] =
{
	joypad_read, /* 00 - P1 - joypad */
	serial_read, /* 01 - SB - serial data */
	serial_read, /* 02 - SC - serial control */
	no_hardware, /* 03 - NO HARDWARE */
	timer_read,  /* 04 - DIV */
	timer_read,  /* 05 - TIMA - timer step */
	timer_read,  /* 06 - TMA - timer count */
	timer_read,  /* 07 - TAC - timer frequency / enable */

	/* 08..0E - NO HARDWARE */
	no_hardware, no_hardware, no_hardware, no_hardware,
	no_hardware, no_hardware, no_hardware,

	int_flag_read, /* 0F - IF - interrupt status.. also CPU-based */

	/* 10..14 - sound mode 1 */
	ugh_sound, ugh_sound, ugh_sound, ugh_sound, ugh_sound,

	no_hardware, /* 15 - NO HARDWARE */

	/* 16..19 - sound mode 2 */
	ugh_sound, ugh_sound, ugh_sound, ugh_sound,
	/* 1A..1E - sound mode 3 */
	ugh_sound, ugh_sound, ugh_sound, ugh_sound, ugh_sound,

	no_hardware, /* 1F - NO HARDWARE */

	/* 20..23 - sound mode 4 */
	ugh_sound, ugh_sound, ugh_sound, ugh_sound,
	/* 24..26 - sound control */
	ugh_sound, ugh_sound, ugh_sound,

	/* 27..2F - NO HARDWARE */
	no_hardware, no_hardware, no_hardware, no_hardware, no_hardware,
	no_hardware, no_hardware, no_hardware, no_hardware,

	/* 30..3F - WAV RAM */
	ugh_sound, ugh_sound, ugh_sound, ugh_sound, ugh_sound, ugh_sound,
	ugh_sound, ugh_sound, ugh_sound, ugh_sound, ugh_sound, ugh_sound,
	ugh_sound, ugh_sound, ugh_sound, ugh_sound,

	/* 40..45 - LCD controller */
	lcdc_read, lcdc_read, lcdc_read, lcdc_read, lcdc_read, lcdc_read,

	no_hardware, /* 46 - DMA - DMA transfer and control */

	/* 47..4B - more graphics stuff */
	lcdc_read, lcdc_read, lcdc_read, lcdc_read, lcdc_read,

	/* 4C..7F - NO HARDWARE */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x4F */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x53 */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x57 */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x5B */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x5F */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x63 */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x67 */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x6B */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x6F */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x73 */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x77 */
	no_hardware, no_hardware, no_hardware, no_hardware, /* 0x7B */
	no_hardware, no_hardware, no_hardware, no_hardware  /* 0x7F */
};

/*! directly read from a location in memory */
static inline uint8_t direct_read(emu_state *restrict state, uint16_t location)
{
	return state->memory[location];
}

/*! read from the switchable ROM bank space */
static inline uint8_t rom_bank_read(emu_state *restrict state unused, uint16_t location)
{
	uint32_t addr = (state->bank - 1) * 0x4000;
	return state->cart_data[addr + location];
}

static inline uint8_t not_impl(emu_state *restrict state unused, uint16_t location)
{
	fatal("reading from %04X is not yet implemented", location);
	/* NOTREACHED */
	return -1;
}

/*! read from the switchable RAM bank space */
static inline uint8_t ram_bank_read(emu_state *restrict state unused, uint16_t location unused)
{
	fatal("RAM bank switching not yet implemented");
	/* NOTREACHED */
	return -1;
}

/*!
 * @brief	Read a byte (8 bits) out of memory.
 * @param	state		The emulator state to use when reading.
 * @param	location	The location in memory to read.
 * @returns	The value of the location in memory.
 * @result	Emulation will terminate if the memory cannot be read.
 */
uint8_t mem_read8(emu_state *restrict state, uint16_t location)
{
	switch(location >> 12)
	{
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		// switchable bank - 0x4000..0x7FFF
		return rom_bank_read(state, location);
	case 0x8:
	case 0x9:
		// video memory - 0x8000..0x9FFF
		return not_impl(state, location);
	case 0xA:
	case 0xB:
		// switchable RAM bank - 0xA000-0xBFFF
		return ram_bank_read(state, location);
	case 0xE:
	case 0xF:
		switch(location >> 8)
		{
		case 0xFE:
			break;
		case 0xFF:
			if(likely(location < 0xFF80))
			{
				return hw_reg_read[location - 0xFF00](state, location);
			}

			break;
		default:
			// who knows? the SHADOW knows! - 0xE000..0xFDFF
			location -= 0x2000;
		}
	default:
		return direct_read(state, location);
	}
}

/*!
 * @brief	Read two bytes (16 bits) out of memory.
 * @param	state		The emulator state to use when reading.
 * @param	location	The location in memory to read.
 * @returns	The value of the two bytes at the location, LSB lower.
 * @result	Emulation will terminate if the memory cannot be read.
 */
uint16_t mem_read16(emu_state *restrict state, uint16_t location)
{
	return (mem_read8(state, location+1) << 8) | mem_read8(state, location);
}




/***********************************************************************
 * writers
 ***********************************************************************/
typedef void (*mem_write8_fn)(emu_state *restrict , uint16_t, uint8_t);

/*!
 * @brief silly name, decent intent
 * @result emulation terminates because some doofus wrote to a r/o reg
 */
static inline void readonly_reg_write(emu_state *restrict state unused, uint16_t location, uint8_t data)
{
	fatal("attempted write of %02X to read-only register %04X",
	      data, location);
}

/*!
 * @brief what happens when you poke around randomly
 * @result emulation terminates because some doofus wrote to a device not there
 */
static inline void doofus_write(emu_state *restrict state unused, uint16_t location, uint8_t data)
{
	fatal("attempted doofus write of %02X to non-existant device at %04X",
	      data, location);
}

static mem_write8_fn hw_reg_write[0x80] =
{
	joypad_write, /* 00 - P1 - joypad */
	serial_write, /* 01 - SB - serial data */
	serial_write, /* 02 - SC - serial control */
	doofus_write, /* 03 - NO HARDWARE */
	timer_write,  /* 04 - DIV - CPU register */
	timer_write,  /* 05 - TIMA - timer step */
	timer_write,  /* 06 - TMA - timer count */
	timer_write,  /* 07 - TAC - timer control */

	/* 08..0E - NO HARDWARE */
	doofus_write, doofus_write, doofus_write, doofus_write,
	doofus_write, doofus_write, doofus_write,

	int_flag_write, /* 0F - IF - interrupt control */

	/* 10..14 - sound bank 1 */
	timer_write, doofus_write, timer_write, doofus_write,
	timer_write,

	doofus_write, /* 15 - NO HARDWARE */

	/* 16..19 - sound bank 2 */
	doofus_write, timer_write, doofus_write, timer_write,

	/* 1A..1E - sound bank 3 */
	timer_write, doofus_write, timer_write, doofus_write,
	doofus_write,

	doofus_write, /* 1F - NO HARDWARE */

	/* 20..23 - sound bank 4 */
	doofus_write, timer_write, doofus_write, timer_write,

	/* 24..26 - sound control */
	timer_write, timer_write, timer_write,

	/* 27..2F - NO HARDWARE */
	doofus_write, doofus_write, doofus_write,
	doofus_write, doofus_write, doofus_write,
	doofus_write, doofus_write, doofus_write,

	/* 30..3F - WAV RAM */
	doofus_write, doofus_write, doofus_write, doofus_write,
	doofus_write, doofus_write, doofus_write, doofus_write,
	doofus_write, doofus_write, doofus_write, doofus_write,
	doofus_write, doofus_write, doofus_write, doofus_write,

	/* 40..45 - LCD controller */
	lcdc_write, lcdc_write, lcdc_write,
	lcdc_write, lcdc_write, lcdc_write,

	doofus_write, /* 46 - DMA */

	/* 47..4B - more video */
	lcdc_write, lcdc_write, lcdc_write, lcdc_write, lcdc_write,

	/* 4C..7F - NO HARDWARE */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x4F */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x53 */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x57 */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x5B */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x5F */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x63 */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x67 */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x6B */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x6F */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x73 */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x77 */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x7B */
	doofus_write, doofus_write, doofus_write, doofus_write, /* 0x7F */
};

/*!
 * @brief	Write a byte (8 bits) to memory.
 * @param	state		The emulator state to use while writing.
 * @param	location	The location in memory to write.
 * @param	data		The data to write.
 * @result	The data is written to the specified location.
 * 		Emulation will terminate if the memory cannot be written to.
 */
void mem_write8(emu_state *restrict state, uint16_t location, uint8_t data)
{
	switch(location >> 12)
	{
	case 0x2:
	case 0x3:
		switch(state->cart_data[OFF_CART_TYPE])
		{
		case CART_ROM_ONLY:
			fatal("invalid memory write at %04X (%02X)",
			      location, data);
		case CART_MBC3:
		case CART_MBC3_RAM:
		case CART_MBC3_RAM_BATT:
		case CART_MBC3_TIMER_BATT:
		case CART_MBC3_TIMER_RAM_BATT:
			state->bank = data & 0x7F;
			debug("switching to bank %04X", state->bank);
			return;
		default:
			fatal("banks for this cart aren't done yet sorry :(");
		}
	case 0xA:
	case 0xB:
		/* switched RAM bank */
		fatal("invalid memory write at %04X (%02X)", location, data);
		return;
	case 0xE:
	case 0xF:
		switch(location >> 8)
		{
		case 0xFE:
			break;
		case 0xFF:
			if(likely(location < 0xFF80))
			{
				hw_reg_write[location - 0xFF00](state, location, data);
				return;
			}

			break;
		default:
			// who knows? the SHADOW knows! - 0xE000..0xFDFF
			location -= 0x2000;
		}
	}

	state->memory[location] = data;
}

void mem_write16(emu_state *restrict state, uint16_t location, uint16_t data)
{
	/* TODO this needs to be filtered just like the readers */
	unsigned char *l = state->memory + location;
	uint16_t *hax = (uint16_t *)l;

	*hax = data;
	//fatal("invalid memory write at %04X (%04X)", location, data);
}
