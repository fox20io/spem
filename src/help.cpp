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
#include "help.h"
#include "spem.h"

extern HINSTANCE hInst;
extern HWND hMainWnd;
extern HWND hKeyWnd;
extern BOOL fKeyInfo;

#define KHELPNUM 148

//
// Map of keyboard symbols and their key combinations can be used to activate them
//
KEYHELPITEMS aKHelp[KHELPNUM] =
{

	// BASIC keywords that can be accessed using the [K] cursor
	{ 0, "PLOT", "[K]+Q" }, { 0, "DRAW", "[K]+W" }, { 0, "REM", "[K]+E" },
	{ 0, "RUN", "[K]+R" }, { 0, "RAND", "[K]+T" }, { 0, "RETURN", "[K]+Y" },
	{ 0, "IF", "[K]+U" }, { 0, "INPUT", "[K]+I" }, { 0, "POKE", "[K]+O" },
	{ 0, "PRINT", "[K]+P" }, { 0, "NEW", "[K]+A" }, { 0, "SAVE", "[K]+S" },
	{ 0, "DIM", "[K]+D" }, { 0, "FOR", "[K]+F" }, { 0, "GOTO", "[K]+G" },
	{ 0, "GOSUB", "[K]+H" }, { 0, "LOAD", "[K]+J" }, { 0, "LIST", "[K]+K" },
	{ 0, "LET", "[K]+L" }, { 0, "COPY", "[K]+Z" }, { 0, "CLEAR", "[K]+X" },
	{ 0, "CONTINUE", "[K]+C" }, { 0, "CLS", "[K]+V" }, { 0, "BORDER", "[K]+B" },
	{ 0, "NEXT", "[K]+N" }, { 0, "PAUSE", "[K]+M" },

	// Keywords that can be accessed using the [E]+SYMBOL SHIFT combination
	{ 0, "DEF FN", "[E]+SYMBOL SHIFT+1" },
	{ 0, "FN", "[E]+SYMBOL SHIFT+2" }, { 0, "LINE", "[E]+SYMBOL SHIFT+3" },
	{ 0, "OPEN#", "[E]+SYMBOL SHIFT+4" }, { 0, "CLOSE#", "[E]+SYMBOL SHIFT+5" },
	{ 0, "MOVE", "[E]+SYMBOL SHIFT+6" }, { 0, "ERASE", "[E]+SYMBOL SHIFT+7" },
	{ 0, "POINT", "[E]+SYMBOL SHIFT+8" }, { 0, "CAT", "[E]+SYMBOL SHIFT+9" },
	{ 0, "FORMAT", "[E]+SYMBOL SHIFT+0" }, { 0, "ASN", "[E]+SYMBOL SHIFT+Q" },
	{ 0, "ACS", "[E]+SYMBOL SHIFT+W" }, { 0, "ATN", "[E]+SYMBOL SHIFT+E" },
	{ 0, "VERIFY", "[E]+SYMBOL SHIFT+R" }, { 0, "MERGE", "[E]+SYMBOL SHIFT+T" },
	{ 0, "IN", "[E]+SYMBOL SHIFT+I" }, { 0, "OUT", "[E]+SYMBOL SHIFT+O" },
	{ 0, "CIRCLE", "[E]+SYMBOL SHIFT+H" }, { 0, "VAL$", "[E]+SYMBOL SHIFT+J" },
	{ 0, "SCREEN$", "[E]+SYMBOL SHIFT+K" }, { 0, "ATTR", "[E]+SYMBOL SHIFT+L" },
	{ 0, "BEEP", "[E]+SYMBOL SHIFT+Z" }, { 0, "INK", "[E]+SYMBOL SHIFT+X" },
	{ 0, "PAPER", "[E]+SYMBOL SHIFT+C" }, { 0, "FLASH", "[E]+SYMBOL SHIFT+V" },
	{ 0, "BRIGHT", "[E]+SYMBOL SHIFT+B" }, { 0, "OVER", "[E]+SYMBOL SHIFT+N" },
	{ 0, "INVERSE", "[E]+SYMBOL SHIFT+M" },

	// Keywords that can be accessed using the [E] cursor
	{ 0, "SIN", "[E]+Q" }, { 0, "COS", "[E]+W" }, { 0, "TAN", "[E]+E" },
	{ 0, "INT", "[E]+R" }, { 0, "RND", "[E]+T" }, { 0, "STR$", "[E]+Y" },
	{ 0, "CHR$", "[E]+U" }, { 0, "CODE", "[E]+I" }, { 0, "PEEK", "[E]+O" },
	{ 0, "TAB", "[E]+P" }, { 0, "READ", "[E]+A" }, { 0, "RESTORE", "[E]+S" },
	{ 0, "DATA", "[E]+D" }, { 0, "SGN", "[E]+F" }, { 0, "ABS", "[E]+G" },
	{ 0, "SQR", "[E]+H" }, { 0, "VAL", "[E]+J" }, { 0, "LEN", "[E]+K" },
	{ 0, "USR", "[E]+L" }, { 0, "LN", "[E]+Z" }, { 0, "EXP", "[E]+X" },
	{ 0, "LPRINT", "[E]+C" }, { 0, "LLIST", "[E]+V" }, { 0, "BIN", "[E]+B" },
	{ 0, "INKEY$", "[E]+N" }, { 0, "PI", "[E]+M" },

	// Keywords that can be accessed by using SYMBOL SHIFT
	{ 0, "AND", "SYMBOL SHIFT+Y" }, { 0, "OR", "SYMBOL SHIFT+U" },
	{ 0, "AT", "SYMBOL SHIFT+I" }, { 0, "STOP", "SYMBOL SHIFT+A" },
	{ 0, "NOT", "SYMBOL SHIFT+S" }, { 0, "TO", "SYMBOL SHIFT+F" },
	{ 0, "THEN", "SYMBOL SHIFT+G" }, { 0, "STEP", "SYMBOL SHIFT+D" },

	// Accessing function keys
	{ 2, "[CAPS SHIFT]", "Left Shift" }, { 2, "[SYMBOL SHIFT]", "/" },
	{ 2, "[SPACE]", "Space" }, { 2, "[BREAK]", "CAPS SHIFT+SPACE" },
	{ 2, "[ENTER]", "Enter" }, { 2, "[DELETE]", "CAPS SHIFT+0" },
	{ 2, "[EDIT]", "CAPS SHIFT+1" }, { 2, "[CAPS LOCK]", "CAPS SHIFT+2" },
	{ 2, "[TRUE VIDEO]", "CAPS SHIFT+3" }, { 2, "[INV. VIDEO]", "CAPS SHIFT+4" },
	{ 2, "[LEFT]", "CAPS SHIFT+5" }, { 2, "[DOWN]", "CAPS SHIFT+6" },
	{ 2, "[UP]", "CAPS SHIFT+7" },  { 2, "[RIGHT]", "CAPS SHIFT+8" },
	{ 2, "[GRAPHICS] = [G]", "CAPS SHIFT+9" }, { 2, "[E]", "CAPS SHIFT+SYMBOL SHIFT" },

	//	Characters
	{ 1, "!", "SYMBOL SHIFT+1" }, { 1, "@", "SYMBOL SHIFT+2" },
	{ 1, "#", "SYMBOL SHIFT+3" }, { 1, "$", "SYMBOL SHIFT+4" },
	{ 1, "%", "SYMBOL SHIFT+5" }, { 1, "&", "SYMBOL SHIFT+6" },
	{ 1, "'", "SYMBOL SHIFT+7" }, { 1, "(", "SYMBOL SHIFT+8" },
	{ 1, ")", "SYMBOL SHIFT+9" }, { 1, "_", "SYMBOL SHIFT+0" },
	{ 1, "<=", "SYMBOL SHIFT+Q" }, { 1, "<>", "SYMBOL SHIFT+W" },
	{ 1, ">=", "SYMBOL SHIFT+E" }, { 1, "<", "SYMBOL SHIFT+R" },
	{ 1, ">", "SYMBOL SHIFT+T" }, { 1, ";", "SYMBOL SHIFT+O" },
	{ 1, "\"", "SYMBOL SHIFT+P" }, { 1, "^", "SYMBOL SHIFT+H" },
	{ 1, "-", "SYMBOL SHIFT+J" }, { 1, "+", "SYMBOL SHIFT+K" },
	{ 1, "=", "SYMBOL SHIFT+L" }, { 1, ":", "SYMBOL SHIFT+A" },
	{ 1, "£", "SYMBOL SHIFT+X" }, { 1, "?", "SYMBOL SHIFT+C" },
	{ 1, "/", "SYMBOL SHIFT+V" }, { 1, "*", "SYMBOL SHIFT+B" },
	{ 1, ",", "SYMBOL SHIFT+N" }, { 1, ".", "SYMBOL SHIFT+M" },
	{ 1, "[", "[E]+SYMBOL SHIFT+Y" }, { 1, "]", "[E]+SYMBOL SHIFT+U" },
	{ 1, "(c) copyright", "[E]+SYMBOL SHIFT+P" }, { 1, "~", "[E]+SYMBOL SHIFT+A" },
	{ 1, "|", "[E]+SYMBOL SHIFT+S" }, { 1, "\\", "[E]+SYMBOL SHIFT+D" },
	{ 1, "{", "[E]+SYMBOL SHIFT+F" }, { 1, "}", "[E]+SYMBOL SHIFT+G" },

	// Accessing colours
	{ 3, "Blue", "[TRUE VIDEO on]+CAPS SHIFT+1" },
	{ 3, "Red", "[TRUE VIDEO on]+CAPS SHIFT+2" },
	{ 3, "Magenta", "[TRUE VIDEO on]+CAPS SHIFT+3" },
	{ 3, "Green", "[TRUE VIDEO on]+CAPS SHIFT+4" },
	{ 3, "Cyan", "[TRUE VIDEO on]+CAPS SHIFT+5" },
	{ 3, "Yellow", "[TRUE VIDEO on]+CAPS SHIFT+6" },
	{ 3, "White", "[TRUE VIDEO on]+CAPS SHIFT+7" },
	{ 3, "Black", "[TRUE VIDEO on]+CAPS SHIFT+0" }
};

