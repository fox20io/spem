/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 * 
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *	
 *	https://github.com/fox20io/spem
 */

#if !defined(ZX_SPECTRUM_DEBUGGER_V0100_INCLUDED)
#define ZX_SPECTRUM_DEBUGGER_V0100_INCLUDED

#define MAX_LINES 60

LRESULT CALLBACK DebuggerWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK JumpWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PokeWndProc(HWND, UINT, WPARAM, LPARAM);

void LoadRegisters(HWND);

#endif