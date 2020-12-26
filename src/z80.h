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

#if !defined(ZILOG_Z80_CPU_V0100_INCLUDED)
#define ZILOG_Z80_CPU_V0100_INCLUDED

#include "z80table.h"

//	Z80 flags
#define FLAG_S	0x80
#define FLAG_Z	0x40
#define FLAG_H	0x10
#define FLAG_PV 0x4
#define FLAG_N	0x2
#define FLAG_C	0x1

typedef struct tagINITCPU
{
	BYTE *pMem;
	BYTE *pInp;
	BYTE *pOutp;
	DWORD MMask;
} INITCPU;

// Macros for significant memory bounds.
#define ML_2K	0x07ff
#define ML_4K	0x0fff
#define ML_8K	0x1fff
#define ML_16K	0x3fff
#define ML_32K	0x7fff
#define ML_64K	0xffff

// Unified register access symbols.
// In the view of realization all register indirection, contant indirection and
// constant referencing must be considered as register.
enum
{ 
	RG_A = 1, RG_B, RG_C, RG_D, RG_E, RG_H, RG_L, RG_I, RG_R, RG_SP,
	RG__AF,	RG_AF, RG_BC, RG_DE, RG_HL, RG_IX, RG_IY, RG_XL, RG_XH, 
	RG_YL, RG_YH, RG_IXINDIRECT, RG_IYINDIRECT, RG_BCINDIRECT, 
	RG_DEINDIRECT, RG_HLINDIRECT, RG_BYTECONST, RG_WORDCONST,
	RG_WORDCONSTINDIRECT, RG_SPINDIRECT, 
	RG_CINDIRECT, RG_HLJUMP, RG_IXJUMP, RG_IYJUMP, RG_BYTECONSTIO
};

typedef struct tagREGISTERS
{
	BYTE C, B, E, D, L, H, F, A, I, R;
	WORD SP, PC;
	BYTE IX_L, IX_H, IY_L, IY_H;
	BYTE _C, _B, _E, _D, _L, _H, _F, _A;
} REGISTERS;

enum { RP_NOTHING, RP_RD, RP_WR };
#define BYTEOP	0
#define WORDOP	1

typedef struct tagREPORT
{
	int  mem_op,   io_op;
	WORD mem_add,  io_add;
	int  mem_mode;
} REPORT;

class CZ80
{
protected:
	REGISTERS	m_Regs;
	BYTE *		m_pMemoryBase;
	BYTE *		m_pPerIBase;
	BYTE *		m_pPerOBase;
	DWORD		m_MMask;
	REPORT		m_Report;
	BYTE		m_d;
	BYTE		m_prefix;
	BOOL		m_isEI;
	char		m_ParityMap[256];
	//DESCREC		m_TableED[256];
	DESCREC		m_TempIN;

protected:
	void *	GetRegAddress(int Reg);
	BYTE	Fetch();
	void	StoreB(BYTE *pdest, BYTE *psrc);
	void	StoreW(BYTE *pdest, WORD *psrc);
	BYTE	ReadIO(WORD addr);
	void	WriteIO(WORD addr, BYTE data);
	void	ModifyPC(BYTE value);
	void	SetFlag(BYTE flag, BOOL state);
	BOOL	GetFlag(BYTE flag);
	BOOL	CheckAddr(BYTE *paddr);
	BOOL	CreateReport(BYTE *dest, BYTE *src, int mode);
	BYTE	Add8(int type, BYTE a, BYTE n);
	void	SetZS(BYTE a);
	BOOL	GetJumpCondition(BYTE opcode);
	BOOL	DecodeCB(BYTE opcode, DESCREC *pdr);
	BOOL	DecodeED(BYTE opcode, DESCREC *pdr);
	int		GetRegBy(BYTE code);
	void	ExecuteInstruction(DESCREC *pdr);
	void	StackPC();
	void	ExecRun();
	void	CreateParityMap();
	//void	CreateTableED();

	void	DoLoader8(DESCREC *pdr);
	void	DoLoader16(DESCREC *pdr);
	void	DoGeneral(DESCREC *pdr);
	void	DoALU8(DESCREC *pdr);
	void	DoALU16(DESCREC *pdr);
	void	DoJump(DESCREC *pdr);
	void	DoCall(DESCREC *pdr);
	void	DoBit(DESCREC *pdr);
	void	DoRotShift(DESCREC *pdr);
	void	DoExBlock(DESCREC *pdr);
	void	DoIo(DESCREC *pdr);

public:
	WORD		m_HIBYTEIN;
	BOOL		m_IFF1, m_IFF2;
	int			m_IM;
	BOOL		m_Halt;

	CZ80();
	~CZ80();

	// Interface for the hosting environment
	void	Reset();			// Hardwareer reset
	void	Int(BYTE n = 0xff);	// Maskable interrupt request
	void	Nmi();				// Non-maskable interrupt request
	void	Run();				// Executing the next instruction in the queue

	// General purpose query functions
	REGISTERS * GetRegs()		{ return &m_Regs; }
	REPORT *	GetReport()		{ return &m_Report; }
	BOOL *		GetIFF1()		{ return &m_IFF1; }
	BOOL *		GetIFF2()		{ return &m_IFF2; }
	int  *		GetIM()			{ return &m_IM; }

	// Functions for initializing and binding the CPU instance to the hosting environment
	void	SetCPUProp(INITCPU &iCPU);
	void	GetCPUProp(INITCPU &iCPU);
};

#endif
