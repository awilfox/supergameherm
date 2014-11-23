#include "config.h"	// macros

#include "timer.h"	// get_time
#include "sgherm.h"	// emu_state, cpu_freq
#include "print.h"	// debug
#include "ctl_unit.h"	// flags
#include "debug.h"	// Prototypes etc
#include "memory.h"	// mem_read8

static const char * const mnemonics[0x100] =
{
	"NOP", "LD BC,d16", "LD (BC),A", "INC BC",		// 0x00
	"INC B", "DEC B", "LD B,d8", "RLCA",			// 0x04
	"LD (a16),SP", "ADD HL,BC", "LD A,(BC)", "DEC BC",	// 0x08
	"INC C", "DEC C", "LD C,d8", "RRCA",			// 0x0C
	"STOP 0", "LD DE,d16", "LD (DE),A", "INC DE",		// 0x10
	"INC D", "DEC D", "LD D,d8", "RLA",			// 0x14
	"JR r8", "ADD HL,DE", "LD A,(DE)", "DEC DE",		// 0x18
	"INC E", "DEC E", "LD E,d8", "RRA",			// 0x1C
	"JR NZ,r8", "LD HL,d16", "LD (HL+),A", "INC HL",	// 0x20
	"INC H", "DEC H", "LD H,d8", "DAA",			// 0x24
	"JR Z,r8", "ADD HL,HL", "LD A,(HL+)", "DEC HL",		// 0x28
	"INC L", "DEC L", "LD L,d8", "CPL",			// 0x2C
	"JR NC,r8", "LD SP,d16", "LD (HL-),A", "INC SP",	// 0x30
	"INC (HL)", "DEC (HL)", "LD (HL),d8", "SCF",		// 0x34
	"JR C,r8", "ADD HL,SP", "LD A,(HL-)", "DEC SP",		// 0x38
	"INC A", "DEC A", "LD A,d8", "CCF",			// 0x3C
	"LD B,B", "LD B,C", "LD B,D", "LD B,E",			// 0x40
	"LD B,H", "LD B,L", "LD B,(HL)", "LD B,A",		// 0x44
	"LD C,B", "LD C,C", "LD C,D", "LD C,E",			// 0x48
	"LD C,H", "LD C,L", "LD C,(HL)", "LD C,A",		// 0x4C
	"LD D,B", "LD D,C", "LD D,D", "LD D,E",			// 0x50
	"LD D,H", "LD D,L", "LD D,(HL)", "LD D,A",		// 0x54
	"LD E,B", "LD E,C", "LD E,D", "LD E,E",			// 0x58
	"LD E,H", "LD E,L", "LD E,(HL)", "LD E,A",		// 0x5C
	"LD H,B", "LD H,C", "LD H,D", "LD H,E",			// 0x60
	"LD H,H", "LD H,L", "LD H,(HL)", "LD H,A",		// 0x64
	"LD L,B", "LD L,C", "LD L,D", "LD L,E",			// 0x68
	"LD L,H", "LD L,L", "LD L,(HL)", "LD L,A",		// 0x6C
	"LD (HL),B", "LD (HL),C", "LD (HL),D", "LD (HL),E",	// 0x70
	"LD (HL),H", "LD (HL),L", "HALT", "LD (HL),A",		// 0x74
	"LD A,B", "LD A,C", "LD A,D", "LD A,E",			// 0x78
	"LD A,H", "LD A,L", "LD A,(HL)", "LD A,A",		// 0x7C
	"ADD A,B", "ADD A,C", "ADD A,D", "ADD A,E",		// 0x80
	"ADD A,H", "ADD A,L", "ADD A,(HL)", "ADD A,A",		// 0x84
	"ADC A,B", "ADC A,C", "ADC A,D", "ADC A,E",		// 0x88
	"ADC A,H", "ADC A,L", "ADC A,(HL)", "ADC A,A",		// 0x8C
	"SUB B", "SUB C", "SUB D", "SUB E",			// 0x90
	"SUB H", "SUB L", "SUB (HL)", "SUB A",			// 0x94
	"SBC A,B", "SBC A,C", "SBC A,D", "SBC A,E",		// 0x98
	"SBC A,H", "SBC A,L", "SBC A,(HL)", "SBC A,A",		// 0x9C
	"AND B", "AND C", "AND D", "AND E",			// 0xA0
	"AND H", "AND L", "AND (HL)", "AND A",			// 0xA4
	"XOR B", "XOR C", "XOR D", "XOR E",			// 0xA8
	"XOR H", "XOR L", "XOR (HL)", "XOR A",			// 0xAC
	"OR B", "OR C", "OR D", "OR E",				// 0xB0
	"OR H", "OR L", "OR (HL)", "OR A",			// 0xB4
	"CP B", "CP C", "CP D", "CP E",				// 0xB8
	"CP H", "CP L", "CP (HL)", "CP A",			// 0xBC
	"RET NZ", "POP BC", "JP NZ,a16", "JP a16",		// 0xC0
	"CALL NZ,a16", "PUSH BC", "ADD A,d8", "RST 00H",	// 0xC4
	"RET Z", "RET", "JP Z,a16", "PREFIX CB",		// 0xC8
	"CALL Z,a16", "CALL a16", "ADC A,d8", "RST 08H",	// 0xCC
	"RET NC", "POP DE", "JP NC,a16", "",			// 0xD0
	"CALL NC,a16", "PUSH DE", "SUB d8", "RST 10H",		// 0xD4
	"RET C", "RETI", "JP C,a16", "",			// 0xD8
	"CALL C,a16", "", "SBC A,d8", "RST 18H",		// 0xDC
	"LDH (a8),A", "POP HL", "LD (C),A", "",			// 0xE0
	"", "PUSH HL", "AND d8", "RST 20H",			// 0xE4
	"ADD SP,r8", "JP (HL)", "LD (a16),A", "",		// 0xE8
	"", "", "XOR d8", "RST 28H",				// 0xEC
	"LDH A,(a8)", "POP AF", "LD A,(C)", "DI",		// 0xF0
	"", "PUSH AF", "OR d8", "RST 30H",			// 0xF4
	"LD HL,SP+r8", "LD SP,HL", "LD A,(a16)", "EI",		// 0xF8
	"", "", "CP d8", "RST 38H",				// 0xFC
};

