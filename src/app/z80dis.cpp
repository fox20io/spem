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

#include "StdAfx.h"
#include "z80dis.h"

extern DESCREC Z80cpu_t1[];
extern DESCREC z80cpu_tCB[];
extern DESCREC z80cpu_tED[];
extern BYTE* pMem;

char* pszMnemonic[] =
{
	"",
	"ADC  ", "ADD  ", "AND  ", "BIT  ", "CALL ", "CCF  ", "CP   ", "CPD  ", "CPDR ",
	"CPI  ", "CPIR ", "CPL  ", "DAA  ", "DEC  ", "DI   ", "DJNZ ", "EI   ", "EX   ",
	"EXX  ", "HALT ", "IM   ", "IN   ", "INC  ", "IND  ", "INDR ", "INI  ", "INIR ",
	"JP   ", "JR   ", "LD   ", "LDD  ", "LDDR ", "LDI  ", "LDIR ", "NEG  ", "NOP  ",
	"OR   ", "OTDR ", "OTIR ", "OUT  ", "OUTD ", "OUTI ", "POP  ", "PUSH ", "RES  ",
	"RET  ", "RETI ", "RETN ", "RL   ", "RLA  ", "RLC  ", "RLCA ", "RLD  ", "RR   ",
	"RRA  ", "RRC  ", "RRCA ", "RRD  ", "RST  ", "SBC  ", "SCF  ", "SET  ", "SLA  ",
	"SRA  ", "SRL  ", "SUB  ", "XOR  "
};

char* pszReg[] =
{
	"",
	"A", "B", "C", "D", "E", "H", "L", "I", "R", "SP", "`AF", "AF", "BC",
	"DE", "HL", "IX", "IY", "IXL", "IXH", "IYL", "IYH", "(IX+%.2X)", "(IY+%.2X)",
	"(BC)", "(DE)", "(HL)", "%.2X", "%.4X", "(%.4X)", "(SP)", "(C)",
	"(HL)", "(IX)", "(IY)", "(%.2X)"
};

char* pszJumpCon[] =
{
	"NZ,", "Z,", "NC,", "C,", "PO,", "PE,", "P,", "M,", ""
};

CZ80Dis::CZ80Dis()
{
}

CZ80Dis::CZ80Dis(BYTE* pMem)
{
	CZ80Dis();
	m_pMem = pMem;
}

CZ80Dis::~CZ80Dis()
{
}

BYTE CZ80Dis::Fetch()
{
	BYTE b = *(m_pMem + m_Addr);
	m_Addr++;
	m_Buff[m_Index] = b;
	m_Index++;
	return b;
}

// Copies one record of the instruction descriptor table into m_Rec.
void CZ80Dis::CopyRec(DESCREC* p)
{
	m_Rec.dest = p->dest;
	m_Rec.src = p->src;
	m_Rec.ex = p->ex;
	m_Rec.icode = p->icode;
	m_Rec.ig = p->ig;
}

// In case of DD, FD prefixes makes the prefix related modification on
// the record read from the instruction descriptor table (m_Rec).
// The L,H registers don't change if isHLi is FALSE.
void CZ80Dis::ChangeRecCont(int* p, BOOL isHLi)
{
	int xyl, xyh, xyi, xyii, xyjump;
	if (m_prefixDDFD == 0xdd)
	{
		xyl = RG_XL;
		xyh = RG_XH;
		xyi = RG_IX;
		xyjump = xyii = RG_IXINDIRECT;
	}
	else
	{
		xyl = RG_YL;
		xyh = RG_YH;
		xyi = RG_IY;
		xyjump = xyii = RG_IYINDIRECT;
	}

	switch (*p)
	{
	case RG_L:			if (isHLi) *p = xyl; break;
	case RG_H:			if (isHLi) *p = xyh; break;
	case RG_HL:			*p = xyi; break;
	case RG_HLJUMP:		*p = xyjump; break;
	case RG_HLINDIRECT:
		*p = xyii;
		if (!m_isD) m_d = Fetch();
	}
}

