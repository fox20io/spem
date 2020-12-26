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
#include "resource.h"
#include "Machine.h"
#include "spem.h"
#include "debugger.h"
#include "help.h"
#include "speaker.h"

extern CZ80 Cpu;
extern IDirectDrawSurface* pDDSurface;
extern IDirectDrawPalette* pDDPalette;
extern IDirectDrawSurface* pDDSBack;
extern LPDIRECTINPUTDEVICE8 pDIk;
extern int  DisplayMagnify;
extern BOOL fDisplayFullScreen;
extern BYTE KeyMatrix[8];
extern BOOL DebugMode;
extern BYTE* pMem;
extern BYTE* pOutp;
extern void (*lpVideoFunc)();
extern int dSpeed;
extern BOOL fInterlace;

// Globals
HINSTANCE hInst;								// current instance
TCHAR	szTitle[MAX_LOADSTRING];				// caption
TCHAR	szWindowClass[MAX_LOADSTRING];			// caption class
TCHAR	szCurrentDir[MAX_LOADSTRING];
TCHAR	szCurrentFile[MAX_LOADSTRING];
HWND	hMainWnd;
HWND	hKeyWnd;
BOOL	fDebugMode = FALSE;
BOOL	fEnableRun = TRUE;
BOOL	fBorder = TRUE;
BOOL	fKeyInfo = FALSE;
BOOL	isSound = TRUE;

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	AboutWndProc(HWND, UINT, WPARAM, LPARAM);
void				LoadSnapshot();
void				SaveSnapshot();
void				ChangeBorder();
void				BeginDebug();
void				SetFrameFresh(BOOL);
void				SetSpeedStyle(int);
void				SetMagnify(int);
void				SetKeyInfoState();
void				SetWindowTitle(CString);
void				SetSoundState();

