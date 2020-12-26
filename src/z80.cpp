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

#include "stdafx.h"
#include "z80.h"

extern DESCREC Z80cpu_t1[];
extern DESCREC Z80cpu_t2[];
extern DESCREC z80cpu_tCB[];
extern DESCREC z80cpu_tED[];

// DWORD mask array for describing individual memory segments (StoreB, StoreW)
DWORD dw_mask[32] =
{
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000
};

BYTE b_mask[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

CZ80::CZ80()
{
	m_pMemoryBase = NULL;
	m_pPerIBase = NULL;
	m_pPerOBase = NULL;
	m_MMask = 0xffffffff;
	CreateParityMap();
	CreateTableCB();
	CreateTableED();
	Reset();
}

CZ80::~CZ80()
{
}

void CZ80::Reset()
{
	m_Regs.A = m_Regs.F = m_Regs.B = m_Regs.C = m_Regs.D = m_Regs.E =
		m_Regs.H = m_Regs.L = m_Regs.I = m_Regs.R =
		m_Regs.IX_H = m_Regs.IX_L = m_Regs.IY_H = m_Regs.IY_L =
		m_Regs._A = m_Regs._F = m_Regs._B = m_Regs._C =
		m_Regs._D = m_Regs._E = m_Regs._H = m_Regs._L = 0;
	m_Regs.SP = m_Regs.PC = 0;
	m_Halt = FALSE;
	m_IM = 0;
	m_IFF1 = m_IFF2 = FALSE;
	m_prefix = 0;
	m_HIBYTEIN = 0x8000;

	// Set report to defaults
	m_Report.mem_op = m_Report.io_op = RP_NOTHING;
	m_Report.mem_add = m_Report.io_add = 0;
}

void CZ80::Int(BYTE n)
{
	if (m_IFF1) {
		if (m_isEI) Run();
		m_IFF1 = m_IFF2 = m_Halt = FALSE;
		switch (m_IM) {
		case 0:
		{
			DESCREC Rec;
			Rec.icode = Z80cpu_t1[n].icode;
			Rec.ig = Z80cpu_t1[n].ig;
			Rec.dest = Z80cpu_t1[n].dest;
			Rec.src = Z80cpu_t1[n].src;
			Rec.ex = Z80cpu_t1[n].ex;
			ExecuteInstruction(&Rec);
			break;
		}
		default:
			StackPC();
			if (m_IM == 1)
				m_Regs.PC = 0x0038;
			else
				m_Regs.PC = //256*m_Regs.I + (n & 0xfe);
				*((WORD*)(m_pMemoryBase + (256 * m_Regs.I + (n & 0xff))));
			break;
		}
	}
}

void CZ80::Nmi()
{
	m_IFF2 = m_IFF1;
	m_IFF1 = FALSE;
	m_Halt = FALSE;
	StackPC();
	m_Regs.PC = 0x0066;
}

void CZ80::Run()
{
	if (m_HIBYTEIN != 0x8000) DoIo(&m_TempIN);
	m_Regs.R++;
	if (m_Halt) return;
	m_Report.mem_op = m_Report.io_op = RP_NOTHING;
	m_isEI = FALSE;
	do
	{
		ExecRun();
	}
	while (m_prefix == 0xdd || m_prefix == 0xfd);
}

void CZ80::ExecRun()
{
	DESCREC Rec;
	BOOL IsD = FALSE;
	BYTE table;

	// Fetch the next instruction
	BYTE icode = Fetch();

	// Classifying the instruction (prefix: CB,ED,DD,FD or code)
	switch (icode) {
	case 0xcb:
		if (m_prefix == 0xdd || m_prefix == 0xfd) {
			m_d = Fetch();
			IsD = TRUE;
		}
		icode = Fetch();
		Rec.dest = z80cpu_tCB[icode].dest;
		Rec.ex = z80cpu_tCB[icode].ex;
		Rec.src = z80cpu_tCB[icode].src;
		Rec.icode = z80cpu_tCB[icode].icode;
		Rec.ig = z80cpu_tCB[icode].ig;
		//if (DecodeCB(icode, &Rec)) return;
		table = 0xcb;
		break;
	case 0xed:
		icode = Fetch();
		Rec.dest = z80cpu_tED[icode].dest;
		Rec.ex = z80cpu_tED[icode].ex;
		Rec.src = z80cpu_tED[icode].src;
		Rec.icode = z80cpu_tED[icode].icode;
		Rec.ig = z80cpu_tED[icode].ig;
		//if (DecodeED(icode, &Rec)) return;
		table = 0xed;
		break;
	case 0xdd:
	case 0xfd:
		m_prefix = icode;
		return;
	default:
		Rec.icode = Z80cpu_t1[icode].icode;
		Rec.ig = Z80cpu_t1[icode].ig;
		Rec.dest = Z80cpu_t1[icode].dest;
		Rec.src = Z80cpu_t1[icode].src;
		Rec.ex = Z80cpu_t1[icode].ex;
		table = 0;
	}

	// Modification of the instruction descriptor in case of DD, FD prefix
	if (m_prefix == 0xdd || m_prefix == 0xfd) {
		BOOL isHLi;
		if (Rec.dest == RG_HLINDIRECT || Rec.src == RG_HLINDIRECT)
			isHLi = FALSE;
		else isHLi = TRUE;

		int xyl, xyh, xyi, xyii, xyjump;
		if (m_prefix == 0xdd)
		{
			xyl = RG_XL;
			xyh = RG_XH;
			xyjump = xyi = RG_IX;
			xyii = RG_IXINDIRECT;
		}
		else
		{
			xyl = RG_YL;
			xyh = RG_YH;
			xyjump = xyi = RG_IY;
			xyii = RG_IYINDIRECT;
		}

		if (table == 0xcb)
		{
			if (Rec.dest == RG_HLINDIRECT)
				Rec.dest = xyii;
			//if (Rec.src == RG_HLINDIRECT) Rec.src = xyii;
		}
		else if (table == 0xed)
		{
			// todo...
		}
		else
		{
			if (Rec.icode != 0xeb)
			{ // EX DE,IX v. EX DE,IY doesn't exist
				switch (Rec.dest)
				{
				case RG_L: if (isHLi) Rec.dest = xyl; break;
				case RG_H: if (isHLi) Rec.dest = xyh; break;
				case RG_HLJUMP: Rec.dest = xyjump; break;
				case RG_HL: Rec.dest = xyi; break;
				case RG_HLINDIRECT:
					Rec.dest = xyii;
					if (!IsD) m_d = Fetch();
				}
				switch (Rec.src)
				{
				case RG_L: if (isHLi) Rec.src = xyl; break;
				case RG_H: if (isHLi) Rec.src = xyh; break;
				case RG_HLJUMP: Rec.src = xyjump; break;
				case RG_HL: Rec.src = xyi; break;
				case RG_HLINDIRECT:
					Rec.src = xyii;
					if (!IsD) m_d = Fetch();
				}
			}
		}
	}

	ExecuteInstruction(&Rec);
	m_prefix = 0;//icode;
}

void CZ80::ExecuteInstruction(DESCREC* pdr)
{
	switch (pdr->ig)
	{
	case IG_LD8:	  DoLoader8(pdr);	break;
	case IG_LD16:	  DoLoader16(pdr);	break;
	case IG_GEN:	  DoGeneral(pdr);	break;
	case IG_ALU8:	  DoALU8(pdr);		break;
	case IG_ALU16:	  DoALU16(pdr);		break;
	case IG_JUMP:	  DoJump(pdr);		break;
	case IG_CALL:	  DoCall(pdr);		break;
	case IG_BIT:	  DoBit(pdr);		break;
	case IG_ROTSHIFT: DoRotShift(pdr);	break;
	case IG_EXBLOCK:  DoExBlock(pdr);	break;
	case IG_IO:		  DoIo(pdr);
	}
}

//////////////////////////////////////////////////////////////////
//	Egy adott flag állapotának beállítása

void CZ80::SetFlag(BYTE flag, BOOL state)
{
	if (state)
		m_Regs.F |= flag;
	else
		m_Regs.F &= ~flag;
}

//////////////////////////////////////////////////////////////////
//	Adott flag állapotának lekérdezése.

BOOL CZ80::GetFlag(BYTE flag)
{
	return (m_Regs.F & flag) ? TRUE : FALSE;
}

//////////////////////////////////////////////////////////////////
//	Egy kódolt regiszter típus nélküli címének lekérdezése.

void* CZ80::GetRegAddress(int Reg)
{
	switch (Reg)
	{
	case RG__AF:	return &m_Regs._F;
	case RG_AF:		return &m_Regs.F;
	case RG_A:		return &m_Regs.A;
	case RG_B:		return &m_Regs.B;
	case RG_BC:
	case RG_CINDIRECT:
	case RG_C:		return &m_Regs.C;
	case RG_D:		return &m_Regs.D;
	case RG_DE:
	case RG_E:		return &m_Regs.E;
	case RG_H:		return &m_Regs.H;
	case RG_HLJUMP:
	case RG_HL:
	case RG_L:		return &m_Regs.L;
	case RG_I:		return &m_Regs.I;
	case RG_R:		return &m_Regs.R;
	case RG_SP:		return &m_Regs.SP;
	case RG_IXJUMP:
	case RG_IX:
	case RG_XL:		return &m_Regs.IX_L;
	case RG_XH:		return &m_Regs.IX_H;
	case RG_IYJUMP:
	case RG_IY:
	case RG_YL:		return &m_Regs.IY_L;
	case RG_YH:		return &m_Regs.IY_H;
	case RG_IXINDIRECT:
	{
		WORD base = 256 * m_Regs.IX_H + m_Regs.IX_L;
		base += WORD(short(char(m_d)));
		return m_pMemoryBase + base;
	}
	case RG_IYINDIRECT:
	{
		WORD base = 256 * m_Regs.IY_H + m_Regs.IY_L;
		base += WORD(short(char(m_d)));
		return m_pMemoryBase + base;
	}
	case RG_BCINDIRECT:
		return m_pMemoryBase + (256 * m_Regs.B + m_Regs.C);
	case RG_DEINDIRECT:
		return m_pMemoryBase + (256 * m_Regs.D + m_Regs.E);
	case RG_HLINDIRECT:
		return m_pMemoryBase + (256 * m_Regs.H + m_Regs.L);
	case RG_SPINDIRECT:
		return m_pMemoryBase + m_Regs.SP;
	case RG_BYTECONSTIO:
	case RG_BYTECONST:
	{
		void* ptr = m_pMemoryBase + m_Regs.PC;
		ModifyPC(1);
		return ptr;
	}
	case RG_WORDCONST:
	{
		void* ptr = m_pMemoryBase + m_Regs.PC;
		ModifyPC(2);
		return ptr;
	}
	case RG_WORDCONSTINDIRECT:
	{
		WORD offs = *((WORD*)(m_pMemoryBase + m_Regs.PC));
		ModifyPC(2);
		return m_pMemoryBase + offs;
	}
	}
	return 0;
}

BYTE CZ80::Fetch()
{
	BYTE n = *(m_pMemoryBase + m_Regs.PC);
	ModifyPC(1);
	return n;
}

void CZ80::StoreB(BYTE* pdest, BYTE* psrc)
{
	if (CreateReport(pdest, psrc, BYTEOP))
	{
		WORD Addr = WORD(pdest - m_pMemoryBase);
		if (!(m_MMask & dw_mask[Addr / 2048]))
			return;
	}
	*pdest = *psrc;
}

void CZ80::StoreW(BYTE* pdest, WORD* psrc)
{
	if (CreateReport(pdest, (BYTE*)psrc, WORDOP))
	{
		WORD Addr = WORD(pdest - m_pMemoryBase);
		if (!(m_MMask & dw_mask[Addr / 2048]))
			return;
	}
	*((WORD*)pdest) = *psrc;
}


// Adds the specified singned offset to the PC register.
// The offset can be -128 - 127.
void CZ80::ModifyPC(BYTE value)
{
	WORD m = WORD(int(char(value)));
	m_Regs.PC += m;
}

// Determines whether the specfied address is a mamory or a register address.
BOOL CZ80::CheckAddr(BYTE* paddr)
{
	if (paddr >= m_pMemoryBase && paddr <= m_pMemoryBase + 0xffff)
		return TRUE;
	return FALSE;
}

BOOL CZ80::CreateReport(BYTE* dest, BYTE* src, int mode)
{
	m_Report.mem_mode = mode;
	if (CheckAddr(dest))
	{
		m_Report.mem_op = RP_WR;
		m_Report.mem_add = dest - m_pMemoryBase;
		return TRUE;
	}
	else if (CheckAddr(src))
	{
		m_Report.mem_op = RP_RD;
		m_Report.mem_add = src - m_pMemoryBase;
	}
	return FALSE;
}

// 8-bit loader instruction (LD)
void CZ80::DoLoader8(DESCREC* pdr)
{
	BYTE* dest = (BYTE*)GetRegAddress(pdr->dest); // [------]
	BYTE* src = (BYTE*)GetRegAddress(pdr->src);
	StoreB(dest, src);
	if (pdr->src == RG_I || pdr->src == RG_R)
	{	// [-***00]
		SetZS(m_Regs.A);
		SetFlag(FLAG_N, FALSE);
		SetFlag(FLAG_H, FALSE);
		SetFlag(FLAG_PV, m_IFF2);
	}
}

// 16-bit loader instructions (LD, PUSH, POP)
void CZ80::DoLoader16(DESCREC* pdr)	// [------]
{
	WORD* dest = (WORD*)GetRegAddress(pdr->dest);
	switch (pdr->ex)
	{
	case GP_PUSH:
	{
		m_Regs.SP -= 2;
		BYTE* spdest = (BYTE*)GetRegAddress(RG_SPINDIRECT);
		StoreW(spdest, dest);
		break;
	}
	case GP_POP:
	{
		WORD* spsrc = (WORD*)GetRegAddress(RG_SPINDIRECT);
		StoreW((BYTE*)dest, spsrc);
		m_Regs.SP += 2;
		break;
	}
	default:
	{
		WORD* src = (WORD*)GetRegAddress(pdr->src);
		StoreW((BYTE*)dest, src);
	}
	}
}

// General purpose instructions (DAA,CPL,NEG,CCF,SCF,NOP,HALT,DI,EI,IM0..2)
void CZ80::DoGeneral(DESCREC* pdr)
{
	switch (pdr->ex) {
	case GP_DI: m_IFF1 = m_IFF2 = FALSE; break;					// [------]
	case GP_EI: m_isEI = m_IFF1 = m_IFF2 = TRUE; break;			// [------]
	case GP_HALT: m_Halt = TRUE; break;							// [------]
	case GP_NOP: break;		// [------]	
	case GP_SCF:			// [1---00]
		SetFlag(FLAG_C, TRUE);
		SetFlag(FLAG_N, FALSE);
		SetFlag(FLAG_H, FALSE);
		break;
	case GP_CCF:			// [*---0?]
		GetFlag(FLAG_C) ? SetFlag(FLAG_C, FALSE) : SetFlag(FLAG_C, TRUE);
		SetFlag(FLAG_N, FALSE);
		break;
	case GP_CPL:			// [----11]
		m_Regs.A = ~m_Regs.A;
		SetFlag(FLAG_N, TRUE);
		SetFlag(FLAG_H, TRUE);
		break;
	case GP_DAA:			// [**P*-*]
	{
		BYTE Ah = m_Regs.A & 0x0f;
		if (GetFlag(FLAG_H) || Ah > 9)
			m_Regs.A = Add8(GetFlag(FLAG_N) ? GP_SUB : GP_ADD,
				m_Regs.A, 6);
		Ah = (m_Regs.A & 0xf0) >> 4;
		if (GetFlag(FLAG_C) || Ah > 9)
			m_Regs.A = Add8(GetFlag(FLAG_N) ? GP_SUB : GP_ADD,
				m_Regs.A, 0x60);
		SetZS(m_Regs.A);
		SetFlag(FLAG_PV, m_ParityMap[m_Regs.A] ? TRUE : FALSE);
		break;
	}
	case GP_NEG: m_Regs.A = Add8(GP_SUB, 0, m_Regs.A); break;	// [**V*1*]
	case GP_IM:													// [------]
		switch (pdr->icode) {
		case 0x46: m_IM = 0; break;
		case 0x56: m_IM = 1; break;
		default:   m_IM = 2;
		}
	}
}

// 8-bit add and substruct instructions (protected). Sets the following
// bits: V,N,H,C,Z,S
// ALU8, ALU16, NEG	[******]
BYTE CZ80::Add8(int type, BYTE a, BYTE n)
{
	WORD temp = WORD(a);
	BYTE ht1 = a & 0xf;
	BYTE ht2 = n & 0xf;
	short vt1 = short(char(a));
	short vt2 = short(char(n));
	char cf;

	if (type == GP_ADC || type == GP_SBC)
		cf = GetFlag(FLAG_C) ? 1 : 0;
	else cf = 0;

	if (type == GP_ADD || type == GP_ADC)
	{
		SetFlag(FLAG_N, FALSE);
		temp = temp + n + cf;
		ht1 = ht1 + ht2 + cf;
		vt1 = vt1 + vt2 + cf;
	}
	else
	{
		SetFlag(FLAG_N, TRUE);
		temp = temp - n - cf;
		ht1 = ht1 - ht2 - cf;
		vt1 = vt1 - vt2 - cf;
	}

	SetFlag(FLAG_C, (temp & 0x100) ? TRUE : FALSE);
	SetFlag(FLAG_H, (ht1 & 0x10) ? TRUE : FALSE);
	if (vt1 < -128 || vt1 > 127)
		SetFlag(FLAG_PV, TRUE);
	else SetFlag(FLAG_PV, FALSE);

	BYTE result = BYTE(temp & 0xff);
	SetZS(result);
	return result;
}

// Checks the specified byte and sets the Z and S flags.
// ALU8
void CZ80::SetZS(BYTE a)
{
	SetFlag(FLAG_Z, (a == 0) ? TRUE : FALSE);
	SetFlag(FLAG_S, (a & 0x80) ? TRUE : FALSE);
}

// Bit triple examination extracted from operand for JP cc and CALL cc
// conditional jump instructions.
BOOL CZ80::GetJumpCondition(BYTE opcode)
{
	int condition = (opcode & 0x38) >> 3;
	switch (condition) {
	case 0:	return !GetFlag(FLAG_Z);	// NZ
	case 1:	return GetFlag(FLAG_Z);		// Z
	case 2:	return !GetFlag(FLAG_C);	// NC
	case 3:	return GetFlag(FLAG_C);		// C
	case 4:	return !GetFlag(FLAG_PV);	// PO
	case 5:	return GetFlag(FLAG_PV);	// PE
	case 6:	return !GetFlag(FLAG_S);	// P
	}
	return GetFlag(FLAG_S);				// M
}

// 8-bit ALU implementation for instructions:
// ADD,ADC,SUB,SBC,AND,OR,XOR,CP,INC,DEC
void CZ80::DoALU8(DESCREC* pdr)
{
	BYTE* src;

	switch (pdr->ex) {
	case GP_ADD:	// [**V*0*]
	case GP_ADC:	// [**V*0*]
	case GP_SUB:	// [**V*1*]
	case GP_SBC:	// [**V*1*]
		src = (BYTE*)GetRegAddress(pdr->src);
		m_Regs.A = Add8(pdr->ex, m_Regs.A, *src);
		break;
	case GP_AND:	// [0*P*01]
	case GP_OR:		// [0*P*00]
	case GP_XOR:	// [0*P*00]
	{
		src = (BYTE*)GetRegAddress(pdr->src);
		if (pdr->ex == GP_AND)
			m_Regs.A &= *src;
		else if (pdr->ex == GP_OR)
			m_Regs.A |= *src;
		else
			m_Regs.A ^= *src;
		SetFlag(FLAG_N, FALSE);
		SetFlag(FLAG_PV, m_ParityMap[m_Regs.A] ? TRUE : FALSE);
		SetFlag(FLAG_H, (pdr->ex == GP_AND) ? TRUE : FALSE);
		SetFlag(FLAG_C, FALSE);
		SetZS(m_Regs.A);
		break;
	}
	case GP_CP:		// [**V*1*]
		src = (BYTE*)GetRegAddress(pdr->src);
		Add8(GP_SUB, m_Regs.A, *src);
		break;
	case GP_INC:	// [-*V*0*]
	case GP_DEC:	// [-*V*1*]
	{
		src = (BYTE*)GetRegAddress(pdr->dest);
		BOOL cf = GetFlag(FLAG_C);
		int op;
		if (pdr->ex == GP_INC)
			op = GP_ADD;
		else
			op = GP_SUB;
		BYTE temp = Add8(op, *src, 1);
		StoreB(src, &temp);
		SetFlag(FLAG_C, cf);
		break;
	}
	}
}

// 16-bit ALU implementation for instructions:
// ADD,ADC,SBC,INC,DEC
void CZ80::DoALU16(DESCREC* pdr)
{
	BYTE* dest = (BYTE*)GetRegAddress(pdr->dest);
	BYTE* src = (BYTE*)GetRegAddress(pdr->src);
	switch (pdr->ex) {
	case GP_ADD:	// [*---0?]
	{
		BYTE temp = m_Regs.F & 0xc4;
		BYTE low = Add8(GP_ADD, *dest, *src);
		BYTE high = Add8(GP_ADC, *(dest + 1), *(src + 1));
		WORD result = 256 * high + low;
		StoreW(dest, &result);
		m_Regs.F = (m_Regs.F & 0x01) | temp;
		break;
	}
	case GP_ADC:	// [**V*0?]
	case GP_SBC:	// [**V*1?]
	{
		BYTE low = Add8(pdr->ex, *dest, *src);
		BYTE high = Add8(pdr->ex, *(dest + 1), *(src + 1));
		WORD result = 256 * high + low;
		StoreW(dest, &result);
		SetFlag(FLAG_Z, (result == 0) ? TRUE : FALSE);
		break;
	}
	case GP_INC: (*((WORD*)dest))++; break;	// [------]
	case GP_DEC: (*((WORD*)dest))--;			// [------]
	}
}

// Jump instructions: JP,JR,DJNZ
void CZ80::DoJump(DESCREC* pdr)					// [------]
{
	if (pdr->ex == GP_JP)
	{
		WORD* dest = (WORD*)GetRegAddress(pdr->dest);
		//	0xc3 -> JP nn, 0xe9 -> JP (HL)
		if (pdr->icode == 0xc3 || pdr->icode == 0xe9 || GetJumpCondition(pdr->icode))
			m_Regs.PC = *dest;
	}
	else {
		BYTE* dest = (BYTE*)GetRegAddress(pdr->dest);
		BOOL dojump = FALSE;
		switch (pdr->icode) {
		case 0x10:	// DJNZ
			if ((--m_Regs.B) != 0) dojump = TRUE;
			break;
		case 0x18: dojump = TRUE; break;				// JR d
		case 0x20: dojump = !GetFlag(FLAG_Z); break;	// JR NZ,d
		case 0x28: dojump = GetFlag(FLAG_Z); break;		// JR Z, d
		case 0x30: dojump = !GetFlag(FLAG_C); break;	// JR NC,d
		case 0x38: dojump = GetFlag(FLAG_C);			// JR C, d
		}
		if (dojump) ModifyPC(*dest);
	}
}

// Caller and return instructions:
// CALL,RET,RETI,RETN,RST
void CZ80::DoCall(DESCREC* pdr)					// [------]
{
	switch (pdr->ex)
	{
	case GP_CALL:
	{
		WORD* dest = (WORD*)GetRegAddress(pdr->dest);
		//	0xcd -> CALL nn
		if (pdr->icode == 0xcd || GetJumpCondition(pdr->icode))
		{
			StackPC();
			m_Regs.PC = *dest;
		}
		break;
	}
	case GP_RETN:
	case GP_RETI:
	case GP_RET:
	{
		//	0xc9 -> RET
		if (pdr->icode == 0xc9 || GetJumpCondition(pdr->icode) ||
			pdr->ex == GP_RETI || pdr->ex == GP_RETN)
		{
			m_Regs.PC = *((WORD*)GetRegAddress(RG_SPINDIRECT));
			m_Regs.SP += 2;
			if (pdr->ex == GP_RETN)
				m_IFF1 = m_IFF2;
		}
		break;
	}
	case GP_RST:
	{
		StackPC();
		m_Regs.PC = WORD(pdr->icode & 0x38);
	}
	}
}

int CZ80::GetRegBy(BYTE code)
{
	switch (code)
	{
	case 0: return RG_B;
	case 1: return RG_C;
	case 2: return RG_D;
	case 3: return RG_E;
	case 4: return RG_H;
	case 5: return RG_L;
	case 6: return RG_HLINDIRECT;
	}
	return RG_A;
}

BOOL CZ80::DecodeCB(BYTE opcode, DESCREC* pdr)
{
	pdr->icode = opcode;
	pdr->dest = GetRegBy(opcode & 0x07);
	BYTE s = opcode & 0xc0;
	if (!s)
	{
		pdr->ig = IG_ROTSHIFT;
		pdr->src = 0;
		switch ((opcode & 0x38) >> 3)
		{
		case 0:  pdr->ex = GP_RLC; break;
		case 1:  pdr->ex = GP_RRC; break;
		case 2:  pdr->ex = GP_RL; break;
		case 3:  pdr->ex = GP_RR; break;
		case 4:  pdr->ex = GP_SLA; break;
		case 5:  pdr->ex = GP_SRA; break;
		case 6:  return TRUE;
		default: pdr->ex = GP_SRL;
		}
	}
	else
	{
		pdr->ig = IG_BIT;
		pdr->src = (opcode & 0x38) >> 3;
		switch (s)
		{
		case 0x40: pdr->ex = GP_BIT; break;
		case 0xc0: pdr->ex = GP_SET; break;
		default:   pdr->ex = GP_RES;
		}
	}
	return FALSE;
}

BOOL CZ80::DecodeED(BYTE opcode, DESCREC* pdr)
{
	for (int i = 0; i < 59; i++)
		if (Z80cpu_t2[i].icode == opcode)
		{
			pdr->icode = opcode;
			pdr->ig = Z80cpu_t2[i].ig;
			pdr->dest = Z80cpu_t2[i].dest;
			pdr->src = Z80cpu_t2[i].src;
			pdr->ex = Z80cpu_t2[i].ex;
			return FALSE;
		}
	return TRUE;
}

// Bit manager and tester instructions
// SET,RES,BIT
void CZ80::DoBit(DESCREC* pdr)
{
	BYTE* dest = (BYTE*)GetRegAddress(pdr->dest);
	BYTE a;
	switch (pdr->ex)
	{
	case GP_SET:	// [------]
		a = *dest | b_mask[pdr->src];
		StoreB(dest, &a);
		break;
	case GP_RES:	// [------]
		a = *dest & (~b_mask[pdr->src]);
		StoreB(dest, &a);
		break;
	case GP_BIT:	// [-*??01]
		SetFlag(FLAG_N, FALSE);
		SetFlag(FLAG_H, TRUE);
		SetFlag(FLAG_Z,
			((*dest & b_mask[pdr->src]) == 0) ? TRUE : FALSE);
	}
}

// Rotation and shift instructions:
// RLCA,RLA,RRCA,RRA,RLC,RR,RRC,RR,SLA,SRA,SRL,RLD,RRD
void CZ80::DoRotShift(DESCREC* pdr)
{
	BYTE* dest;
	BYTE value;
	m_Regs.F &= 0xed; // clear of the N,H signal bits

	if (pdr->ex == GP_RLD || pdr->ex == GP_RRD)
	{	// [-*P*00]
		dest = (BYTE*)GetRegAddress(RG_HLINDIRECT);
		BYTE a;
		if (pdr->ex == GP_RLD)
		{
			a = m_Regs.A & 0xf;
			m_Regs.A = (m_Regs.A & 0xf0) | (*dest >> 4);
			value = (*dest << 4) | a;
		}
		else
		{
			a = *dest & 0xf;
			value = (*dest >> 4) | (m_Regs.A << 4);
			m_Regs.A = (m_Regs.A & 0xf0) | a;
		}
		StoreB(dest, &value);
		dest = &m_Regs.A;
	}
	else {
		BYTE carry, cy;
		dest = (BYTE*)GetRegAddress(pdr->dest);
		switch (pdr->ex)
		{
		case GP_RLCA:				// [*---00]
		case GP_RLC:				// [**P*00]
			cy = *dest & 0x80;
			value = *dest << 1;
			if (cy) value |= 0x01;
			break;
		case GP_RRCA:				// [*---00]
		case GP_RRC:				// [**P*00]
			cy = *dest & 0x01;
			value = *dest >> 1;
			if (cy) value |= 0x80;
			break;
		case GP_RLA:				// [*---00]
		case GP_RL:					// [**P*00]
			cy = *dest & 0x80;
			value = *dest << 1;
			if (GetFlag(FLAG_C)) value |= 0x01;
			break;
		case GP_RRA:				// [*---00]
		case GP_RR:					// [**P*00]
			cy = *dest & 0x01;
			value = *dest >> 1;
			if (GetFlag(FLAG_C)) value |= 0x80;
			break;
		case GP_SLA:				// [**P*00]
			cy = *dest & 0x80;
			value = *dest << 1;
			break;
		case GP_SRL:				// [**P*00]
			cy = *dest & 0x01;
			value = *dest >> 1;
			break;
		case GP_SRA:				// [**P*00]
			cy = *dest & 0x01;
			carry = *dest & 0x80;
			value = (*dest >> 1) | carry;
		}

		StoreB(dest, &value);
		SetFlag(FLAG_C, (cy == 0) ? FALSE : TRUE);
	}

	switch (pdr->ex)
	{
	case GP_RLCA:
	case GP_RLA:
	case GP_RRCA:
	case GP_RRA: break;
	default:
		SetZS(*dest);
		SetFlag(FLAG_PV, m_ParityMap[*dest] ? TRUE : FALSE);
	}
}

// Exchange and block move instructions:
// EX,EXX,LDI,LDIR,LDD,LDDR,CPI,CPIR,CPD,CPDR
void CZ80::DoExBlock(DESCREC* pdr)
{
	if (pdr->ex == GP_EX)
	{			// [------]
		WORD* a, * b, temp;
		a = (WORD*)GetRegAddress(pdr->dest);
		b = (WORD*)GetRegAddress(pdr->src);
		temp = *a;
		StoreW((BYTE*)a, b);
		StoreW((BYTE*)b, &temp);
	}
	else if (pdr->ex == GP_EXX)
	{	// [------]
		BYTE temp;
		temp = m_Regs.B;
		m_Regs.B = m_Regs._B;
		m_Regs._B = temp;
		temp = m_Regs.C;
		m_Regs.C = m_Regs._C;
		m_Regs._C = temp;
		temp = m_Regs.D;
		m_Regs.D = m_Regs._D;
		m_Regs._D = temp;
		temp = m_Regs.E;
		m_Regs.E = m_Regs._E;
		m_Regs._E = temp;
		temp = m_Regs.H;
		m_Regs.H = m_Regs._H;
		m_Regs._H = temp;
		temp = m_Regs.L;
		m_Regs.L = m_Regs._L;
		m_Regs._L = temp;
	}
	else
	{
		BOOL dir, op, rel;
		switch (pdr->ex)
		{
		case GP_LDI:	dir = TRUE;  op = FALSE; rel = FALSE; break; // [--*-00]
		case GP_LDIR:	dir = TRUE;  op = FALSE; rel = TRUE;  break; // [--0-00]
		case GP_LDD:	dir = FALSE; op = FALSE; rel = FALSE; break; // [--*-00]
		case GP_LDDR:	dir = FALSE; op = FALSE; rel = TRUE;  break; // [--0-00]
		case GP_CPI:	dir = TRUE;  op = TRUE;  rel = FALSE; break; // [-***1*]
		case GP_CPIR:	dir = TRUE;  op = TRUE;  rel = TRUE;  break; // [-***1*]
		case GP_CPD:	dir = FALSE; op = TRUE;  rel = FALSE; break; // [-***1*]
		case GP_CPDR:	dir = FALSE; op = TRUE;  rel = TRUE;		 // [-***1*]
		}

		WORD* de, * hl, * bc;
		BYTE* ide, * ihl;
		de = (WORD*)GetRegAddress(RG_DE);
		hl = (WORD*)GetRegAddress(RG_HL);
		bc = (WORD*)GetRegAddress(RG_BC);
		ide = (BYTE*)GetRegAddress(RG_DEINDIRECT);
		ihl = (BYTE*)GetRegAddress(RG_HLINDIRECT);
		if (op)
		{
			BOOL fc = GetFlag(FLAG_C);
			Add8(GP_SUB, m_Regs.A, *ihl);
			SetFlag(FLAG_C, fc);
		}
		else
		{
			StoreB(ide, ihl);
			m_Regs.F &= 0xed;	// clear of the N,H signal flags
		}

		if (dir)
		{
			if (!op) (*de)++;
			(*hl)++;
		}
		else
		{
			if (!op) (*de)--;
			(*hl)--;
		}
		(*bc)--;
		if (op || pdr->ex == GP_LDI || pdr->ex == GP_LDD)
			SetFlag(FLAG_PV, (*bc == 1) ? FALSE : TRUE);
		else
			SetFlag(FLAG_PV, FALSE);
		if (rel && !(*bc == 0 || (op && GetFlag(FLAG_Z))))
			ModifyPC(-2);
	}
}

// Reading from the input periphery + report generation
BYTE CZ80::ReadIO(WORD addr)
{
	m_Report.io_op = RP_RD;
	m_Report.io_add = addr;
	return *(m_pPerIBase + (addr & 0xff));
}

// Writing to the output periphery + report generation
void CZ80::WriteIO(WORD addr, BYTE data)
{
	m_Report.io_op = RP_WR;
	m_Report.io_add = addr;
	*(m_pPerOBase + (addr & 0xff)) = data;
}

// I/O instructions:
// IN,INI,INIR,IND,INDR,OUT,OUTI,OTIR,OUTD,OTDR
void CZ80::DoIo(DESCREC* pdr)
{
	if (pdr->ex == GP_IN || pdr->ex == GP_INI || pdr->ex == GP_INIR ||
		pdr->ex == GP_IND || pdr->ex == GP_INDR)
	{
		if (m_HIBYTEIN == 0x8000)
		{
			if (pdr->icode == 0xdb)
				m_HIBYTEIN = m_Regs.A;
			else
				m_HIBYTEIN = m_Regs.B;
			m_TempIN.dest = pdr->dest;
			m_TempIN.ex = pdr->ex;
			m_TempIN.icode = pdr->icode;
			m_TempIN.ig = pdr->ig;
			m_TempIN.src = pdr->src;
			return;
		}
		else m_HIBYTEIN = 0x8000;
	}

	if (pdr->ex == GP_IN)
	{					// [------] or [-*P*00]
		BYTE* src = (BYTE*)GetRegAddress(pdr->src);
		BYTE* dest = (BYTE*)GetRegAddress(pdr->dest);
		BYTE d;
		WORD ioaddr;
		ioaddr = ((pdr->src == RG_BYTECONST) ? m_Regs.A : m_Regs.B) * 256 + *src;
		d = ReadIO(ioaddr);
		if (pdr->icode != 0x70) *dest = d;	// filteirng the IN F,(C)
		if (pdr->src != RG_BYTECONST)
		{
			m_Regs.F &= 0xed;				// clear of the N,H signal bits
			SetFlag(FLAG_PV, m_ParityMap[d] ? TRUE : FALSE);
			SetZS(d);
		}
	}
	else if (pdr->ex == GP_OUT)
	{			// [------]
		BYTE* src = (BYTE*)GetRegAddress(pdr->src);
		BYTE* dest = (BYTE*)GetRegAddress(pdr->dest);
		WORD ioaddr;
		ioaddr = ((pdr->dest == RG_BYTECONST) ? m_Regs.A : m_Regs.B) * 256 + *dest;
		WriteIO(ioaddr, *src);
	}
	else
	{
		SetFlag(FLAG_N, TRUE);
		BYTE* ihl = (BYTE*)GetRegAddress(RG_HLINDIRECT);
		BYTE* c = (BYTE*)GetRegAddress(RG_C);

		BOOL type, dir, rec;
		switch (pdr->ex)
		{
		case GP_INI:	type = FALSE; dir = TRUE;  rec = FALSE; break; // [-*??1?]
		case GP_INIR:	type = FALSE; dir = TRUE;  rec = TRUE;  break; // [-1??1?]
		case GP_IND:	type = FALSE; dir = FALSE; rec = FALSE; break; // [-*??1?]
		case GP_INDR:	type = FALSE; dir = FALSE; rec = TRUE;  break; // [-1??1?]
		case GP_OUTI:	type = TRUE;  dir = TRUE;  rec = FALSE; break; // [-*??1?]
		case GP_OTIR:	type = TRUE;  dir = TRUE;  rec = TRUE;  break; // [-1??1?]
		case GP_OUTD:	type = TRUE;  dir = FALSE; rec = FALSE; break; // [-*??1?]
		case GP_OTDR:	type = TRUE;  dir = FALSE; rec = TRUE;		   // [-1??1?]
		}

		WORD ioaddr = m_Regs.B * 256 + m_Regs.C;
		if (type)
			WriteIO(ioaddr, *ihl);
		else
		{
			BYTE a = ReadIO(ioaddr);
			StoreB(ihl, &a);
		}

		WORD* hl = (WORD*)GetRegAddress(RG_HL);
		if (dir) (*hl)++; else (*hl)--;

		m_Regs.B--;
		if (rec && m_Regs.B != 0) ModifyPC(-2);
		if (rec || (!rec && m_Regs.B == 1))
			SetFlag(FLAG_Z, TRUE);
		else
			SetFlag(FLAG_Z, FALSE);
	}
}

void CZ80::StackPC()
{
	m_Regs.SP -= 2;
	BYTE* spdest = (BYTE*)GetRegAddress(RG_SPINDIRECT);
	StoreW(spdest, &m_Regs.PC);
}

// Binds the CPU to the memory and periphery.
void CZ80::SetCPUProp(INITCPU& iCPU)
{
	m_MMask = iCPU.MMask;
	m_pMemoryBase = iCPU.pMem;
	m_pPerIBase = iCPU.pInp;
	m_pPerOBase = iCPU.pOutp;
}

void CZ80::GetCPUProp(INITCPU& iCPU)
{
	iCPU.MMask = m_MMask;
	iCPU.pInp = m_pPerIBase;
	iCPU.pMem = m_pMemoryBase;
	iCPU.pOutp = m_pPerOBase;
}

// Creates the parity table.
void CZ80::CreateParityMap()
{
	BOOL p;
	for (int i = 0; i < 256; i++)
	{
		p = TRUE;
		for (int j = 0; j < 8; j++)
			if ((i >> j) & 1)
				p = !p;
		m_ParityMap[i] = p ? 1 : 0;
	}
}