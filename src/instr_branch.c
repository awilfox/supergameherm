/*!
 * @brief JR n (0x18)
 * @result add n to pc
 */
static inline void jr_r8(emu_state *restrict state, uint8_t data[])
{
	int8_t to_add = data[0];

	REG_PC(state) += to_add;

	state->wait = 12;
}

/*!
 * @brief JR NZ,n (0x20)
 * @result add n to pc if Z (zero) flag clear
 */
static inline void jr_nz_r8(emu_state *restrict state, uint8_t data[])
{
	int8_t to_add = data[0];

	state->wait = 8;

	if(!IS_FLAG(state, FLAG_Z))
	{
		REG_PC(state) += to_add;

		state->wait += 4;
	}
}

/*!
 * @brief JR Z,n (0x28)
 * @result add n to pc if Z (zero) flag set
 */
static inline void jr_z_r8(emu_state *restrict state, uint8_t data[])
{
	int8_t to_add = data[0];

	state->wait = 8;

	if(IS_FLAG(state, FLAG_Z))
	{
		REG_PC(state) += to_add;

		state->wait += 4;
	}
}

/*!
 * @brief JR NC,n (0x30)
 * @result add n to pc if C (carry) flag clear
 */
static inline void jr_nc_r8(emu_state *restrict state, uint8_t data[])
{
	int8_t to_add = data[0];

	state->wait = 8;

	if(!IS_FLAG(state, FLAG_C))
	{
		REG_PC(state) += to_add;

		state->wait += 4;
	}
}

/*!
 * @brief JR C,n (0x38)
 * @result add n to pc if C (carry) flag set
 */
static inline void jr_c_r8(emu_state *restrict state, uint8_t data[])
{
	int8_t to_add = data[0];

	state->wait = 8;

	if(IS_FLAG(state, FLAG_C))
	{
		REG_PC(state) += to_add;

		state->wait += 4;
	}
}

/*!
 * @brief JP NZ,nn (0xC2)
 * @result pc is set to 16-bit immediate value (LSB, MSB) if Z flag is not set
 */
static inline void jp_nz_d16(emu_state *restrict state, uint8_t data[])
{
	state->wait = 12;

	if(!IS_FLAG(state, FLAG_Z))
	{
		REG_PC(state) = (data[1]<<8) | data[0];

		state->wait += 4;
	}
}

/*!
 * @brief JP nn (0xC3)
 * @result pc is set to 16-bit immediate value (LSB, MSB)
 */
static inline void jp_d16(emu_state *restrict state, uint8_t data[])
{
	REG_PC(state) = (data[1]<<8)|data[0];

	state->wait = 16;
}

/*!
 * @brief CALL nn (0xCD)
 * @result next pc stored in stack; jump to nn
 */
static inline void call_d16(emu_state *restrict state, uint8_t data[])
{
	uint16_t value = (data[1]<<8)|data[0];

	REG_SP(state) -= 2;
	mem_write16(state, REG_SP(state), REG_PC(state));

	REG_PC(state) = value;

	state->wait = 24;
}

/*!
 * @brief CALL NZ,nn (0xC4)
 * @result CALL nn if Z flag is not set
 */
static inline void call_nz_d16(emu_state *restrict state, uint8_t data[])
{
	if(IS_FLAG(state, FLAG_Z))
	{
		state->wait = 12;
	}
	else
	{
		call_d16(state, data);
	}
}

static inline void rst(emu_state *restrict state, uint8_t data[] UNUSED)
{
	uint16_t to = mem_read8(state, REG_PC(state) - 1) - 0xC7;

	REG_SP(state) -= 2;
	mem_write16(state, REG_SP(state), REG_PC(state));
	REG_PC(state) = to;

	state->wait = 16;
}

/*!
 * @brief RET (0xC9) - return from CALL
 * @result pop two bytes from the stack and jump to that location
 */
static inline void ret(emu_state *restrict state, uint8_t data[] UNUSED)
{
	REG_PC(state) = mem_read16(state, REG_SP(state));
	REG_SP(state) += 2;

	state->wait = 16;
}

/*!
 * @brief RETNZ (0xC0) - return from CALL if Z flag not set
 * @result RET, if Z flag not set, otherwise nothing.
 */
static inline void retnz(emu_state *restrict state, uint8_t data[] UNUSED)
{
	if(IS_FLAG(state, FLAG_Z))
	{
		state->wait = 8;
	}
	else
	{
		ret(state, NULL);

		// ret already adds 16
		state->wait += 4;
	}
}

