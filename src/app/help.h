/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

#if !defined(ZX_SPECTRUM_HELP_V0100_INCLUDED)
#define ZX_SPECTRUM_HELP_V0100_INCLUDED

typedef struct _keyhelpitems
{
    int cat;
    char *string;
    char *keycomb;
} KEYHELPITEMS;

void HelpKeyboard();

#endif