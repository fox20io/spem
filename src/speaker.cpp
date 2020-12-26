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
#include "speaker.h"
#include "machine.h"

WAVEFORMATEX	sWfx;
WAVEHDR			sWhdr;
BYTE			wb[BUFFER_LENGTH];
BYTE			wb2[BUFFER_LENGTH];
HWAVEOUT	  	hWo = 0;
DWORD			dwEvents = 0;
DWORD			dwSec;
MMRESULT		idTimer;
DWORD           dwStartTime = 0;
int             iLastPos = 0;

extern	HWND	hMainWnd;
extern  THREADSTRUCT gl_ThreadData;

UINT ThreadFunc(LPVOID lParam);
void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

//
// Initializes DirectSound
// - PCM driver initialization
// - wavetable format selection
// - starts multimedia timer
//
void InitWaveOut()
{
	gl_ThreadData.lpAudioBuffer = wb;
	gl_ThreadData.lpStartTime = &dwStartTime;

	memset(wb, 0x80, BUFFER_LENGTH);
	//memset(wb2, 0x80, BUFFER_LENGTH);

	memset(&sWfx, 0, sizeof(WAVEFORMATEX));
	sWfx.wFormatTag = WAVE_FORMAT_PCM;
	sWfx.nChannels = 1;
	sWfx.nSamplesPerSec = BUFFER_RATE;
	sWfx.nAvgBytesPerSec = BUFFER_RATE;
	sWfx.wBitsPerSample = 8;
	sWfx.nBlockAlign = 1;
	sWfx.cbSize = 0;

	sWhdr.dwBufferLength = BUFFER_LENGTH;

	//*sWhdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
	//sWhdr.dwFlags = WHDR_BEGINLOOP;
	//sWhdr.dwLoops = 0xffffffff;
	//*sWhdr.dwLoops = 1;

	sWhdr.dwFlags = 0L;
	sWhdr.dwLoops = 0L;

	sWhdr.lpData = (LPSTR)wb;

	waveOutOpen(&hWo, WAVE_MAPPER, &sWfx,
		(DWORD)waveOutProc, 0x12345, CALLBACK_FUNCTION);

	//for (int i =0; i < BUFFER_LENGTH; i++)
	  //  wb[i] = rand()%256;

	//*waveOutOpen( &hWo, WAVE_MAPPER, &sWfx, (DWORD)hMainWnd, 0L, CALLBACK_WINDOW );
	//waveOutOpen( &hWo, WAVE_MAPPER, &sWfx, NULL, 0L, WAVE_ALLOWSYNC );

	waveOutPrepareHeader(hWo, &sWhdr, sizeof(WAVEHDR));
	waveOutWrite(hWo, &sWhdr, sizeof(WAVEHDR));
	dwStartTime = ::GetTickCount();
	//*StartPlay();
	//*idTimer = timeSetEvent( 1000, 0, TimerProc, 0, TIME_PERIODIC );
}


//----------------------------------------------------------------------------;
//  waveOutProc
//----------------------------------------------------------------------------;
void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg == WOM_DONE)
	{
		//        waveOutPrepareHeader( hWo, &sWhdr, sizeof( WAVEHDR ) );
		//        waveOutWrite( hWo, &sWhdr, sizeof( WAVEHDR ) );
		dwStartTime = ::GetTickCount();
	}
}


//----------------------------------------------------------------------------;
//
//	void TermWaveOut
//
//	Leírás:
//		A PCM lejátszó driver lezárása, valamint a sebességmérõ multimédia
//		timer kilövése.
//
//----------------------------------------------------------------------------;

void TermWaveOut()
{
	StopPlay();
	waveOutUnprepareHeader(hWo, &sWhdr, sizeof(WAVEHDR));
	waveOutClose(hWo);
	//*timeKillEvent( idTimer );
}

void IncWaveCursor(BYTE b)
{
	/*	static int wc = 0;
		static int cnt = 0;

		dwEvents++;
		cnt++;

		if ( cnt < (int)dwSec )
			return;

		cnt = 0;

		wb[wc] = ( b & 16 ) ? 0x20 : 0xe0;
		wc++;

		if ( wc == BUFFER_LENGTH )
			wc = 0;*/

	DWORD dwTime = GetTickCount() - dwStartTime;
	DWORD dwMax = MulDiv(1000, BUFFER_LENGTH, BUFFER_RATE);
	if (dwTime > dwMax)
		return;

	int pos = MulDiv(BUFFER_LENGTH, dwTime, dwMax);
	if (pos >= BUFFER_LENGTH)
		return;

	//for (int i = iLastPos; i < pos; i++)
	  //  wb2[i] = wb2[iLastPos];

	wb2[pos] = (b & 16) ? 0x20 : 0xe0;
	iLastPos = pos;
}

void CALLBACK TimerProc(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
	dwSec = dwEvents / (BUFFER_RATE - 1000);
	dwEvents = 0;
}

void StartPlay()
{
	//waveOutPrepareHeader( hWo, &sWhdr, sizeof( WAVEHDR ) );
	//memcpy(wb, wb2, BUFFER_LENGTH);
	waveOutWrite(hWo, &sWhdr, sizeof(WAVEHDR));
	//dwStartTime = GetTickCount();
	//iLastPos = 0;
	//memset(wb2, 0x80, BUFFER_LENGTH);
}

void StopPlay()
{
	//    waveOutPause( hWo );
}

UINT ThreadFunc(LPVOID lParam)
{
	//THREADSTRUCT* pData = (THREADSTRUCT*)lParam;
	//DWORD dwPos;
	return 0;
}