// Builds the operand string
void CZ80Dis::BuildOp()
{
	if (m_isDDFD)
	{
		BOOL isHLi;
		if (m_Rec.dest == RG_HLINDIRECT || m_Rec.src == RG_HLINDIRECT)
			isHLi = FALSE;
		else
			isHLi = TRUE;

		if (m_prefix == 0xcb)
		{
			if (m_Rec.dest == RG_HLINDIRECT)
				m_Rec.dest = (m_prefixDDFD == 0xdd) ? RG_IXINDIRECT : RG_IYINDIRECT;
			else if (m_prefix == 0xed)
			{
				// TODO...
			}
			else {
				if (m_Rec.icode != 0xeb)
				{ // EX DE,IX or EX DE,IY doesn't exist
					ChangeRecCont(&m_Rec.dest, isHLi);
					ChangeRecCont(&m_Rec.src, isHLi);
				}
			}
		}
	}

	strcpy(m_szOp, "");
	BuildOneReg(m_Rec.dest);
	if (m_Rec.src)
	{
		strcat(m_szOp, ",");
		BuildOneReg(m_Rec.src);
	}
}

// Builds a register string
void CZ80Dis::BuildOneReg(int reg)
{
	char s[16];
	BYTE bl, bh;
	switch (reg)
	{
	case RG_IXINDIRECT:
	case RG_IYINDIRECT:
		sprintf(s, pszReg[reg], m_d);
		strcat(m_szOp, s);
		break;
	case RG_BYTECONSTIO:
	case RG_BYTECONST:
		sprintf(s, pszReg[reg], Fetch());
		strcat(m_szOp, s);
		break;
	case RG_WORDCONST:
	case RG_WORDCONSTINDIRECT:
		bl = Fetch();
		bh = Fetch();
		sprintf(s, pszReg[reg], 256 * bh + bl);
		strcat(m_szOp, s);
		break;
	default:
		strcat(m_szOp, pszReg[reg]);
	}
}

// Builds the data field string
void CZ80Dis::BuildData()
{
	char s[16];
	strcpy(m_szData, "");
	for (int i = 0; i < m_Index; i++)
	{
		sprintf(s, "%.2X", m_Buff[i]);
		strcat(m_szData, s);
	}
}

// Builds the DEFB menonic-operand string
void CZ80Dis::BuildDEFB()
{
	char s[16];
	strcpy(m_szMnemonic, "DEFB ");
	strcpy(m_szOp, "");
	for (int i = 0; i < m_Index; i++)
	{
		sprintf(s, "%.2X", m_Buff[i]);
		strcat(m_szOp, s);
		if (i != m_Index - 1)
			strcat(m_szOp, ",");
	}
	BuildData();
	BuildAll();
}

// Builds the complete output string
void CZ80Dis::BuildAll()
{
	char samp[] = { "                " };
	strcat(m_szLine, m_szAddr);
	strncat(m_szData, samp, 9 - strlen(m_szData));
	strcat(m_szLine, m_szData);
	strncat(m_szMnemonic, samp, 5 - strlen(m_szMnemonic));
	strcat(m_szLine, m_szMnemonic);
	strcat(m_szLine, m_szOp);
}