/*!
 * @brief RETZ (0xC8) - return from CALL if Z flag set
 * @result RET, if Z flag is set, otherwise nothing.
 */
static inline void retz(emu_state *restrict state, uint8_t data[] UNUSED)
{
	if(IS_FLAG(state, FLAG_Z))
	{
		ret(state, NULL);

		// ret already adds 16
		state->wait += 4;
	}
	else
	{
		state->wait = 8;
	}
}

/*!
 * @brief JP Z,nn (0xCA)
 * @result pc is set to 16-bit immediate value (LSB, MSB) if Z flag is set
 */
static inline void jp_z_d16(emu_state *restrict state, uint8_t data[])
{
	state->wait = 12;

	if(IS_FLAG(state, FLAG_Z))
	{
		REG_PC(state) = (data[1]<<8)|data[0];

		state->wait += 4;
	}
}

/*!
 * @brief CALL Z,nn (0xCC)
 * @result CALL nn if Z flag is set
 */
static inline void call_z_d16(emu_state *restrict state, uint8_t data[])
{
	if(IS_FLAG(state, FLAG_Z))
	{
		call_d16(state, data);
	}
	else
	{
		state->wait = 12;
	}
}

/*!
 * @brief RETNC (0xD0) - return from CALL if C flag not set
 * @result RET, if C flag not set, otherwise nothing.
 */
static inline void retnc(emu_state *restrict state, uint8_t data[] UNUSED)
{
	if(IS_FLAG(state, FLAG_C))
	{
		state->wait = 8;
	}
	else
	{
		ret(state, NULL);

		// ret already adds 16
		state->wait += 4;
	}
}

/*!
 * @brief JP NC,nn (0xD2)
 * @result pc is set to 16-bit immediate value (LSB, MSB) if C flag is not set
 */
static inline void jp_nc_d16(emu_state *restrict state, uint8_t data[])
{
	state->wait = 12;

	if(!IS_FLAG(state, FLAG_C))
	{
		REG_PC(state) = (data[1]<<8)|data[0];

		state->wait += 4;
	}
}

/*!
 * @brief CALL NC,nn (0xD4)
 * @result CALL nn if C flag is not set
 */
static inline void call_nc_d16(emu_state *restrict state, uint8_t data[])
{
	if(IS_FLAG(state, FLAG_C))
	{
		state->wait = 12;
	}
	else
	{
		call_d16(state, data);
	}
}

/*!
 * @brief RETC (0xD8) - return from CALL if C flag set
 * @result RET, if C flag is set, otherwise nothing.
 */
static inline void retc(emu_state *restrict state, uint8_t data[] UNUSED)
{
	if(IS_FLAG(state, FLAG_C))
	{
		ret(state, NULL);

		// ret already adds 16
		state->wait += 4;
	}
	else
	{
		state->wait = 8;
	}
}

/*!
 * @brief RETI (0xD9) - return from CALL and enable interrupts
 * @result RET + EI
 */
static inline void reti(emu_state *restrict state, uint8_t data[] UNUSED)
{
	// Immediate on halt
	if(mem_read8(state, REG_PC(state)) == 0x76)
	{
		state->interrupts.enable_ctr = 1;
	}
	else
	{
		state->interrupts.enable_ctr = 2;
	}

	ret(state, NULL);
}

/*!
 * @brief JP C,nn (0xDA)
 * @result pc is set to 16-bit immediate value (LSB, MSB) if C flag is set
 */
static inline void jp_c_d16(emu_state *restrict state, uint8_t data[])
{
	state->wait = 12;

	if(IS_FLAG(state, FLAG_C))
	{
		REG_PC(state) = (data[1]<<8)|data[0];

		state->wait += 4;
	}
}

/*!
 * @brief CALL C,nn (0xDC)
 * @result CALL nn if C flag is set
 */
static inline void call_c_d16(emu_state *restrict state, uint8_t data[])
{
	if(IS_FLAG(state, FLAG_C))
	{
		call_d16(state, data);
	}
	else
	{
		state->wait = 12;
	}
}

/*!
 * @brief JP HL (0xE9)
 * @result pc = HL
 */
static inline void jp_hl(emu_state *restrict state, uint8_t data[] UNUSED)
{
	REG_PC(state) = REG_HL(state);

	state->wait = 4;
}