//
//	Enty point of the application
//
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;
	InitContext();

	// Initialize global strings
	::LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	::LoadString(hInstance, IDC_SAMPLE, szWindowClass, MAX_LOADSTRING);
	::GetCurrentDirectory(MAX_LOADSTRING, szCurrentDir);

	MyRegisterClass(hInstance);

	// Application initialization
	if (!InitInstance(hInstance, nCmdShow))
		return FALSE;

	InitWaveOut();
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_ACCEL);

	for (;;)
	{
		if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if (fEnableRun)
			Operate();
	}

	TermDD();
	TermDI();

	if (isSound)
		TermWaveOut();

	return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_SAMPLE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = (LPCSTR)IDC_SAMPLE;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle,
		WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, 0, 0, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	hMainWnd = hWnd;

	// Set the size of the window which is 1x1 at startup, as well as, the window is always
	// apperas in the center of the screen.
	SetMagnify(DMODE_1X);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	if (InitDI())
	{
		Failure(IDS_FAIL_INITDI);
		return FALSE;
	}

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_ACTIVATE:
		if (wParam == WA_INACTIVE)
		{
			if ( pDIk )
                pDIk->Unacquire();
		}
		else
		{
			if ( pDIk )
                pDIk->Acquire();
		}
		break;

	case WM_SETCURSOR:
		if (fDisplayFullScreen)
			SetCursor(NULL);
		else
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;

	case WM_ENTERMENULOOP:
		if (isSound)
			StopPlay();
		break;

	case WM_EXITMENULOOP:
		if (isSound)
			StartPlay();
		break;

	case WM_SETFOCUS:
		if (!fDebugMode)
			fEnableRun = TRUE;

		if (fDisplayFullScreen)
			SetMagnify(DMODE_FULLSCREEN);
		break;

	case WM_KILLFOCUS:
		if (!fDebugMode)
			fEnableRun = FALSE;
		break;

	case WM_ACTIVATEAPP:
		if ((BOOL)wParam)
			SetMagnify(fDisplayFullScreen ? DMODE_FULLSCREEN : DisplayMagnify);
		break;

	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		if (wmEvent)
		{
			switch (wmId)
			{
			case ID_FILE_RESET:
				RandomMemory();
				Cpu.Reset();
				SetWindowTitle(_T(""));
				break;

			case ID_FILE_LOAD:
				if (!fDisplayFullScreen)
					LoadSnapshot();
				break;

			case ID_FILE_SAVEAS:
				if (!fDisplayFullScreen)
					SaveSnapshot();
				break;

			case ID_FILE_DEBUG:
				if (!fDisplayFullScreen)
					BeginDebug();
				break;

			case ID_OPTIONS_VIEW_1X1:
				SetMagnify(DMODE_1X);
				break;

			case ID_OPTIONS_VIEW_2X2:
				SetMagnify(DMODE_2X);
				break;

			case ID_TOGGLE_FULLSCREEN:
				SetMagnify(fDisplayFullScreen ? DMODE_1X : DMODE_FULLSCREEN);
				break;

			case ID_OPTIONS_VIEW_BORDER:
				ChangeBorder();
				break;

			case ID_FOCUSTOKINF:
				if (fKeyInfo && !fDisplayFullScreen)
					SetFocus(hKeyWnd);
				break;

			case ID_HELP_DESC:
				if (!fKeyInfo && !fDisplayFullScreen)
					SetKeyInfoState();
			}
		}
		else
		{
			switch (wmId)
			{
			case ID_FILE_LOAD:
				LoadSnapshot();
				break;

			case ID_FILE_SAVE:
				SaveSnapshot();
				break;

			case ID_FILE_RESET:
				RandomMemory();
				Cpu.Reset();
				SetWindowTitle(_T(""));
				break;

			case ID_FILE_DEBUG:
				BeginDebug();
				break;

			case ID_OPTIONS_VIEW_1X1:
				SetMagnify(DMODE_1X);
				break;

			case ID_OPTIONS_VIEW_2X2:
				SetMagnify(DMODE_2X);
				break;

			case ID_OPTIONS_VIEW_FULLSCREEN:
				SetMagnify(DMODE_FULLSCREEN);
				break;

			case ID_OPTIONS_VIEW_INTERLACE:
				SetFrameFresh(TRUE);
				break;

			case ID_OPTIONS_VIEW_NONINTERLACE:
				SetFrameFresh(FALSE);
				break;

			case ID_OPTIONS_VIEW_BORDER:
				ChangeBorder();
				break;

			case ID_OPTIONS_SPEED_REAL:
				SetSpeedStyle(SPEED_REAL);
				break;

			case ID_OPTIONS_SPEED_SYNC:
				SetSpeedStyle(SPEED_SYNCTOVIDEO);
				break;

			case ID_OPTIONS_SPEED_FULL:
				SetSpeedStyle(SPEED_FULL);
				break;

			case ID_OPTIONS_SPEEDTEST:
				SpeedTest();
				break;

			case ID_OPTIONS_SOUND:
				SetSoundState();
				break;

			case IDM_ABOUT:
				DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)AboutWndProc);
				break;

			case ID_HELP_DESC:
			{
				TCHAR str[MAX_LOADSTRING + 8];

				strcpy(str, szCurrentDir);
				strcat(str, _T("\\Spem.hlp"));
				WinHelp(hMainWnd, str, HELP_FINDER, 0);
				break;
			}

			case ID_HELP_KEYBOARD:
				SetKeyInfoState();
				break;

			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;

			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if (pDDSBack)
			DrawScreen();
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		TermDD();
		TermDI();
		if (isSound)
			TermWaveOut();
		PostQuitMessage(0);
		break;

	case MM_WOM_DONE:
		StartPlay();
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


LRESULT CALLBACK AboutWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}

	return FALSE;
}

void SetMagnify(int value)
{
	if (value == DMODE_FULLSCREEN)
		lpVideoFunc = VideoFullScreen;
	else
		lpVideoFunc = fBorder ? Video : VideoNoBorder;

	DisplayMagnify = (value == DMODE_1X) ? 1 : 2;
	fDisplayFullScreen = (value == DMODE_FULLSCREEN) ? TRUE : FALSE;

	SetMenu(hMainWnd, fDisplayFullScreen ? 0 :
		LoadMenu(hInst, MAKEINTRESOURCE(IDC_SAMPLE)));

	MENUITEMINFO minf;
	minf.cbSize = sizeof(MENUITEMINFO);
	minf.fMask = MIIM_STATE;

	minf.fState = (value == DMODE_1X) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_VIEW_1X1, FALSE, &minf);
	minf.fState = (value == DMODE_2X) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_VIEW_2X2, FALSE, &minf);

	minf.fState = (value == DMODE_FULLSCREEN) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_VIEW_FULLSCREEN, FALSE, &minf);

	minf.fState = (fBorder) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_VIEW_BORDER, FALSE, &minf);

	RECT rect, rdt;
	SIZE size;
	rect.top = rect.left = 0;
	if (fBorder)
	{
		rect.right = value * 320;
		rect.bottom = value * 240;
	}
	else
	{
		rect.right = value * 256;
		rect.bottom = value * 192;
	}

	if (!fDisplayFullScreen)
	{
		HWND hdtWnd = GetDesktopWindow();

		GetWindowRect(hdtWnd, &rdt);
		AdjustWindowRectEx(&rect, GetWindowLong(hMainWnd, GWL_STYLE), TRUE,
			GetWindowLong(hMainWnd, GWL_EXSTYLE));
		size.cx = rect.right - rect.left;
		size.cy = rect.bottom - rect.top;
		SetWindowPos(hMainWnd, HWND_NOTOPMOST, (rdt.right - size.cx) / 2,
			(rdt.bottom - size.cy) / 2, size.cx, size.cy,
			SWP_NOOWNERZORDER);
	}

	TermDD();

	if (!InitDD())
	{
		Failure(IDS_FAIL_INITDD);
		SendMessage(hMainWnd, WM_DESTROY, 0, 0);
	}
}