LRESULT CALLBACK HelpKeyboardWndProc(HWND, UINT, WPARAM, LPARAM);
void LoadList(HWND, int);

void HelpKeyboard()
{
	hKeyWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_KEYBOARD), hMainWnd,
		(DLGPROC)HelpKeyboardWndProc);
	ShowWindow(hKeyWnd, SW_SHOW);
	return;
}

LRESULT CALLBACK HelpKeyboardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int CurCatSel = 4;	// the current category
	HWND hw;
	int i, wmId, wmEvent;
	char str[MAX_LOADSTRING];

	switch (msg) {
	case WM_INITDIALOG:

		// Loading the content of the combobox and listbox from resources determined by the current category
		hw = GetDlgItem(hwnd, IDC_COMBO_CAT);
		LoadString(hInst, IDS_CATEGORIES2, str, MAX_LOADSTRING);
		SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)str);
		LoadString(hInst, IDS_CATEGORIES3, str, MAX_LOADSTRING);
		SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)str);
		LoadString(hInst, IDS_CATEGORIES4, str, MAX_LOADSTRING);
		SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)str);
		LoadString(hInst, IDS_CATEGORIES5, str, MAX_LOADSTRING);
		SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)str);
		LoadString(hInst, IDS_CATEGORIES, str, MAX_LOADSTRING);
		SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)str);

		SendMessage(hw, CB_SETCURSEL, CurCatSel, 0);
		LoadList(GetDlgItem(hwnd, IDC_LIST_ITEMS), CurCatSel);
		break;

	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmEvent)
		{
		case CBN_SELCHANGE:
			switch ((int)wmId)
			{
			case IDC_COMBO_CAT:
				CurCatSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				LoadList(GetDlgItem(hwnd, IDC_LIST_ITEMS), CurCatSel);
				SetWindowText(GetDlgItem(hwnd, IDC_STATIC_DESC), "");
				break;
			case IDC_LIST_ITEMS:
				i = SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
				SendMessage((HWND)lParam, LB_GETTEXT, i, (LPARAM)(LPCSTR)str);
				for (i = 0; i < KHELPNUM; i++)
					if (strcmp(aKHelp[i].string, str) == 0) break;
				SetWindowText(GetDlgItem(hwnd, IDC_STATIC_DESC), aKHelp[i].keycomb);
			}
			break;
		default:
		{
			if (wmId == IDOK || wmId == IDCANCEL)
			{
				EndDialog(hwnd, wmId);
				fKeyInfo = FALSE;
				MENUITEMINFO minf;
				minf.cbSize = sizeof(MENUITEMINFO);
				minf.fMask = MIIM_STATE;
				minf.fState = MFS_UNCHECKED;
				SetMenuItemInfo(GetMenu(hMainWnd), ID_HELP_KEYBOARD, FALSE, &minf);
				return TRUE;
			}
		}
		}
	}

	return FALSE;
}

void LoadList(HWND hwnd, int cat)
{
	SendMessage(hwnd, LB_RESETCONTENT, 0, 0);
	for (int i = 0; i < KHELPNUM; i++)
	{
		if (cat == 4)
		{
			SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)aKHelp[i].string);
		}
		else
		{
			if (aKHelp[i].cat == cat)
				SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)aKHelp[i].string);
		}
	}
}