const char * lookup_mnemonic(uint8_t opcode)
{
	return mnemonics[opcode];
}

void print_cpu_state(emu_state *restrict state)
{
	debug("[%X] (af bc de hl sp %X %X %X %X %X)", REG_PC(state),
		REG_AF(state), REG_BC(state), REG_DE(state),
		REG_HL(state), REG_SP(state));
}

void print_cycles(emu_state *restrict state)
{
	uint64_t finish = get_time();
	double taken = (finish - state->start_time) / 1e9;
	double cps = state->cycles / taken;
	const cpu_freq freq_gb = CPU_FREQ_GB, freq_gbc = CPU_FREQ_GBC;

	info("Time taken: %.3f seconds", taken);
	info("Cycle count: %ld", state->cycles);
	info("Cycles per second: %.3f (%.3fx GB, %.3fx GBC)", cps,
			cps / freq_gb, cps / freq_gbc);
}

void print_flags(emu_state *restrict state)
{
	debug("flags = %s%s%s%s",
		(REG_F(state) & FLAG_Z) ? "Z":"z",
		(REG_F(state) & FLAG_N) ? "N":"n",
		(REG_F(state) & FLAG_H) ? "H":"h",
		(REG_F(state) & FLAG_C) ? "C":"c");
}

void dump_all_state(emu_state *restrict state)
{
	debug("pc=%04X\tsp=%04X\tbk=%04X\n",
		   REG_PC(state),
	REG_SP(state),
	state->bank);
	debug("af=%04X\tbc=%04X\tde=%04X\thl=%04X\n",
		   REG_AF(state),
	REG_BC(state),
	REG_DE(state),
	REG_HL(state));
	debug("interrupts are %s\n", (state->registers.interrupts ? "ENABLED" : "DISABLED"));
	print_flags(state);
	debug("bytes at pc: %02X %02X %02X %02X\n",
		mem_read8(state, REG_PC(state)),
		mem_read8(state, REG_PC(state)+1),
		mem_read8(state, REG_PC(state)+2),
		mem_read8(state, REG_PC(state)+3));
	debug("bytes at sp: %02X %02X %02X %02X\n",
		mem_read8(state, REG_SP(state)),
		mem_read8(state, REG_SP(state)+1),
		mem_read8(state, REG_SP(state)+2),
		mem_read8(state, REG_SP(state)+3));
}