void ChangeBorder()
{
	fBorder = !fBorder;
	//if (fBorder) lpVideoFunc = VideoNoBorder;
	//else lpVideoFunc = Video;
	SetMagnify(DisplayMagnify);
}

void LoadSnapshot()
{
	StopPlay();

	TCHAR szFilter[MAX_LOADSTRING];
	LoadString(hInst, IDS_LOAD_FILTER, szFilter, MAX_LOADSTRING);
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		szFilter);
	if (dlg.DoModal() == IDOK)
	{
		CString s, stitle;
		s = dlg.m_pOFN->lpstrFile;
		SetWindowTitle(dlg.GetFileName());

		FILE* fp;
		fp = fopen(LPCTSTR(s), _T("rb"));

		if (fp == NULL)
		{
			Failure(IDS_FAIL_LOAD);
			return;
		}

		fseek(fp, 0, SEEK_END);
		long fsize = ftell(fp);

		if (fsize != 49179)
		{
			Failure(IDS_FAIL_LOADCORRUPT);
			return;
		}

		rewind(fp);

		REGISTERS* r = Cpu.GetRegs();

		r->I = fgetc(fp);
		r->_L = fgetc(fp);
		r->_H = fgetc(fp);
		r->_E = fgetc(fp);
		r->_D = fgetc(fp);
		r->_C = fgetc(fp);
		r->_B = fgetc(fp);
		r->_F = fgetc(fp);
		r->_A = fgetc(fp);
		r->L = fgetc(fp);
		r->H = fgetc(fp);
		r->E = fgetc(fp);
		r->D = fgetc(fp);
		r->C = fgetc(fp);
		r->B = fgetc(fp);
		r->IY_L = fgetc(fp);
		r->IY_H = fgetc(fp);
		r->IX_L = fgetc(fp);
		r->IX_H = fgetc(fp);

		if (fgetc(fp) & 0x02)
			Cpu.m_IFF1 = Cpu.m_IFF2 = TRUE;
		else
			Cpu.m_IFF1 = Cpu.m_IFF2 = FALSE;

		r->R = fgetc(fp);
		r->F = fgetc(fp);
		r->A = fgetc(fp);
		r->SP = fgetc(fp);
		r->SP |= fgetc(fp) << 8;
		Cpu.m_IM = fgetc(fp);

		// A keret színe
		*(pOutp + 0xfe) = fgetc(fp);
		fread((pMem + 0x4000), 49152, 1, fp);

		// The value of the PC register has been stacked before save; therefore, it can be easely read
		// from the stack at this point.
		r->PC = *(pMem + (r->SP));
		r->SP++;
		r->PC |= *(pMem + (r->SP)) << 8;
		r->SP++;

		fclose(fp);
	}

	if (isSound)
		StartPlay();

	return;
}

