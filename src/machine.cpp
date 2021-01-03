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
#include "Machine.h"
#include "resource.h"
#include "speaker.h"

CZ80			Cpu;
Speaker			ZxSpeaker;
BYTE* pMem = NULL;
BYTE* pInp = NULL;
BYTE* pOutp = NULL;
DWORD			temp;
void			(*lpVideoFunc)() = Video;
int				dSpeed = SPEED_SYNCTOVIDEO;
BOOL			fInterlace = TRUE;
int				GL_InstructionsPerEmulatorLoop = CPU_REAL_TICKS;

THREADSTRUCT gl_ThreadData = { NULL, NULL, NULL };

extern BYTE*    pVMem;
extern BYTE		KeyMatrix[8];
extern HWND		hMainWnd;
extern IDirectDraw* pDD;
extern HINSTANCE hInst;

void InitContext()
{
	//	Allocating the memory and periphery space for the ZX Spectrum
	pMem = new BYTE[0x10000];
	pInp = new BYTE[0x100];
	pOutp = new BYTE[0x100];

	// CPU initialization
	INITCPU iCPU;
	iCPU.MMask = 0xffffff00;
	iCPU.pMem = pMem;
	iCPU.pInp = pInp;
	iCPU.pOutp = pOutp;

	Cpu.SetCPUProp(iCPU);

	pVMem = pMem + 0x4000;

	// Line address cache generation
	BuildAccelTables();

	// Loading the Spectrum ROM program
	FILE* fp = NULL;
	fp = fopen(_T("spect.rom"), _T("rb"));
	if (fp != NULL)
	{
		fread(pMem, 16384, 1, fp);
		fclose(fp);
	}
	else
	{
		// Loading the ROM program from internal resource
		HRSRC hrc = ::FindResource(hInst, MAKEINTRESOURCE(IDR_ROM), TEXT("ROM"));
		if (hrc == NULL)
		{
			::MessageBox(NULL, _T("Fatal error! The emulator cannot be started."), _T("Error"), MB_OK);
			::PostQuitMessage(-1);
		}

		HANDLE hMem = ::LoadResource(hInst, hrc);
		LPVOID lpRom = ::LockResource(hMem);
		//DWORD dwSize = ::SizeofResource( hInst, hrc );
		//ASSERT( dwSize == 0x4000 );
		memcpy(pMem, lpRom, 0x4000);
	}

	RandomMemory();
	Cpu.Reset();

	gl_ThreadData.lpSpeaker = (pOutp + 0xfe);
	//    AfxBeginThread( ThreadFunc, (LPVOID)&gl_ThreadData );

	GL_InstructionsPerEmulatorLoop = ((double)CPU_INT_TIMER / 1000.0) / (CPU_AVG_MCYCLES / (double)CPU_FREQUENCY_HZ);
}

