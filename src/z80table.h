/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

/////////////////////////////////////////////////////////////////////////////
// THIS IMPLEMENTATION HAS BEEN REALIZED ONLY FOR PUBLIC Z80 INSTRUCTIONS! //
/////////////////////////////////////////////////////////////////////////////

#if !defined(ZILOG_Z80_CPU_TABLES_V0100_INCLUDED)
#define ZILOG_Z80_CPU_TABLES_V0100_INCLUDED

// Instruction group types
enum
{
	IG_LD8,
	IG_LD16,
	IG_ALU8,
	IG_ALU16,
	IG_EXBLOCK,
	IG_IO,
	IG_ROTSHIFT,
	IG_BIT,
	IG_JUMP,
	IG_CALL,
	IG_GEN,
	IG_INVALID
};

// Instruction mnemonic types
enum
{
	GP_ADC = 1, GP_ADD, GP_AND,
	GP_BIT,
	GP_CALL, GP_CCF, GP_CP, GP_CPD, GP_CPDR, GP_CPI, GP_CPIR, GP_CPL,
	GP_DAA, GP_DEC, GP_DI, GP_DJNZ,
	GP_EI, GP_EX, GP_EXX,
	GP_HALT,
	GP_IM, GP_IN, GP_INC, GP_IND, GP_INDR, GP_INI, GP_INIR,
	GP_JP, GP_JR,
	GP_LD, GP_LDD, GP_LDDR, GP_LDI, GP_LDIR,
	GP_NEG, GP_NOP,
	GP_OR, GP_OTDR, GP_OTIR, GP_OUT, GP_OUTD, GP_OUTI,
	GP_POP, GP_PUSH,
	GP_RES, GP_RET, GP_RETI, GP_RETN, GP_RL, GP_RLA, GP_RLC, GP_RLCA,
	GP_RLD, GP_RR, GP_RRA, GP_RRC, GP_RRCA, GP_RRD, GP_RST,
	GP_SBC, GP_SCF, GP_SET, GP_SLA, GP_SRA, GP_SRL, GP_SUB,
	GP_XOR
};

// One record from the instruction descriptor code table
typedef struct tagDESCREC {
	BYTE icode;
	int ig;
	int dest;
	int src;
	int ex;
} DESCREC;

int		GetRegBy(BYTE code);
void	CreateTableCB();
void	CreateTableED();

#endif