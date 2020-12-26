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
#include "debugger.h"
#include "resource.h"
#include "z80.h"
#include "z80dis.h"
#include "Ula.h"
#include "spem.h"

extern BOOL fDebugMode;
extern BOOL fEnableRun;
extern CZ80 Cpu;
extern BYTE* pMem;
extern HINSTANCE hInst;

WORD AddrTable[MAX_LINES];
WORD DisAddr;
WORD DisMemAddr;

//
// Window procedure of the Dissassembler window.
//
LRESULT CALLBACK DebuggerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int i;
    
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        fDebugMode = FALSE;
        fEnableRun = TRUE;
        return TRUE;

    case WM_INITDIALOG:
    {
        // Set fixed size fonts in listboxes
        HDC hDC = GetDC(hWnd);
        HFONT Font = CreateFont(0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Fixedsys");
        ReleaseDC(hWnd, hDC);
        SendMessage(GetDlgItem(hWnd, IDC_LIST), WM_SETFONT, (WPARAM)Font, MAKELPARAM(FALSE, 0));
        SendMessage(GetDlgItem(hWnd, IDC_LIST_MEM), WM_SETFONT, (WPARAM)Font, MAKELPARAM(FALSE, 0));

        for (i = 0; i < MAX_LINES; i++)
            AddrTable[i] = 0;
        DisAddr = Cpu.GetRegs()->PC;
        LoadRegisters(hWnd);
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCLOSE:
            DestroyWindow(hWnd);
            fDebugMode = FALSE;
            fEnableRun = TRUE;
            return TRUE;

        case IDSTEP:
        {
            // Execution of the next instruction, redrawing the screen, refreshing the registers and flags.
            Cpu.Run();
            for (int i = 0; i < 240; i++)
                DrawScreen();
            DisAddr = Cpu.GetRegs()->PC;
            LoadRegisters(hWnd);
            break;
        }

        case IDJUMP:
            if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_JUMP), hWnd, (DLGPROC)JumpWndProc, 0))
            {
                LoadRegisters(hWnd);
            }

        case IDINT:
            Cpu.Int();
            break;

        case IDPOKE:
            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_POKE), hWnd, (DLGPROC)PokeWndProc, 0);
            break;

        case IDGOTO:
            if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_JUMP), hWnd, (DLGPROC)JumpWndProc, 1))
            {
                LoadRegisters(hWnd);
            }
        }
    }

    return FALSE;
}

//
// Window procedure of the Jump dialog window
//
LRESULT CALLBACK JumpWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int operate;
    switch (msg)
    {
    case WM_INITDIALOG:
        operate = lParam;
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            char s[11], * e;
            WORD addr;
            long num;
            GetWindowText(GetDlgItem(hWnd, IDC_EDIT_JUMP), s, 10);
            num = strtol(s, &e, 16);
            if (*e || num > 0xffff || num < 0 || !strlen(s))
            {
                Failure(IDS_FAIL_WINPFIELD);
                return TRUE;
            }
            addr = (WORD)num;
            if (operate)
                DisMemAddr = addr;
            else
                DisAddr = addr;
            EndDialog(hWnd, 1);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hWnd, 0);
            return TRUE;
        }
    }
    return FALSE;
}

void LoadRegisters(HWND hWnd)
{
    static WORD prevMem = 0xffff;
    REGISTERS* r = Cpu.GetRegs();
    char s[33];

    // Display the content of registers
    sprintf(s, "%.4X", r->C + 256 * r->B);
    SetWindowText(GetDlgItem(hWnd, IDC_REGBC), s);
    sprintf(s, "%.4X", r->E + 256 * r->D);
    SetWindowText(GetDlgItem(hWnd, IDC_REGDE), s);
    sprintf(s, "%.4X", r->L + 256 * r->H);
    SetWindowText(GetDlgItem(hWnd, IDC_REGHL), s);
    sprintf(s, "%.4X", r->F + 256 * r->A);
    SetWindowText(GetDlgItem(hWnd, IDC_REGAF), s);
    sprintf(s, "%.4X", r->_C + 256 * r->_B);
    SetWindowText(GetDlgItem(hWnd, IDC_REGBC2), s);
    sprintf(s, "%.4X", r->_E + 256 * r->_D);
    SetWindowText(GetDlgItem(hWnd, IDC_REGDE2), s);
    sprintf(s, "%.4X", r->_L + 256 * r->_H);
    SetWindowText(GetDlgItem(hWnd, IDC_REGHL2), s);
    sprintf(s, "%.4X", r->_F + 256 * r->_A);
    SetWindowText(GetDlgItem(hWnd, IDC_REGAF2), s);
    sprintf(s, "%.4X", r->IX_L + 256 * r->IX_H);
    SetWindowText(GetDlgItem(hWnd, IDC_REGIX), s);
    sprintf(s, "%.4X", r->IY_L + 256 * r->IY_H);
    SetWindowText(GetDlgItem(hWnd, IDC_REGIY), s);
    sprintf(s, "%.4X", r->PC);
    SetWindowText(GetDlgItem(hWnd, IDC_REGPC), s);
    sprintf(s, "%.4X", r->SP);
    SetWindowText(GetDlgItem(hWnd, IDC_REGSP), s);
    sprintf(s, "%.2X", r->I);
    SetWindowText(GetDlgItem(hWnd, IDC_REGI), s);
    sprintf(s, "%.2X", r->R);
    SetWindowText(GetDlgItem(hWnd, IDC_REGR), s);

    // Displaying the interrupt mode, IFF content and the HALT state
    sprintf(s, "%X", Cpu.m_IM);
    SetWindowText(GetDlgItem(hWnd, IDC_IM), s);
    sprintf(s, "%X", Cpu.m_IFF2);
    SetWindowText(GetDlgItem(hWnd, IDC_IFF), s);
    EnableWindow(GetDlgItem(hWnd, IDC_HALT), Cpu.m_Halt);

    //	Display the bits of the FLAG register
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_S), BM_SETCHECK, (r->F & 0x80) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_Z), BM_SETCHECK, (r->F & 0x40) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_H), BM_SETCHECK, (r->F & 0x10) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_PV), BM_SETCHECK, (r->F & 0x04) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_N), BM_SETCHECK, (r->F & 0x02) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_C), BM_SETCHECK, (r->F & 0x01) ? BST_CHECKED : BST_UNCHECKED, 0);

    // Disassembling the memory content starting from DisAddr.
    // If the content of the starting address has already been contained by the disassembled list then only
    // item selection is needed, otherwise the current content of the list is clearing and regenerating from the
    // new address.
    CZ80Dis dis(pMem);
    WORD Addr = DisAddr;

    // Try to find the address in the list.
    for (int line = 0; line < MAX_LINES; line++)
    {
        if (AddrTable[line] == Addr)
        {
            SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_SETCURSEL, line, 0);
            goto refreshmem;
        }
    }

    // Regenerating the whole list and selecting the first item.
    SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_RESETCONTENT, 0, 0);
    int line = 0;
    while (line < MAX_LINES)
    {
        AddrTable[line] = Addr;
        Addr += dis.BuildLine(Addr);
        SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_ADDSTRING, 0, LPARAM(dis.m_szLine));
        line++;
    }
    SendMessage(GetDlgItem(hWnd, IDC_LIST), LB_SETCURSEL, 0, 0);