//
// The heart beat of the ZX Spectrum.
// Called from the main message loop.
//
void Operate()
{
	static DWORD lTC_df = 0;
	static BOOL divider = FALSE;

	switch (dSpeed)
	{
	case SPEED_REAL:
	{
		DWORD tTC;

		tTC = GetTickCount();
		if ((tTC - lTC_df) >= CPU_INT_TIMER)
		{
			lTC_df = tTC;
			divider = !divider;

			ZxSpeaker.CreateBuffer(GL_InstructionsPerEmulatorLoop);

			//for (int i = 0; i < CPU_REAL_TICKS; i++)
			for (int i = 0; i < GL_InstructionsPerEmulatorLoop; i++)
			{
				Cpu.Run();

				if (Cpu.m_HIBYTEIN != 0x8000)
				{
					// The CPU starts to execute an IN instruction a this point; therefore, we need to
					// refresh keyboard states prior to it
					BuildKeyMatrix();
					BYTE rows = ~(Cpu.m_HIBYTEIN & 0xff);
					BYTE col = 0;

					for (int i = 0; i < 8; i++)
						if ((rows >> i) & 1)
							col |= KeyMatrix[i];

					*(pInp + 0xfe) = ~col;
				}

				ZxSpeaker.WriteNextBufferBit(*(pOutp + 0xfe) & 16 ? TRUE : FALSE);
			}

			ZxSpeaker.ApplyBuffer(CPU_INT_TIMER);

			if (fInterlace)
			{
				if (divider)
					lpVideoFunc();
			}
			else
				lpVideoFunc();

			Cpu.Int();
		}

		break;
	}

	case SPEED_SYNCTOVIDEO:
	{
		pDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
		divider = !divider;

		ZxSpeaker.CreateBuffer(GL_InstructionsPerEmulatorLoop);  // consider!

		for (int i = 0; i < CPU_REAL_TICKS; i++)
		{
			Cpu.Run();

			if (Cpu.m_HIBYTEIN != 0x8000)
			{
				// The CPU starts to execute an IN instruction a this point; therefore, we need to
				// refresh keyboard states prior to it
				BuildKeyMatrix();
				BYTE rows = ~(Cpu.m_HIBYTEIN & 0xff);
				BYTE col = 0;

				for (int i = 0; i < 8; i++)
					if ((rows >> i) & 1)
						col |= KeyMatrix[i];

				*(pInp + 0xfe) = ~col;
			}

			ZxSpeaker.WriteNextBufferBit(*(pOutp + 0xfe) & 16 ? TRUE : FALSE);
		}

		ZxSpeaker.ApplyBuffer(CPU_INT_TIMER);

		if (fInterlace)
		{
			if (divider)
				lpVideoFunc();
		}
		else
		{
			lpVideoFunc();
		}

		Cpu.Int();
		break;
	}

	case SPEED_FULL:
	{
		ZxSpeaker.CreateBuffer(GL_InstructionsPerEmulatorLoop); // consider!

		for (int i = 0; i < CPU_IDLE_TICKS; i++)
		{
			Cpu.Run();

			if (Cpu.m_HIBYTEIN != 0x8000)
			{
				// The CPU starts to execute an IN instruction a this point; therefore, we need to
				// refresh keyboard states prior to it
				BuildKeyMatrix();
				BYTE rows = ~(Cpu.m_HIBYTEIN & 0xff);
				BYTE col = 0;

				for (int i = 0; i < 8; i++)
					if ((rows >> i) & 1)
						col |= KeyMatrix[i];

				*(pInp + 0xfe) = ~col;
			}

			ZxSpeaker.WriteNextBufferBit(*(pOutp + 0xfe) & 16 ? TRUE : FALSE);
		}

		ZxSpeaker.ApplyBuffer(CPU_INT_TIMER);

		DWORD tTC;

		tTC = GetTickCount();

		if ((tTC - lTC_df) >= CPU_INT_TIMER)
		{
			lTC_df = tTC;
			divider = !divider;
			if (fInterlace)
			{
				if (divider)
					lpVideoFunc();
			}
			else
				lpVideoFunc();

			Cpu.Int();
		}
	}
	}
}

//
// Fills the video memory with random data in order to simulate the real startup visual pattern.
//
void RandomMemory()
{
	for (WORD i = 16384; i < 23296; i++)
		*(pMem + i) = rand() % 256;
}


//
// This algorithm manages the video speed test which measures the frame rate.
//
// The result is 80 percent CPU dependent (2000) and it could influence by the
// apllied video options, as well. For instance, using border and 2x2 resolution can
// add extra resource costs.
// Measurement results using a Celeron 466 MHz CPU:
//		# border on:	1X1 = 243.90 fps;	2X2 = 166.39 fps.
//		# border off:	1X1 = 303.03 fps;	2X2 = 221.73 fps.

#define NUMS 100

void SpeedTest()
{
	DWORD t1;
	DWORD t2;
	TCHAR sBuff[100];

	t1 = GetTickCount();

	for (int i = 0; i < NUMS; i++)
		lpVideoFunc();

	t2 = GetTickCount();

	sprintf(sBuff, _T("The refresh rate is: %.2f fps"),
		float(NUMS * 1000) / float(t2 - t1));

	::MessageBox(hMainWnd, sBuff, _T("Video speed test"), MB_OK);
}