void SaveSnapshot()
{
	StopPlay();

	TCHAR szFilter[MAX_LOADSTRING];
	LoadString(hInst, IDS_SAVE_FILTER, szFilter, MAX_LOADSTRING);
	CFileDialog dlg(FALSE, _T("sna"), _T("noname"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);

	if (dlg.DoModal() == IDOK)
	{
		CString s;
		s = dlg.GetFileName();
		SetWindowTitle(s);

		FILE* fp;
		fp = fopen(LPCTSTR(s), _T("wb"));
		if (fp == NULL)
		{
			Failure(IDS_FAIL_LOAD);
			return;
		}

		REGISTERS* r = Cpu.GetRegs();

		fputc(r->I, fp);
		fputc(r->_L, fp);
		fputc(r->_H, fp);
		fputc(r->_E, fp);
		fputc(r->_D, fp);
		fputc(r->_C, fp);
		fputc(r->_B, fp);
		fputc(r->_F, fp);
		fputc(r->_A, fp);
		fputc(r->L, fp);
		fputc(r->H, fp);
		fputc(r->E, fp);
		fputc(r->D, fp);
		fputc(r->C, fp);
		fputc(r->B, fp);
		fputc(r->IY_L, fp);
		fputc(r->IY_H, fp);
		fputc(r->IX_L, fp);
		fputc(r->IX_H, fp);

		if (Cpu.m_IFF1)
			fputc(0x02, fp);
		else
			fputc(0, fp);

		fputc(r->R, fp);
		fputc(r->F, fp);
		fputc(r->A, fp);

		// The value of the PC register has to be saved into the stack in order the preserve it for recovering the original state.
		WORD sp = r->SP;

		r->SP--;
		*(pMem + r->SP) = r->PC / 256;
		r->SP--;
		*(pMem + r->SP) = r->PC % 256;

		fputc(r->SP % 256, fp);
		fputc(r->SP / 256, fp);
		r->SP = sp;

		fputc(Cpu.m_IM, fp);
		// Colour of the border
		fputc(*(pOutp + 0xfe), fp);
		fwrite((pMem + 0x4000), 49152, 1, fp);

		fclose(fp);
	}

	if (isSound)
		StartPlay();
}

//
//	Common error massage handler
//
void Failure(int textid)
{
	TCHAR szCaption[MAX_LOADSTRING];
	TCHAR szText[MAX_LOADSTRING];
	LoadString(hInst, IDS_APP_TITLE, szCaption, MAX_LOADSTRING);
	LoadString(hInst, textid, szText, MAX_LOADSTRING);
	MessageBox(hMainWnd, szText, szCaption, MB_OK | MB_ICONEXCLAMATION);
}

void BeginDebug()
{
	fDebugMode = TRUE;
	fEnableRun = FALSE;
	HWND hWndDebug;
	hWndDebug = CreateDialog(hInst, (LPCTSTR)IDD_DEBUG, hMainWnd, (DLGPROC)DebuggerWndProc);
	ShowWindow(hWndDebug, SW_SHOW);
}

//
//	Switch between interlaced and non-interlaced modes
//
void SetFrameFresh(BOOL value)
{
	fInterlace = value;

	MENUITEMINFO minf;
	minf.cbSize = sizeof(MENUITEMINFO);
	minf.fMask = MIIM_STATE;

	minf.fState = value ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_VIEW_INTERLACE, FALSE, &minf);
	minf.fState = value ? MFS_UNCHECKED : MFS_CHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_VIEW_NONINTERLACE, FALSE, &minf);
}

//
//	Managing the state of menu items due to the current speed settings
//
void SetSpeedStyle(int speed)
{
	dSpeed = speed;
	MENUITEMINFO minf;
	minf.cbSize = sizeof(MENUITEMINFO);
	minf.fMask = MIIM_STATE;

	minf.fState = (speed == SPEED_REAL) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_SPEED_REAL, FALSE, &minf);
	minf.fState = (speed == SPEED_SYNCTOVIDEO) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_SPEED_SYNC, FALSE, &minf);
	minf.fState = (speed == SPEED_FULL) ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_SPEED_FULL, FALSE, &minf);
}

//
// Managing the keyboard assistant window
//
void SetKeyInfoState()
{
	fKeyInfo = !fKeyInfo;
	MENUITEMINFO minf;
	minf.cbSize = sizeof(MENUITEMINFO);
	minf.fMask = MIIM_STATE;
	minf.fState = fKeyInfo ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_HELP_KEYBOARD, FALSE, &minf);

	if (fKeyInfo)
		HelpKeyboard();
	else
		DestroyWindow(hKeyWnd);
}

//
// Appends the specified string to the title of the main window
//
void SetWindowTitle(CString s)
{
	CString str;

	if (!s.IsEmpty())
	{
		int r = s.Find('.');
		if (r != -1)
			str = s.Left(r);
		else
			str = s;

		str += _T(" - ");
	}

	str += szTitle;
	SetWindowText(hMainWnd, str);
}

//
// Manages the sound on/off option.
//
void SetSoundState()
{
	isSound = isSound ? FALSE : TRUE;
	MENUITEMINFO minf;
	minf.cbSize = sizeof(MENUITEMINFO);
	minf.fMask = MIIM_STATE;
	minf.fState = isSound ? MFS_CHECKED : MFS_UNCHECKED;
	SetMenuItemInfo(GetMenu(hMainWnd), ID_OPTIONS_SOUND, FALSE, &minf);

	if (isSound)
		InitWaveOut();
	else
		TermWaveOut();
}