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
#include "z80table.h"

DESCREC z80cpu_tCB[256];
DESCREC z80cpu_tED[256];

//////////////////////////////////////////////////////////////////
//	Prefix nélküli utasításkódok táblázata

DESCREC Z80cpu_t1[256] = {
	{ 0x00, IG_GEN, 0, 0, GP_NOP },						// NOP
	{ 0x01, IG_LD16, RG_BC, RG_WORDCONST, 0 },			// LD BC,nn
	{ 0x02, IG_LD8, RG_BCINDIRECT, RG_A, 0 },			// LD (BC),A
	{ 0x03, IG_ALU16, RG_BC, 0, GP_INC },				// INC BC
	{ 0x04, IG_ALU8, RG_B, 0, GP_INC },					// INC B
	{ 0x05, IG_ALU8, RG_B, 0, GP_DEC },					// DEC B
	{ 0x06, IG_LD8, RG_B, RG_BYTECONST },				// LD B,n
	{ 0x07, IG_ROTSHIFT, RG_A, 0, GP_RLCA },			// RLCA

	{ 0x08, IG_EXBLOCK, RG_AF, RG__AF, GP_EX },			// EX AF,AF'
	{ 0x09, IG_ALU16, RG_HL, RG_BC, GP_ADD },			// ADD HL,BC
	{ 0x0A, IG_LD8, RG_A, RG_BCINDIRECT, 0 },			// LD A,(BC)
	{ 0x0B, IG_ALU16, RG_BC, 0, GP_DEC },				// DEC BC
	{ 0x0C, IG_ALU8, RG_C, 0, GP_INC },					// INC C
	{ 0x0D, IG_ALU8, RG_C, 0, GP_DEC },					// DEC C
	{ 0x0E, IG_LD8, RG_C, RG_BYTECONST, 0 },			// LD C,n
	{ 0x0F, IG_ROTSHIFT, RG_A, 0, GP_RRCA },			// RRCA

	{ 0x10, IG_JUMP, RG_BYTECONST, 0, GP_DJNZ },		// DJNZ n
	{ 0x11, IG_LD16, RG_DE, RG_WORDCONST, 0 },			// LD DE,nn
	{ 0x12, IG_LD8, RG_DEINDIRECT, RG_A, 0 },			// LD (DE),A
	{ 0x13, IG_ALU16, RG_DE, 0, GP_INC },				// INC DE
	{ 0x14, IG_ALU8, RG_D, 0, GP_INC },					// INC D
	{ 0x15, IG_ALU8, RG_D, 0, GP_DEC },					// DEC D
	{ 0x16, IG_LD8, RG_D, RG_BYTECONST, 0 },			// LD D,n
	{ 0x17, IG_ROTSHIFT, RG_A, 0, GP_RLA },				// RLA

	{ 0x18, IG_JUMP, RG_BYTECONST, 0, GP_JR },			// JR n
	{ 0x19, IG_ALU16, RG_HL, RG_DE, GP_ADD },			// ADD HL,DE
	{ 0x1A, IG_LD8, RG_A, RG_DEINDIRECT, 0 },			// LD A,(DE)
	{ 0x1B, IG_ALU16, RG_DE, 0, GP_DEC },				// DEC DE
	{ 0x1C, IG_ALU8, RG_E, 0, GP_INC },					// INC E
	{ 0x1D, IG_ALU8, RG_E, 0, GP_DEC },					// DEC E
	{ 0x1E, IG_LD8, RG_E, RG_BYTECONST, 0 },			// LD E,n
	{ 0x1F, IG_ROTSHIFT, RG_A, 0, GP_RRA },				// RRA

	{ 0x20, IG_JUMP, RG_BYTECONST, 0, GP_JR },			// JR NZ,n
	{ 0x21, IG_LD16, RG_HL, RG_WORDCONST, 0 },			// LD HL,nn
	{ 0x22, IG_LD16, RG_WORDCONSTINDIRECT, RG_HL, 0 },	// LD (nn),HL
	{ 0x23, IG_ALU16, RG_HL, 0, GP_INC },				// INC HL
	{ 0x24, IG_ALU8, RG_H, 0, GP_INC },					// INC H
	{ 0x25, IG_ALU8, RG_H, 0, GP_DEC },					// DEC H
	{ 0x26, IG_LD8, RG_H, RG_BYTECONST, 0 },			// LD H,n
	{ 0x27, IG_GEN, 0, 0, GP_DAA },						// DAA

	{ 0x28, IG_JUMP, RG_BYTECONST, 0, GP_JR },			// JR Z,n
	{ 0x29, IG_ALU16, RG_HL, RG_HL, GP_ADD },			// ADD HL,HL
	{ 0x2A, IG_LD16, RG_HL, RG_WORDCONSTINDIRECT, 0 },	// LD HL,(nn)
	{ 0x2B, IG_ALU16, RG_HL, 0, GP_DEC },				// DEC HL
	{ 0x2C, IG_ALU8, RG_L, 0, GP_INC },					// INC L
	{ 0x2D, IG_ALU8, RG_L, 0, GP_DEC },					// DEC L
	{ 0x2E, IG_LD8, RG_L, RG_BYTECONST, 0 },			// LD L,n
	{ 0x2F, IG_GEN, 0, 0, GP_CPL },						// CPL

	{ 0x30, IG_JUMP, RG_BYTECONST, 0, GP_JR },			// JR NC,n
	{ 0x31, IG_LD16, RG_SP, RG_WORDCONST, 0 },			// LD SP,nn
	{ 0x32, IG_LD8, RG_WORDCONSTINDIRECT, RG_A, 0 },	// LD (nn),A
	{ 0x33, IG_ALU16, RG_SP, 0, GP_INC },				// INC SP
	{ 0x34, IG_ALU8, RG_HLINDIRECT, 0, GP_INC },		// INC (HL)
	{ 0x35, IG_ALU8, RG_HLINDIRECT, 0, GP_DEC },		// DEC (HL)
	{ 0x36, IG_LD8, RG_HLINDIRECT, RG_BYTECONST, 0 },	// LD (HL),n
	{ 0x37, IG_GEN, 0, 0, GP_SCF },						// SCF

	{ 0x38, IG_JUMP, RG_BYTECONST, 0, GP_JR },			// JR C,n
	{ 0x39, IG_ALU16, RG_HL, RG_SP, GP_ADD },			// ADD HL,SP
	{ 0x3A, IG_LD8, RG_A, RG_WORDCONSTINDIRECT, 0 },	// LD A,(nn)
	{ 0x3B, IG_ALU16, RG_SP, 0, GP_DEC },				// DEC SP
	{ 0x3C, IG_ALU8, RG_A, 0, GP_INC },					// INC A
	{ 0x3D, IG_ALU8, RG_A, 0, GP_DEC },					// DEC A
	{ 0x3E, IG_LD8, RG_A, RG_BYTECONST, 0 },			// LD A,n
	{ 0x3F, IG_GEN, 0, 0, GP_CCF },						// CCF

	{ 0x40, IG_LD8, RG_B, RG_B, 0 },					// LD B,B
	{ 0x41, IG_LD8, RG_B, RG_C, 0 },					// LD B,C
	{ 0x42, IG_LD8, RG_B, RG_D, 0 },					// LD B,D
	{ 0x43, IG_LD8, RG_B, RG_E, 0 },					// LD B,E
	{ 0x44, IG_LD8, RG_B, RG_H, 0 },					// LD B,H
	{ 0x45, IG_LD8, RG_B, RG_L, 0 },					// LD B,L
	{ 0x46, IG_LD8, RG_B, RG_HLINDIRECT, 0 },			// LD B,(HL)
	{ 0x47, IG_LD8, RG_B, RG_A, 0 },					// LD B,A

	{ 0x48, IG_LD8, RG_C, RG_B, 0 },					// LD C,B
	{ 0x49, IG_LD8, RG_C, RG_C, 0 },					// LD C,C
	{ 0x4A, IG_LD8, RG_C, RG_D, 0 },					// LD C,D
	{ 0x4B, IG_LD8, RG_C, RG_E, 0 },					// LD C,E
	{ 0x4C, IG_LD8, RG_C, RG_H, 0 },					// LD C,H
	{ 0x4D, IG_LD8, RG_C, RG_L, 0 },					// LD C,L
	{ 0x4E, IG_LD8, RG_C, RG_HLINDIRECT, 0 },			// LD C,(HL)
	{ 0x4F, IG_LD8, RG_C, RG_A, 0 },					// LD C,A

	{ 0x50, IG_LD8, RG_D, RG_B, 0 },					// LD D,B
	{ 0x51, IG_LD8, RG_D, RG_C, 0 },					// LD D,C
	{ 0x52, IG_LD8, RG_D, RG_D, 0 },					// LD D,D
	{ 0x53, IG_LD8, RG_D, RG_E, 0 },					// LD D,E
	{ 0x54, IG_LD8, RG_D, RG_H, 0 },					// LD D,H
	{ 0x55, IG_LD8, RG_D, RG_L, 0 },					// LD D,L
	{ 0x56, IG_LD8, RG_D, RG_HLINDIRECT, 0 },			// LD D,(HL)
	{ 0x57, IG_LD8, RG_D, RG_A, 0 },					// LD D,A

	{ 0x58, IG_LD8, RG_E, RG_B, 0 },					// LD E,B
	{ 0x59, IG_LD8, RG_E, RG_C, 0 },					// LD E,C
	{ 0x5A, IG_LD8, RG_E, RG_D, 0 },					// LD E,D
	{ 0x5B, IG_LD8, RG_E, RG_E, 0 },					// LD E,E
	{ 0x5C, IG_LD8, RG_E, RG_H, 0 },					// LD E,H
	{ 0x5D, IG_LD8, RG_E, RG_L, 0 },					// LD E,L
	{ 0x5E, IG_LD8, RG_E, RG_HLINDIRECT, 0 },			// LD E,(HL)
	{ 0x5F, IG_LD8, RG_E, RG_A, 0 },					// LD E,A

	{ 0x60, IG_LD8, RG_H, RG_B, 0 },					// LD H,B
	{ 0x61, IG_LD8, RG_H, RG_C, 0 },					// LD H,C
	{ 0x62, IG_LD8, RG_H, RG_D, 0 },					// LD H,D
	{ 0x63, IG_LD8, RG_H, RG_E, 0 },					// LD H,E
	{ 0x64, IG_LD8, RG_H, RG_H, 0 },					// LD H,H
	{ 0x65, IG_LD8, RG_H, RG_L, 0 },					// LD H,L
	{ 0x66, IG_LD8, RG_H, RG_HLINDIRECT, 0 },			// LD H,(HL)
	{ 0x67, IG_LD8, RG_H, RG_A, 0 },					// LD H,A

	{ 0x68, IG_LD8, RG_L, RG_B, 0 },					// LD L,B
	{ 0x69, IG_LD8, RG_L, RG_C, 0 },					// LD L,C
	{ 0x6A, IG_LD8, RG_L, RG_D, 0 },					// LD L,D
	{ 0x6B, IG_LD8, RG_L, RG_E, 0 },					// LD L,E
	{ 0x6C, IG_LD8, RG_L, RG_H, 0 },					// LD L,H
	{ 0x6D, IG_LD8, RG_L, RG_L, 0 },					// LD L,L
	{ 0x6E, IG_LD8, RG_L, RG_HLINDIRECT, 0 },			// LD L,(HL)
	{ 0x6F, IG_LD8, RG_L, RG_A, 0 },					// LD L, A

	{ 0x70, IG_LD8, RG_HLINDIRECT, RG_B, 0 },			// LD (HL),B
	{ 0x71, IG_LD8, RG_HLINDIRECT, RG_C, 0 },			// LD (HL),C
	{ 0x72, IG_LD8, RG_HLINDIRECT, RG_D, 0 },			// LD (HL),D
	{ 0x73, IG_LD8, RG_HLINDIRECT, RG_E, 0 },			// LD (HL),E
	{ 0x74, IG_LD8, RG_HLINDIRECT, RG_H, 0 },			// LD (HL),H
	{ 0x75, IG_LD8, RG_HLINDIRECT, RG_L, 0 },			// LD (HL),L
	{ 0x76, IG_GEN, 0, 0, GP_HALT },					// HALT
	{ 0x77, IG_LD8, RG_HLINDIRECT, RG_A, 0 },			// LD (HL),A

	{ 0x78, IG_LD8, RG_A, RG_B, 0 },					// LD A,B
	{ 0x79, IG_LD8, RG_A, RG_C, 0 },					// LD A,C
	{ 0x7A, IG_LD8, RG_A, RG_D, 0 },					// LD A,D
	{ 0x7B, IG_LD8, RG_A, RG_E, 0 },					// LD A,E
	{ 0x7C, IG_LD8, RG_A, RG_H, 0 },					// LD A,H
	{ 0x7D, IG_LD8, RG_A, RG_L, 0 },					// LD A,L
	{ 0x7E, IG_LD8, RG_A, RG_HLINDIRECT, 0 },			// LD A,(HL)
	{ 0x7F, IG_LD8, RG_A, RG_A, 0 },					// LD A,A

	{ 0x80, IG_ALU8, RG_A, RG_B, GP_ADD},				// ADD A,B
	{ 0x81, IG_ALU8, RG_A, RG_C, GP_ADD},				// ADD A,C
	{ 0x82, IG_ALU8, RG_A, RG_D, GP_ADD},				// ADD A,D
	{ 0x83, IG_ALU8, RG_A, RG_E, GP_ADD},				// ADD A,E
	{ 0x84, IG_ALU8, RG_A, RG_H, GP_ADD},				// ADD A,H
	{ 0x85, IG_ALU8, RG_A, RG_L, GP_ADD},				// ADD A,L
	{ 0x86, IG_ALU8, RG_A, RG_HLINDIRECT, GP_ADD},		// ADD A,(HL)
	{ 0x87, IG_ALU8, RG_A, RG_A, GP_ADD},				// ADD A,A

	{ 0x88, IG_ALU8, RG_A, RG_B, GP_ADC},				// ADC A,B
	{ 0x89, IG_ALU8, RG_A, RG_C, GP_ADC},				// ADC A,C
	{ 0x8A, IG_ALU8, RG_A, RG_D, GP_ADC},				// ADC A,D
	{ 0x8B, IG_ALU8, RG_A, RG_E, GP_ADC},				// ADC A,E
	{ 0x8C, IG_ALU8, RG_A, RG_H, GP_ADC},				// ADC A,H
	{ 0x8D, IG_ALU8, RG_A, RG_L, GP_ADC},				// ADC A,L
	{ 0x8E, IG_ALU8, RG_A, RG_HLINDIRECT, GP_ADC},		// ADC A,(HL)
	{ 0x8F, IG_ALU8, RG_A, RG_A, GP_ADC},				// ADC A,A

	{ 0x90, IG_ALU8, RG_A, RG_B, GP_SUB},				// SUB B
	{ 0x91, IG_ALU8, RG_A, RG_C, GP_SUB},				// SUB C
	{ 0x92, IG_ALU8, RG_A, RG_D, GP_SUB},				// SUB D
	{ 0x93, IG_ALU8, RG_A, RG_E, GP_SUB},				// SUB E
	{ 0x94, IG_ALU8, RG_A, RG_H, GP_SUB},				// SUB H
	{ 0x95, IG_ALU8, RG_A, RG_L, GP_SUB},				// SUB L
	{ 0x96, IG_ALU8, RG_A, RG_HLINDIRECT, GP_SUB},		// SUB (HL)
	{ 0x97, IG_ALU8, RG_A, RG_A, GP_SUB},				// SUB A

	{ 0x98, IG_ALU8, RG_A, RG_B, GP_SBC},				// SBC A,B
	{ 0x99, IG_ALU8, RG_A, RG_C, GP_SBC},				// SBC A,C
	{ 0x9A, IG_ALU8, RG_A, RG_D, GP_SBC},				// SBC A,D
	{ 0x9B, IG_ALU8, RG_A, RG_E, GP_SBC},				// SBC A,E
	{ 0x9C, IG_ALU8, RG_A, RG_H, GP_SBC},				// SBC A,H
	{ 0x9D, IG_ALU8, RG_A, RG_L, GP_SBC},				// SBC A,L
	{ 0x9E, IG_ALU8, RG_A, RG_HLINDIRECT, GP_SBC},		// SBC A,(HL)
	{ 0x9F, IG_ALU8, RG_A, RG_A, GP_SBC},				// SBC A,A

	{ 0xA0, IG_ALU8, RG_A, RG_B, GP_AND},				// AND B
	{ 0xA1, IG_ALU8, RG_A, RG_C, GP_AND},				// AND C
	{ 0xA2, IG_ALU8, RG_A, RG_D, GP_AND},				// AND D
	{ 0xA3, IG_ALU8, RG_A, RG_E, GP_AND},				// AND E
	{ 0xA4, IG_ALU8, RG_A, RG_H, GP_AND},				// AND H
	{ 0xA5, IG_ALU8, RG_A, RG_L, GP_AND},				// AND L
	{ 0xA6, IG_ALU8, RG_A, RG_HLINDIRECT, GP_AND},		// AND (HL)
	{ 0xA7, IG_ALU8, RG_A, RG_A, GP_AND},				// AND A

	{ 0xA8, IG_ALU8, RG_A, RG_B, GP_XOR},				// XOR B
	{ 0xA9, IG_ALU8, RG_A, RG_C, GP_XOR},				// XOR C
	{ 0xAA, IG_ALU8, RG_A, RG_D, GP_XOR},				// XOR D
	{ 0xAB, IG_ALU8, RG_A, RG_E, GP_XOR},				// XOR E
	{ 0xAC, IG_ALU8, RG_A, RG_H, GP_XOR},				// XOR H
	{ 0xAD, IG_ALU8, RG_A, RG_L, GP_XOR},				// XOR L
	{ 0xAE, IG_ALU8, RG_A, RG_HLINDIRECT, GP_XOR},		// XOR (HL)
	{ 0xAF, IG_ALU8, RG_A, RG_A, GP_XOR},				// XOR A

	{ 0xB0, IG_ALU8, RG_A, RG_B, GP_OR},				// OR B
	{ 0xB1, IG_ALU8, RG_A, RG_C, GP_OR},				// OR C
	{ 0xB2, IG_ALU8, RG_A, RG_D, GP_OR},				// OR D
	{ 0xB3, IG_ALU8, RG_A, RG_E, GP_OR},				// OR E
	{ 0xB4, IG_ALU8, RG_A, RG_H, GP_OR},				// OR H
	{ 0xB5, IG_ALU8, RG_A, RG_L, GP_OR},				// OR L
	{ 0xB6, IG_ALU8, RG_A, RG_HLINDIRECT, GP_OR},		// OR (HL)
	{ 0xB7, IG_ALU8, RG_A, RG_A, GP_OR},				// OR A

	{ 0xB8, IG_ALU8, RG_A, RG_B, GP_CP},				// CP B
	{ 0xB9, IG_ALU8, RG_A, RG_C, GP_CP},				// CP C
	{ 0xBA, IG_ALU8, RG_A, RG_D, GP_CP},				// CP D
	{ 0xBB, IG_ALU8, RG_A, RG_E, GP_CP},				// CP E
	{ 0xBC, IG_ALU8, RG_A, RG_H, GP_CP},				// CP H
	{ 0xBD, IG_ALU8, RG_A, RG_L, GP_CP},				// CP L
	{ 0xBE, IG_ALU8, RG_A, RG_HLINDIRECT, GP_CP},		// CP (HL)
	{ 0xBF, IG_ALU8, RG_A, RG_A, GP_CP},				// CP A

	{ 0xC0, IG_CALL, 0, 0, GP_RET },					// RET NZ
	{ 0xC1, IG_LD16, RG_BC, 0, GP_POP },				// POP BC
	{ 0xC2, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP NZ,nn
	{ 0xC3, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP nn
	{ 0xC4, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL NZ,nn
	{ 0xC5, IG_LD16, RG_BC, 0, GP_PUSH },				// PUSH BC
	{ 0xC6, IG_ALU8, RG_A, RG_BYTECONST, GP_ADD },		// ADD A,n
	{ 0xC7, IG_CALL, 0, 0, GP_RST },					// RST 0

	{ 0xC8, IG_CALL, 0, 0, GP_RET },					// RET Z
	{ 0xC9, IG_CALL, 0, 0, GP_RET },					// RET
	{ 0xCA, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP Z,nn
	{ 0xCB, IG_INVALID, 0, 0, 0 },						// *
	{ 0xCC, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL Z,nn
	{ 0xCD, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL nn
	{ 0xCE, IG_ALU8, RG_A, RG_BYTECONST, GP_ADC },		// ADC A,n
	{ 0xCF, IG_CALL, 0, 0, GP_RST },					// RST 8

	{ 0xD0, IG_CALL, 0, 0, GP_RET },					// RET NC
	{ 0xD1, IG_LD16, RG_DE, 0, GP_POP },				// POP DE
	{ 0xD2, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP NC,nn
	{ 0xD3, IG_IO, RG_BYTECONSTIO, RG_A, GP_OUT },		// OUT (n),A
	{ 0xD4, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL NC,nn
	{ 0xD5, IG_LD16, RG_DE, 0, GP_PUSH },				// PUSH DE
	{ 0xD6, IG_ALU8, RG_A, RG_BYTECONST, GP_SUB },		// SUB n
	{ 0xD7, IG_CALL, 0, 0, GP_RST },					// RST 10H

	{ 0xD8, IG_CALL, 0, 0, GP_RET },					// RET C
	{ 0xD9, IG_EXBLOCK, 0, 0, GP_EXX },					// EXX
	{ 0xDA, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP C,nn
	{ 0xDB, IG_IO, RG_A, RG_BYTECONSTIO, GP_IN },		// IN A,(n)
	{ 0xDC, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL C,nn
	{ 0xDD, IG_INVALID, 0, 0, 0 },						// *
	{ 0xDE, IG_ALU8, RG_A, RG_BYTECONST, GP_SBC },		// SBC A,n
	{ 0xDF, IG_CALL, 0, 0, GP_RST },					// RST 18H

	{ 0xE0, IG_CALL, 0, 0, GP_RET },					// RET PO
	{ 0xE1, IG_LD16, RG_HL, 0, GP_POP },				// POP HL
	{ 0xE2, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP PO,nn
	{ 0xE3, IG_EXBLOCK, RG_SPINDIRECT, RG_HL, GP_EX },	// EX (SP),HL
	{ 0xE4, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL PO,nn
	{ 0xE5, IG_LD16, RG_HL, 0, GP_PUSH },				// PUSH HL
	{ 0xE6, IG_ALU8, RG_A, RG_BYTECONST, GP_AND },		// AND n
	{ 0xE7, IG_CALL, 0, 0, GP_RST },					// RST 20H

	{ 0xE8, IG_CALL, 0, 0, GP_RET },					// RET PE
	{ 0xE9, IG_JUMP, RG_HLJUMP, 0, GP_JP },				// JP (HL)
	{ 0xEA, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP PE,nn
	{ 0xEB, IG_EXBLOCK, RG_DE, RG_HL, GP_EX },			// EX DE,HL
	{ 0xEC, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL PE,nn
	{ 0xED, IG_INVALID, 0, 0, 0 },						// *
	{ 0xEE, IG_ALU8, RG_A, RG_BYTECONST, GP_XOR },		// XOR n
	{ 0xEF, IG_CALL, 0, 0, GP_RST },					// RST 28H

	{ 0xF0, IG_CALL, 0, 0, GP_RET },					// RET P
	{ 0xF1, IG_LD16, RG_AF, 0, GP_POP },				// POP AF
	{ 0xF2, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP P,nn
	{ 0xF3, IG_GEN, 0, 0, GP_DI },						// DI
	{ 0xF4, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL P,nn
	{ 0xF5, IG_LD16, RG_AF, 0, GP_PUSH },				// PUSH AF
	{ 0xF6, IG_ALU8, RG_A, RG_BYTECONST, GP_OR },		// OR n
	{ 0xF7, IG_CALL, 0, 0, GP_RST },					// RST 30H

	{ 0xF8, IG_CALL, 0, 0, GP_RET },					// RET M
	{ 0xF9, IG_LD16, RG_SP, RG_HL, 0 },					// LD SP,HL
	{ 0xFA, IG_JUMP, RG_WORDCONST, 0, GP_JP },			// JP M,nn
	{ 0xFB, IG_GEN, 0, 0, GP_EI },						// EI
	{ 0xFC, IG_CALL, RG_WORDCONST, 0, GP_CALL },		// CALL M,nn
	{ 0xFD, IG_INVALID, 0, 0, 0 },						// *
	{ 0xFE, IG_ALU8, RG_A, RG_BYTECONST, GP_CP },		// CP n
	{ 0XFF, IG_CALL, 0, 0, GP_RST }						// RST 38H
};

//////////////////////////////////////////////////////////////////
//	Az 'ED' prefixet követõ utasításkódok táblázata

DESCREC Z80cpu_t2[59] = {
	{ 0x40, IG_IO, RG_B, RG_CINDIRECT, GP_IN },			// IN B,(C)
	{ 0x41, IG_IO, RG_CINDIRECT, RG_B, GP_OUT },		// OUT (C),B
	{ 0x42, IG_ALU16, RG_HL, RG_BC, GP_SBC },			// SBC HL,BC
	{ 0x43, IG_LD16, RG_WORDCONSTINDIRECT, RG_BC, 0 },	// LD (nn),BC
	{ 0x44, IG_GEN, 0, 0, GP_NEG },						// NEG
	{ 0x45, IG_CALL, 0, 0, GP_RETN },					// RETN
	{ 0x46, IG_GEN, 0, 0, GP_IM },						// IM 0
	{ 0x47, IG_LD8, RG_I, RG_A, 0 },					// LD I,A
	{ 0x48, IG_IO, RG_C, RG_CINDIRECT, GP_IN },			// IN C,(C)
	{ 0x49, IG_IO, RG_CINDIRECT, RG_C, GP_OUT },		// OUT (C),C
	{ 0x4A, IG_ALU16, RG_HL, RG_BC, GP_ADC },			// ADC HL,BC
	{ 0x4B, IG_LD16, RG_BC, RG_WORDCONSTINDIRECT, 0 },	// LD BC,(nn)
	{ 0x4D, IG_CALL, 0, 0, GP_RETI },					// RETI
	{ 0x4F, IG_LD8, RG_R, RG_A, 0 },					// LD R,A

	{ 0x50, IG_IO, RG_D, RG_CINDIRECT, GP_IN },			// IN D,(C)
	{ 0x51, IG_IO, RG_CINDIRECT, RG_D, GP_OUT },		// OUT (C),D
	{ 0x52, IG_ALU16, RG_HL, RG_DE, GP_SBC },			// SBC HL,DE
	{ 0x53, IG_LD16, RG_WORDCONSTINDIRECT, RG_DE, 0 },	// LD (nn),DE
	{ 0x56, IG_GEN, 0, 0, GP_IM },						// IM 1
	{ 0x57, IG_LD8, RG_A, RG_I, 0 },					// LD A,I
	{ 0x58, IG_IO, RG_E, RG_CINDIRECT, GP_IN },			// IN E,(C)
	{ 0x59, IG_IO, RG_CINDIRECT, RG_E, GP_OUT },		// OUT (C),E
	{ 0x5A, IG_ALU16, RG_HL, RG_DE, GP_ADC },			// ADC HL,DE
	{ 0x5B, IG_LD16, RG_DE, RG_WORDCONSTINDIRECT, 0 },	// LD DE,(nn)
	{ 0x5E, IG_GEN, 0, 0, GP_IM },						// IM 2
	{ 0x5F, IG_LD8, RG_A, RG_R, 0 },					// LD A,R

	{ 0x60, IG_IO, RG_H, RG_CINDIRECT, GP_IN },			// IN H,(C)
	{ 0x61, IG_IO, RG_CINDIRECT, RG_H, GP_OUT },		// OUT (C),H
	{ 0x62, IG_ALU16, RG_HL, RG_HL, GP_SBC },			// SBC HL,HL
	{ 0x63, IG_LD16, RG_WORDCONSTINDIRECT, RG_HL, 0 },	// LD (nn),HL
	{ 0x67, IG_ROTSHIFT, 0, 0, GP_RRD },				// RRD
	{ 0x68, IG_IO, RG_L, RG_CINDIRECT, GP_IN },			// IN L,(C)
	{ 0x69, IG_IO, RG_CINDIRECT, RG_L, GP_OUT },		// OUT (C),L
	{ 0x6A, IG_ALU16, RG_HL, RG_HL, GP_ADC },			// ADC HL,HL
	{ 0x6B, IG_LD16, RG_HL, RG_WORDCONSTINDIRECT, 0 },	// LD HL,(nn)
	{ 0x6F, IG_ROTSHIFT, 0, 0, GP_RLD },				// RLD

	{ 0x70, IG_IO, RG_AF, RG_CINDIRECT, GP_IN },		// IN F,(C)
	{ 0x72, IG_ALU16, RG_HL, RG_SP, GP_SBC },			// SBC HL,SP
	{ 0x73, IG_LD16, RG_WORDCONSTINDIRECT, RG_SP, 0 },	// LD (nn),SP
	{ 0x78, IG_IO, RG_A, RG_CINDIRECT, GP_IN },			// IN A,(C)
	{ 0x79, IG_IO, RG_CINDIRECT, RG_A, GP_OUT },		// OUT (C),A
	{ 0x7A, IG_ALU16, RG_HL, RG_SP, GP_ADC },			// ADC HL,SP
	{ 0x7B, IG_LD16, RG_SP, RG_WORDCONSTINDIRECT, 0 },	// LD HL,(nn)

	{ 0xA0, IG_EXBLOCK, 0, 0, GP_LDI },
	{ 0xA1, IG_EXBLOCK, 0, 0, GP_CPI },
	{ 0xA2, IG_IO, 0, 0, GP_INI },
	{ 0xA3, IG_IO, 0, 0, GP_OUTI },
	{ 0xA8, IG_EXBLOCK, 0, 0, GP_LDD },
	{ 0xA9, IG_EXBLOCK, 0, 0, GP_CPD },
	{ 0xAA, IG_IO, 0, 0, GP_IND },
	{ 0xAB, IG_IO, 0, 0, GP_OUTD },

	{ 0xB0, IG_EXBLOCK, 0, 0, GP_LDIR },
	{ 0xB1, IG_EXBLOCK, 0, 0, GP_CPIR },
	{ 0xB2, IG_IO, 0, 0, GP_INIR },
	{ 0xB3, IG_IO, 0, 0, GP_OTIR },
	{ 0xB8, IG_EXBLOCK, 0, 0, GP_LDDR },
	{ 0xB9, IG_EXBLOCK, 0, 0, GP_CPDR },
	{ 0xBA, IG_IO, 0, 0, GP_INDR },
	{ 0xBB, IG_IO, 0, 0, GP_OTDR }
};

//////////////////////////////////////////////////////////////////
//	A parameterkent atadott kod alapjan visszad egy regiszter-
//	azonositot.
//	A Z80 "beleertett" cimzesi modja, 3 biten tarolja.

int GetRegBy(BYTE code)
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

//////////////////////////////////////////////////////////////////
//

void CreateTableCB()
{
	for (int opcode = 0; opcode < 256; opcode++)
	{
		if (opcode >= 0x30 && opcode < 0x38)
		{
			z80cpu_tCB[opcode].ig = IG_INVALID;
			continue;
		}
		z80cpu_tCB[opcode].icode = opcode;
		z80cpu_tCB[opcode].dest = GetRegBy(opcode & 0x07);
		BYTE s = opcode & 0xc0;
		if (!s)
		{
			z80cpu_tCB[opcode].ig = IG_ROTSHIFT;
			z80cpu_tCB[opcode].src = 0;
			switch ((opcode & 0x38) >> 3)
			{
			case 0:  z80cpu_tCB[opcode].ex = GP_RLC; break;
			case 1:  z80cpu_tCB[opcode].ex = GP_RRC; break;
			case 2:  z80cpu_tCB[opcode].ex = GP_RL; break;
			case 3:  z80cpu_tCB[opcode].ex = GP_RR; break;
			case 4:  z80cpu_tCB[opcode].ex = GP_SLA; break;
			case 5:  z80cpu_tCB[opcode].ex = GP_SRA; break;
			case 6:  break;
			default: z80cpu_tCB[opcode].ex = GP_SRL;
			}
		}
		else
		{
			z80cpu_tCB[opcode].ig = IG_BIT;
			z80cpu_tCB[opcode].src = (opcode & 0x38) >> 3;
			switch (s)
			{
			case 0x40: z80cpu_tCB[opcode].ex = GP_BIT; break;
			case 0xc0: z80cpu_tCB[opcode].ex = GP_SET; break;
			default:   z80cpu_tCB[opcode].ex = GP_RES;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
//

void CreateTableED()
{
	int j;
	for (int i = 0; i < 256; i++)
		z80cpu_tED[i].ig = IG_INVALID;
	for (int i = 0; i < 59; i++)
	{
		j = Z80cpu_t2[i].icode;
		z80cpu_tED[j].dest = Z80cpu_t2[i].dest;
		z80cpu_tED[j].src = Z80cpu_t2[i].src;
		z80cpu_tED[j].icode = Z80cpu_t2[i].icode;
		z80cpu_tED[j].ig = Z80cpu_t2[i].ig;
		z80cpu_tED[j].ex = Z80cpu_t2[i].ex;
	}
}