/*
 *	SpEm - ZX Spectrum 48K emulator
 *	Copyright (c) 2000 Norbert László
 *
 *	This code is licensed under MIT license (see LICENSE.txt for details)
 *	https://mit-license.org/
 *
 *	https://github.com/fox20io/spem
 */

#include "StdAfx.h"
#include "Ula.h"
#include "spem.h"

 // Representation of the keyboard matrix.
 // The bit is set at the position of the pressed key.
 // NOTE: The original hardware clears this bit.
BYTE KeyMatrix[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

KEYMATRIX	ScanCodeToSpec[128] = {
	{0,0}, {0,0}, {3,1}, {3,2}, {3,4}, {3,8}, {3,16}, {4,16},//0
	{4,8}, {4,4}, {4,2}, {4,1}, {0,0}, {0,0}, {0,0}, {0,0},  //8
	{2,1}, {2,2}, {2,4}, {2,8}, {2,16}, {5,16}, {5,8}, {5,4},//16
	{5,2}, {5,1}, {0,0}, {0,0}, {6,1}, {0,0}, {1,1}, {1,2},  //24
	{1,4}, {1,8}, {1,16}, {6,16}, {6,8}, {6,4}, {6,2}, {0,0},//32
	{0,0}, {0,0}, {0,1}, {0,0}, {0,2}, {0,4}, {0,8}, {0,16}, //40
	{7,16}, {7,8}, {7,4}, {0,0}, {0,0}, {7,2}, {0,0}, {0,0}, //48
	{0,0}, {7,1}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}   //56
};

// RGB colour components for the Spectrum's 8 base and 8 intensity colours.
CLRS SpecClrsRGB[16] =
{
	{ 000, 000, 000 },
	{ 000, 000, 200 },
	{ 200, 000, 000 },
	{ 200, 000, 200 },
	{ 000, 200, 000 },
	{ 000, 200, 200 },
	{ 200, 200, 000 },
	{ 220, 220, 220 },
	{ 000, 000, 000 },
	{ 000, 000, 255 },
	{ 255, 000, 000 },
	{ 255, 000, 255 },
	{ 000, 255, 000 },
	{ 000, 255, 255 },
	{ 255, 255, 000 },
	{ 255, 255, 255 }
};

// 16-bit array of Spectrum colours.
WORD	SpecClrs16[16];

BYTE* pVMem = NULL;
WORD					LineAddr[192];
WORD					AddrLine[2048];
BYTE					BitCount;
int						DisplayMagnify = DMODE_1X;
BOOL					fDisplayFullScreen = FALSE;
int						FlashSpeed = 10;
BOOL					fFlashFlag = FALSE;
int						FlashCounter = 0;


// DirectDraw objects
IDirectDraw* pDD = NULL;
IDirectDrawSurface* pDDSurface = NULL;
IDirectDrawSurface* pDDSBack = NULL;
IDirectDrawPalette* pDDPalette = NULL;
IDirectDrawClipper* pDDClipper = NULL;

// DirectInput objects
IDirectInput8* pDI;
LPDIRECTINPUTDEVICE8 pDIk;

// DirectSound objects
LPDIRECTSOUND			pDS = NULL;
LPDIRECTSOUNDBUFFER		pDSB = NULL;

// External globals
extern HWND				hMainWnd;
extern HINSTANCE		hInst;
extern BYTE* pOutp;
extern BOOL				fBorder;

