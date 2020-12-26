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

#if !defined(ZILOG_Z80_CPU_DISASSEMBLER_V0100_INCLUDED)
#define ZILOG_Z80_CPU_DISASSEMBLER_V0100_INCLUDED

#include "z80table.h"
#include "z80.h"

#define ISDDFD(a) (a == 0xdd || a == 0xfd)

class CZ80Dis
{
protected:
	BYTE		m_prefix;
	BYTE		m_prefixDDFD;
	BYTE		m_Buff[4];
	BOOL		m_isDDFD;
	char		m_szAddr[6];
	char		m_szData[10];
	char		m_szMnemonic[6];
	char		m_szOp[16];
	BYTE* m_pMem;
	WORD		m_Addr;
	int			m_Index;
	BYTE		m_d;
	BOOL		m_isD;
	DESCREC		m_Rec;

	void		BuildOp();
	void		BuildData();
	void		BuildDEFB();
	BYTE		Fetch();
	void		BuildAll();
	void		BuildOneReg(int reg);
	void		ChangeRecCont(int*, BOOL);
	void		CopyRec(DESCREC*);

public:
	char		m_szLine[32];

	CZ80Dis();
	~CZ80Dis();
	CZ80Dis(BYTE* pMem);
	int			BuildLine(WORD Addr);
	void		SetMemBase(BYTE* pMem) { m_pMem = pMem; }
};

#endif