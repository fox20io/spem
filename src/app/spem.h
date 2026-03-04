/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

#if !defined(AFX_SAMPLE_H__73ED832E_EC8D_11D3_B897_876BF6C40F4C__INCLUDED_)
#define AFX_SAMPLE_H__73ED832E_EC8D_11D3_B897_876BF6C40F4C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

#define MAX_LOADSTRING 100
#define SAFERELEASE(p) if (p) { p->Release(); p = NULL; }

void Failure(int textid);

#endif // !defined(AFX_SAMPLE_H__73ED832E_EC8D_11D3_B897_876BF6C40F4C__INCLUDED_)
