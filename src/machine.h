/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

#if !defined(ZX_SPECTRUM_48K_V0100_INCLUDED)
#define ZX_SPECTRUM_48K_V0100_INCLUDED

#include "z80.h"
#include "Ula.h"

enum { SPEED_REAL, SPEED_SYNCTOVIDEO, SPEED_FULL };

#define CPU_FREQUENCY_HZ  3500000
#define CPU_AVG_MCYCLES   4.5
#define CPU_REAL_TICKS	7000
#define CPU_IDLE_TICKS	5000
#define	CPU_INT_TIMER	20

void				InitContext();
void				Operate();
LRESULT CALLBACK	Speed(HWND,UINT,WPARAM,LPARAM);
void				SpeedTest();

void RandomMemory();

typedef struct _THREADSTRUCT
{
    LPBYTE lpSpeaker;
    LPVOID lpAudioBuffer;
    DWORD* lpStartTime;
} THREADSTRUCT;

#endif