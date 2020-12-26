/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

#if !defined(ZX_SPECTRUM_SPEAKER_V0100_INCLUDED)
#define ZX_SPECTRUM_SPEAKER_V0100_INCLUDED

#define	BUFFER_RATE		22050
#define BUFFER_LENGTH	BUFFER_RATE*2

void	InitWaveOut();
void	TermWaveOut();
void	IncWaveCursor(BYTE b);
void	CALLBACK TimerProc(UINT, UINT, DWORD, DWORD, DWORD);
void	StartPlay();
void	StopPlay();
//void	CALLBACK OutProc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);

#endif