//
// Generates acceleration table for the video display
// LineAddr[192]:	Contains video memory addresses for line coordinates
//					(addresses of the first byte in each line)
// AddrLine[2048]:	Contains coordinate pairs for video memory addresses
//					mapping only a third of the video memory (coordinates
//					of the first bit of each 8-bit pixel groups)
//
void BuildAccelTables()
{
	WORD wAddr;
	int y;

	for (int i = 0; i < 192; i++)
	{
		if (i > 127)
		{
			wAddr = 4096;
			y = i - 128;
		}

		else if (i > 63)
		{
			wAddr = 2048;
			y = i - 64;
		}

		else
		{
			wAddr = 0;
			y = i;
		}

		wAddr += (y >> 3) * 32 + (y & 7) * 256;
		LineAddr[i] = wAddr;
	}

	for (y = 0; y < 64; y++)
		for (int i = 0; i < 32; i++)
			AddrLine[LineAddr[i] + i] = 256 * y + i * 8;
}

//
// Initializes the DirectInput
//
BOOL InitDI()
{
	if (FAILED(DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pDI, NULL)))
		return TRUE;

	if (FAILED(pDI->CreateDevice(GUID_SysKeyboard, &pDIk, NULL)))
		return TRUE;

	if (FAILED(pDIk->SetDataFormat(&c_dfDIKeyboard)))
		return TRUE;

	if (FAILED(pDIk->SetCooperativeLevel(hMainWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
		return TRUE;

	return FALSE;
}

//
// Closes the DirectInput
//
void TermDI()
{
	if (pDIk)
		pDIk->Unacquire();

	SAFERELEASE(pDIk);
	SAFERELEASE(pDI);
}

//
// Initializes the "Out-of-Screen" DD surface
//
BOOL InitDDSBack()
{
	HRESULT hr;
	DDSURFACEDESC ddsd;

	SAFERELEASE(pDDSBack);

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

	if (fBorder)
	{
		ddsd.dwWidth = 320;
		ddsd.dwHeight = 240;
	}
	else
	{
		ddsd.dwWidth = 256;
		ddsd.dwHeight = 192;
	}

	hr = pDD->CreateSurface(&ddsd, &pDDSBack, NULL);
	if (hr != DD_OK)
		return FALSE;

	return TRUE;
}

//
// Initializes the DirectDraw
//
BOOL InitDD()
{
	SAFERELEASE(pDD);
	SAFERELEASE(pDDSurface);
	SAFERELEASE(pDDPalette);

	HRESULT hr;

	hr = DirectDrawCreate(NULL, &pDD, NULL);
	if (hr != DD_OK)
		return FALSE;

	DWORD dwFlags = fDisplayFullScreen
		? (DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE)
		: DDSCL_NORMAL;

	hr = pDD->SetCooperativeLevel(hMainWnd, dwFlags);
	if (hr != DD_OK)
		return FALSE;

	if (fDisplayFullScreen)
	{
		hr = pDD->SetDisplayMode(320, 200, 8);
		// CR: hr isn't checked!?
	}

	DDSURFACEDESC ddsd;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = pDD->CreateSurface(&ddsd, &pDDSurface, NULL);
	if (hr != DD_OK)
		return FALSE;

	if (!InitDDSBack())
		return FALSE;

	if (!fDisplayFullScreen)
	{
		hr = pDD->CreateClipper(0, &pDDClipper, 0);
		if (hr != DD_OK)
			return FALSE;

		hr = pDDClipper->SetHWnd(0, hMainWnd);

		pDDSurface->SetClipper(pDDClipper);
	}

	pDD->GetDisplayMode(&ddsd);
	BitCount = BYTE(ddsd.ddpfPixelFormat.dwRGBBitCount);

	if (BitCount < 8)
	{
		Failure(IDS_FAIL_FEWBITCOUNT);
		PostQuitMessage(WM_DESTROY);
	}

	//	Build SpecClrs16 colour table for 65536 colour screen
	//	bit masks:	RED		0xf800
	//				GREEN	0x07c0
	//				BLUE	0x001f
	if (BitCount == 16)
	{
		BYTE r, g, b;
		WORD c;

		for (int i = 0; i < 16; i++)
		{
			r = ((32 * SpecClrsRGB[i].r) / 256);
			g = ((32 * SpecClrsRGB[i].g) / 256);
			b = ((32 * SpecClrsRGB[i].b) / 256);
			c = 0;
			c |= r;
			c <<= 5;
			c |= g;
			c <<= 6;
			c |= b;
			SpecClrs16[i] = c;
		}
	}

	if (BitCount == 8)
	{
		PALETTEENTRY ape[256];
		HDC hdc = ::GetDC(NULL);
		GetSystemPaletteEntries(hdc, 0, 256, ape);
		for (int i = 0; i < 16; i++)
		{
			ape[50 + i].peBlue = SpecClrsRGB[i].b;
			ape[50 + i].peGreen = SpecClrsRGB[i].g;
			ape[50 + i].peRed = SpecClrsRGB[i].r;
		}

		hr = pDD->CreatePalette(DDPCAPS_8BIT, ape, &pDDPalette, NULL);
		if (hr == DD_OK)
			pDDSurface->SetPalette(pDDPalette);

		::ReleaseDC(NULL, hdc);
	}

	return TRUE;
}

//
// Terminates the DirectDraw
//
void TermDD()
{
	SAFERELEASE(pDDClipper);
	SAFERELEASE(pDDSurface);
	SAFERELEASE(pDDSBack);
	SAFERELEASE(pDDPalette);
	SAFERELEASE(pDD);
}

//
// Draws the video image on the screen.
//
void DrawScreen()
{
	HRESULT hr;
	RECT rect;

	if (fDisplayFullScreen)
	{
		::GetWindowRect(::GetDesktopWindow(), &rect);
	}
	else
	{
		::GetClientRect(hMainWnd, &rect);
		POINT point;
		point.x = point.y = 0;
		::ClientToScreen(hMainWnd, &point);
		::OffsetRect(&rect, point.x, point.y);
	}

	hr = pDDSurface->Blt(&rect, pDDSBack, 0, 0, NULL);
	// CR: hr isn't checked!?
}

//
// Generating the content of the keyboard matrix using DirectInput
//
void BuildKeyMatrix()
{
	HRESULT hr;
	BYTE diks[256];

	if (pDIk)
	{
		if (SUCCEEDED(pDIk->Acquire()))
		{
		again:
			hr = pDIk->GetDeviceState(sizeof(diks), &diks);
			if (hr == DIERR_INPUTLOST)
			{
				if (SUCCEEDED(pDIk->Acquire()))
					goto again;
			}
			else if (SUCCEEDED(hr))
			{
				for (int i = 0; i < 64; i++)
				{
					if (diks[i] & 0x80)
						KeyMatrix[ScanCodeToSpec[i].row] |= ScanCodeToSpec[i].pos;
					else
						KeyMatrix[ScanCodeToSpec[i].row] &= ~ScanCodeToSpec[i].pos;
				}
			}
		}
	}
}

//
// Displaying the content of the video memory in one step with border.
//
void Video()
{
	DDSURFACEDESC ddsd;
	ddsd.dwSize = sizeof(ddsd);

	if (pDDSBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK)
		return;

	int j, k;
	BYTE c, border;
	BYTE* pdd, * pvm, * pam;
	BOOL bit;

	border = *(pOutp + 0xfe) & 7;

	for (BYTE ActLine = 0; ActLine < 240; ActLine++)
	{
		pdd = (BYTE*)ddsd.lpSurface + ddsd.lPitch * ActLine;

		for (j = 0; j < 32; j++)
		{
			switch (BitCount)
			{
			case 8:
				*pdd = 50 + border;
				pdd += 1;
				break;

			case 16:
				*pdd = SpecClrs16[border] & 0x00ff;
				*(pdd + 1) = SpecClrs16[border] / 256;
				pdd += 2;
				break;

			case 24:
				*pdd = SpecClrsRGB[border].b;
				*(pdd + 1) = SpecClrsRGB[border].g;
				*(pdd + 2) = SpecClrsRGB[border].r;
				pdd += 3;
				break;

			case 32:
				*pdd = SpecClrsRGB[border].b;
				*(pdd + 1) = SpecClrsRGB[border].g;
				*(pdd + 2) = SpecClrsRGB[border].r;
				pdd += 4;
			}
		}

		if (ActLine >= 24 && ActLine < 216)
		{
			pvm = pVMem + LineAddr[ActLine - 24];
			pam = pVMem + 6144 + ((ActLine - 24) >> 3) * 32;

			for (j = 0; j < 256; j += 8)
			{
				for (k = 0; k < 8; k++)
				{
					bit = ((*(pvm + (j >> 3)) << k) & 0x80) ? TRUE : FALSE;

					if (fFlashFlag && (*pam & 0x80))
						bit = !bit;

					c = bit ? (*pam & 7) : (((*pam) >> 3) & 7);

					if (*pam & 0x40)
						c += 8;

					switch (BitCount)
					{
					case 8:
						*pdd = 50 + c;
						pdd += 1;
						break;

					case 16:
						*pdd = SpecClrs16[c] & 0x00ff;
						*(pdd + 1) = SpecClrs16[c] / 256;
						pdd += 2;
						break;

					case 24:
						*pdd = SpecClrsRGB[c].b;
						*(pdd + 1) = SpecClrsRGB[c].g;
						*(pdd + 2) = SpecClrsRGB[c].r;
						pdd += 3;
						break;

					case 32:
						*pdd = SpecClrsRGB[c].b;
						*(pdd + 1) = SpecClrsRGB[c].g;
						*(pdd + 2) = SpecClrsRGB[c].r;
						pdd += 4;
					}
				}
				pam++;
			}
		}
		else
		{
			for (j = 0; j < 256; j++)
			{
				switch (BitCount)
				{
				case 8:
					*pdd = 50 + border;
					pdd += 1;
					break;

				case 16:
					*pdd = SpecClrs16[border] & 0x00ff;
					*(pdd + 1) = SpecClrs16[border] / 256;
					pdd += 2;
					break;

				case 24:
					*pdd = SpecClrsRGB[border].b;
					*(pdd + 1) = SpecClrsRGB[border].g;
					*(pdd + 2) = SpecClrsRGB[border].r;
					pdd += 3;
					break;

				case 32:
					*pdd = SpecClrsRGB[border].b;
					*(pdd + 1) = SpecClrsRGB[border].g;
					*(pdd + 2) = SpecClrsRGB[border].r;
					pdd += 4;
				}
			}
		}

		for (j = 0; j < 32; j++)
		{
			switch (BitCount)
			{
			case 8:
				*pdd = 50 + border;
				pdd += 1;
				break;

			case 16:
				*pdd = SpecClrs16[border] & 0x00ff;
				*(pdd + 1) = SpecClrs16[border] / 256;
				pdd += 2;
				break;

			case 24:
				*pdd = SpecClrsRGB[border].b;
				*(pdd + 1) = SpecClrsRGB[border].g;
				*(pdd + 2) = SpecClrsRGB[border].r;
				pdd += 3;
				break;

			case 32:
				*pdd = SpecClrsRGB[border].b;
				*(pdd + 1) = SpecClrsRGB[border].g;
				*(pdd + 2) = SpecClrsRGB[border].r;
				pdd += 4;
			}
		}
	}

	if (FlashCounter >= VIDEO_FLASH_RATE)
	{
		fFlashFlag = !fFlashFlag;
		FlashCounter = 0;
	}
	else
	{
		FlashCounter++;
	}

	pDDSBack->Unlock(NULL);
	DrawScreen();
}

//
// Displaying the content of the video memory in one step without border.
//
void VideoNoBorder()
{
	DDSURFACEDESC ddsd;
	ddsd.dwSize = sizeof(ddsd);

	if (pDDSBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK)
		return;

	int j, k;
	BYTE c, border;
	BYTE* pdd, * pvm, * pam;
	BOOL bit;

	border = *(pOutp + 0xfe) & 7;

	for (BYTE ActLine = 0; ActLine < 192; ActLine++)
	{
		pdd = (BYTE*)ddsd.lpSurface + ddsd.lPitch * ActLine;

		pvm = pVMem + LineAddr[ActLine];
		pam = pVMem + 6144 + (ActLine >> 3) * 32;
		for (j = 0; j < 256; j += 8)
		{
			for (k = 0; k < 8; k++)
			{
				bit = ((*(pvm + (j >> 3)) << k) & 0x80) ? TRUE : FALSE;

				if (fFlashFlag && (*pam & 0x80))
					bit = !bit;

				c = bit ? (*pam & 7) : (((*pam) >> 3) & 7);

				if (*pam & 0x40)
					c += 8;

				switch (BitCount)
				{
				case 8:
					*pdd = 50 + c;
					pdd += 1;
					break;

				case 16:
					*pdd = SpecClrs16[c] & 0x00ff;
					*(pdd + 1) = SpecClrs16[c] / 256;
					pdd += 2;
					break;

				case 24:
					*pdd = SpecClrsRGB[c].b;
					*(pdd + 1) = SpecClrsRGB[c].g;
					*(pdd + 2) = SpecClrsRGB[c].r;
					pdd += 3;
					break;

				case 32:
					*pdd = SpecClrsRGB[c].b;
					*(pdd + 1) = SpecClrsRGB[c].g;
					*(pdd + 2) = SpecClrsRGB[c].r;
					pdd += 4;
				}
			}
			pam++;
		}
	}

	if (FlashCounter >= VIDEO_FLASH_RATE)
	{
		fFlashFlag = !fFlashFlag;
		FlashCounter = 0;
	}
	else
	{
		FlashCounter++;
	}

	pDDSBack->Unlock(NULL);
	DrawScreen();
}

//
// Displaying the content of the video memory for full screen in one step with border.
//
void VideoFullScreen()
{
	DDSURFACEDESC ddsd;
	ddsd.dwSize = sizeof(ddsd);

	if (pDDSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK)
		return;

	int j, k;
	BYTE c, border;
	BYTE* pdd, * pvm, * pam;
	BOOL bit;

	border = *(pOutp + 0xfe) & 7;

	for (BYTE ActLine = 0; ActLine < 200; ActLine++)
	{
		pdd = (BYTE*)ddsd.lpSurface + ddsd.lPitch * ActLine;

		for (j = 0; j < 32; j++)
		{
			*pdd = 50 + border;
			pdd++;
		}

		if (ActLine >= 4 && ActLine < 196)
		{
			pvm = pVMem + LineAddr[ActLine - 4];
			pam = pVMem + 6144 + ((ActLine - 4) >> 3) * 32;

			for (j = 0; j < 256; j += 8)
			{
				for (k = 0; k < 8; k++)
				{
					bit = ((*(pvm + (j >> 3)) << k) & 0x80) ? TRUE : FALSE;

					if (fFlashFlag && (*pam & 0x80))
						bit = !bit;

					c = bit ? (*pam & 7) : (((*pam) >> 3) & 7);
					if (*pam & 0x40)
						c += 8;

					*pdd = 50 + c;
					pdd++;
				}
				pam++;
			}
		}
		else
		{
			for (j = 0; j < 256; j++)
			{
				*pdd = 50 + border;
				pdd++;
			}
		}

		for (j = 0; j < 32; j++)
		{
			*pdd = 50 + border;
			pdd++;
		}
	}

	if (FlashCounter >= VIDEO_FLASH_RATE)
	{
		fFlashFlag = !fFlashFlag;
		FlashCounter = 0;
	}
	else
	{
		FlashCounter++;
	}

	pDDSurface->Unlock(NULL);
}