// Builds the disassembled string (szLine)
int CZ80Dis::BuildLine(WORD Addr)
{
	m_isDDFD = m_isD = FALSE;
	m_Addr = Addr;
	m_Index = 0;
	strcpy(m_szLine, "");

	// building the address
	sprintf(m_szAddr, "%.4X ", Addr);

	BYTE icode = Fetch();
	if (ISDDFD(icode))
	{
		m_isDDFD = TRUE;
		m_prefixDDFD = icode;
		icode = Fetch();
	}

	m_prefix = icode;
	switch (icode)
	{
	case 0xcb:
		icode = Fetch();
		if (m_isDDFD)
		{
			m_d = icode;
			m_isD = TRUE;
			icode = Fetch();
		}
		CopyRec(&z80cpu_tCB[icode]);
		break;
	case 0xed:
		if (m_isDDFD)
		{
			BuildDEFB();
			return m_Index;
		}
		icode = Fetch();
		CopyRec(&z80cpu_tED[icode]);
		break;
	case 0xdd:
	case 0xfd:
		BuildDEFB();
		return m_Index;
	default:
		CopyRec(&Z80cpu_t1[icode]);
	}

	// building the mnemonic
	if (m_Rec.ig == IG_LD8 || (m_Rec.ig == IG_LD16 && !m_Rec.ex))
		m_Rec.ex = GP_LD;
	strcpy(m_szMnemonic, pszMnemonic[m_Rec.ex]);

	int i;
	switch (m_Rec.ig)
	{
	case IG_LD8:		BuildOp(); break;
	case IG_LD16:		BuildOp(); break;
	case IG_ALU8:
		switch (m_Rec.ex)
		{
		case GP_SUB:
		case GP_AND:
		case GP_OR:
		case GP_XOR:
		case GP_CP:
			strcpy(m_szOp, "");
			BuildOneReg(m_Rec.src);
			break;
		default: BuildOp();
		}
		break;
	case IG_ALU16:		
		BuildOp(); 
		break;
	case IG_GEN:
		switch (m_Rec.icode)
		{
		case 0x46: strcpy(m_szOp, "0"); break;
		case 0x56: strcpy(m_szOp, "1"); break;
		case 0x5e: strcpy(m_szOp, "2"); break;
		default:
			BuildOp();
			break;
		}
	case IG_JUMP:
		switch (m_Rec.ex)
		{
		case GP_JP:
			if (m_Rec.icode == 0xc3 || m_Rec.icode == 0xe9)
				i = 8;
			else i = (m_Rec.icode & 0x38) >> 3;
			strcpy(m_szOp, pszJumpCon[i]);
			BuildOneReg(m_Rec.dest);
			break;
		case GP_JR:
		case GP_DJNZ:
		{
			switch (m_Rec.icode)
			{
			case 0x10:
			case 0x18: i = 8; break;
			case 0x38: i = 3; break;
			case 0x30: i = 2; break;
			case 0x28: i = 1; break;
			case 0x20: i = 0;
			}
			strcpy(m_szOp, pszJumpCon[i]);
			BYTE n = Fetch();
			char s[10];
			sprintf(s, "%.4X", WORD(m_Addr + WORD(int(char(n)))));
			strncat(m_szOp, s, 5);
		}
		}
		break;
	case IG_CALL:
		switch (m_Rec.ex)
		{
		case GP_CALL:
		case GP_RET:
			if (m_Rec.icode == 0xcd || m_Rec.icode == 0xc9) i = 8;
			else i = (m_Rec.icode & 0x38) >> 3;
			strcpy(m_szOp, pszJumpCon[i]);
			if (m_Rec.ex == GP_RET) m_szOp[strlen(m_szOp) - 1] = 0;
			break;
		case GP_RST:
			sprintf(m_szOp, "%.2X", m_Rec.icode & 0x38);
			break;
		default:
			strcpy(m_szOp, "");
		}
		BuildOneReg(m_Rec.dest);
		break;
	case IG_ROTSHIFT:
		switch (m_Rec.ex)
		{
		case GP_RLCA:
		case GP_RLA:
		case GP_RRCA:
		case GP_RRA:
			strcpy(m_szOp, "");
			break;
		default:
			BuildOp();
		}
		break;
	case IG_EXBLOCK:
		BuildOp();
		break;
	case IG_BIT:
		sprintf(m_szOp, "%d,", m_Rec.src);
		if (m_isDDFD && m_Rec.dest == RG_HLINDIRECT)
			ChangeRecCont(&m_Rec.dest, 0);
		BuildOneReg(m_Rec.dest);
		break;
	case IG_IO:			
		BuildOp();
	}

	BuildData();
	BuildAll();
	return m_Index;
}