refreshmem:

    // Loading memory content view.

    // Clearing the current content.
    SendMessage(GetDlgItem(hWnd, IDC_LIST_MEM), LB_RESETCONTENT, 0, 0);
    int max;

    // Normalize too big addresses given by the user.
    if (65535 - 800 < DisMemAddr)
        max = 65535 - DisMemAddr; else max = 800;

    for (line = 0; line < max; line += 8)
    {
        char s[100];
        char s2[100];

        // Compose memory address
        sprintf(s, "%.4X ", DisMemAddr + line);
        for (int k = 0; k < 8; k++)
        {
            sprintf(s2, "%.2X ", *(pMem + DisMemAddr + line + k));
            strcat(s, s2);
        }
        strcat(s, " ");

        // 8 bytes in one row, row generation
        for (int k = 0; k < 8; k++)
        {
            BYTE b = *(pMem + DisMemAddr + line + k);
            if (b > 31)
                sprintf(s2, "%c", b);
            else
                strcpy(s2, ".");
            strcat(s, s2);
        }
        SendMessage(GetDlgItem(hWnd, IDC_LIST_MEM), LB_ADDSTRING, 0, LPARAM(s));
    }
    prevMem = DisMemAddr;
}

//
//	Window procedure of the POKE window
//
LRESULT CALLBACK PokeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL fDecHex = TRUE;

    switch (msg)
    {
    case WM_INITDIALOG:
        SendMessage(GetDlgItem(hWnd, IDC_RADIO_DEC), BM_SETCHECK,
            fDecHex ? BST_UNCHECKED : BST_CHECKED, 0);
        SendMessage(GetDlgItem(hWnd, IDC_RADIO_HEX), BM_SETCHECK,
            fDecHex ? BST_CHECKED : BST_UNCHECKED, 0);
        break;

    case WM_COMMAND:

        // Processing notification messages
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
            case IDC_RADIO_DEC: fDecHex = FALSE; break;
            case IDC_RADIO_HEX:	fDecHex = TRUE;
            case IDOK:
            {
                // Processing string inputs, converting them to numeric values depending on the
                // selected number system. Input validations. (Address range 0000-3FFF is readonly - ROM)
                char s[11], * e;
                WORD addr;
                BYTE val;
                long num;
                GetWindowText(GetDlgItem(hWnd, IDC_EDIT_ADDR), s, 10);
                num = strtol(s, &e, fDecHex ? 16 : 10);
                if (*e || num > 0xffff || num < 0 || !strlen(s))
                {
                    Failure(IDS_FAIL_WINPFIELD);
                    return TRUE;
                }
                addr = (WORD)num;
                GetWindowText(GetDlgItem(hWnd, IDC_EDIT_VAL), s, 10);
                num = strtol(s, &e, fDecHex ? 16 : 10);
                if (*e || num > 0xff || num < 0 || !strlen(s))
                {
                    Failure(IDS_FAIL_WINPFIELD);
                    return TRUE;
                }
                val = (BYTE)num;
                if (addr > 0x3fff) *(pMem + addr) = val;
            }
            return TRUE;

            case IDCANCEL:
                EndDialog(hWnd, 0);
                return TRUE;
            }
        }
    }
    return FALSE;
}