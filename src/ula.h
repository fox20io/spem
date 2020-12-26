/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

#if !defined(ZX_SPECTRUM_ULA_V0100_INCLUDED)
#define ZX_SPECTRUM_ULA_V0100_INCLUDED

enum { DMODE_1X = 1, DMODE_2X, DMODE_FULLSCREEN };

typedef struct _ATTR { BYTE ink, paper, bright, flash; } ATTR;
typedef struct _CLRS { BYTE r, g, b; } CLRS;

typedef struct _KEYMATRIX
{
	BYTE row;
	BYTE pos;
} KEYMATRIX;

#define VIDEO_FLASH_RATE	12
#define DI_VERSION			0x0300

BOOL	InitDD();
BOOL	InitDI();
BOOL	InitDDSBack();
void	TermDD();
void	TermDI();
void	DrawScreen();
void	BuildKeyMatrix();
void	BuildAccelTables();

void	Video();
void	VideoNoBorder();
void	VideoFullScreen();

#endif
