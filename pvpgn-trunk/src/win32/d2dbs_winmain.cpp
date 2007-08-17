/*
 * Copyright (C) 2004   CreepLord (creeplord@pvpgn.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#ifdef WIN32_GUI 
 
#include "common/setup_before.h" 
#include <windows.h>
#include <windowsx.h>
#include <richedit.h>
#include <process.h>
#include "gui_printf.h"
#include "d2dbs_resource.h"
#include "common/eventlog.h"
#include "d2dbs/version.h"
#include "d2dbs/handle_signal.h"
#include "service.h"
#include "winmain.h"
#include "common/setup_after.h"

#define WM_SHELLNOTIFY          (WM_USER+1)

extern int	server_main(int argc, char *argv[]); /* d2dbs main function in d2dbs/main.c */

namespace pvpgn
{
          
HWND		ghwndConsole; /* hwnd for eventlog output */
          
namespace d2dbs
{

static void	KillTrayIcon(HWND hwnd);
static int	OnShellNotify(HWND hwnd, int uID, int uMessage);
static void	OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL	OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
static void	OnClose(HWND hwnd);
static void	OnSize(HWND hwnd, UINT state, int cx, int cy);

LRESULT CALLBACK	WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK		DlgProcAbout(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static int	gui_run = TRUE;		/* default state: run gui */
static int	d2dbs_run = TRUE;	/* default state: run d2dbs */
static int	d2dbs_running = FALSE;	/* currect state: not running */

int fprintf(FILE *stream, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if(stream == stderr || stream == stdout)
		return gui_lvprintf(eventlog_level_error, format, args);
	else
		return vfprintf(stream, format, args);
}

static void KillTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA dta;
     
    dta.cbSize = sizeof(NOTIFYICONDATA);
    dta.hWnd = hwnd;
    dta.uID = ID_TRAY;
    dta.uFlags = 0;
    Shell_NotifyIcon(NIM_DELETE, &dta);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_CLOSE, OnClose);
        case WM_SHELLNOTIFY:
            return OnShellNotify(hwnd, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    ghwndConsole = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, NULL,
                                  WS_CHILD|WS_VISIBLE|ES_READONLY|ES_MULTILINE|
                                  WS_VSCROLL|WS_HSCROLL|ES_NOHIDESEL,
                                  0, 0,
                                  0, 0, hwnd, 0,
                                  0, NULL);

    if(!ghwndConsole)
        return FALSE;

    return TRUE;
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {
        case ID_RESTORE:
            OnShellNotify(hwnd, ID_TRAY, WM_LBUTTONDBLCLK);
            break;
        case ID_START_D2DBS:
            d2dbs::fprintf(stderr,"Sending Start Signal to d2dbs\n");
            d2dbs_run = TRUE;
            break;
        case ID_SHUTDOWN_D2DBS:
            d2dbs::fprintf(stderr,"Sending Shutdown Signal to d2dbs\n");
            d2dbs_run = FALSE;
            d2dbs_signal_quit_wrapper();
            break;
        case ID_RESTART_D2DBS:
            d2dbs::fprintf(stderr,"Sending Restart Signal To d2dbs\n");
            d2dbs_run = TRUE;
            d2dbs_signal_quit_wrapper();
            break;
        case ID_EDITCONFIG_D2DBS:
            ShellExecute(NULL, "open", "notepad.exe", "conf\\d2dbs.conf", NULL, SW_SHOW );
            break;
        case ID_LOADCONFIG_D2DBS:
            d2dbs::fprintf(stderr,"Sending Reload Config Signal To d2dbs\n");
            d2dbs_signal_reload_config_wrapper();
            break;
        case ID_LADDER_SAVE:
            d2dbs::fprintf(stderr,"Sending Save Ladder Signal To d2dbs\n");
            d2dbs_signal_save_ladder_wrapper();
            break;
        case ID_EXIT:
            OnClose(hwnd);
            break;
        case ID_CLEAR:
            SendMessage(ghwndConsole, WM_SETTEXT, 0, 0);
            break;
        case ID_ABOUT:
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ABOUT_BOX), hwnd, (DLGPROC)DlgProcAbout);
            break;
    }
}

static int OnShellNotify(HWND hwnd, int uID, int uMessage)
{
    if(uID == ID_TRAY) {
        if(uMessage == WM_LBUTTONDBLCLK) {
            if(!IsWindowVisible(hwnd))
                ShowWindow(hwnd, SW_RESTORE);
            
            SetForegroundWindow(hwnd);
        }
    }
    return 0;
}

static void OnClose(HWND hwnd)
{
    d2dbs::fprintf(stderr,"Sending Exit Signal To d2dbs\n");
    gui_run = FALSE;
    d2dbs_run = FALSE;
    d2dbs_signal_exit_wrapper();
}

static void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    NOTIFYICONDATA dta;

    if( state == SIZE_MINIMIZED ) {
        dta.cbSize = sizeof(NOTIFYICONDATA);
        dta.hWnd = hwnd;
        dta.uID = ID_TRAY;
        dta.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
        dta.uCallbackMessage = WM_SHELLNOTIFY;
        dta.hIcon = LoadIcon(GetWindowInstance(hwnd), MAKEINTRESOURCE(ID_ICON1));
        strcpy(dta.szTip, "D2DBS Version");
        strcat(dta.szTip, " ");
        strcat(dta.szTip, D2DBS_VERSION_NUMBER);

        Shell_NotifyIcon(NIM_ADD, &dta);
        ShowWindow(hwnd, SW_HIDE);
        return;
    }

    MoveWindow(ghwndConsole, 0, 0, cx, cy, TRUE);
}

BOOL CALLBACK DlgProcAbout(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CLOSE:
            EndDialog(hwnd, TRUE);
            return TRUE;
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
            switch ((int)wParam) {
                case IDOK:
                    EndDialog(hwnd, TRUE);
                    return TRUE;
            }
    }
    return FALSE;
}

#define EXIT_ERROR	 -1
#define EXIT_OK		  0
#define EXIT_SERVICE  1

static void d2dbs(void * dummy)
{
    switch (server_main(__argc, __argv))
    {
        case EXIT_SERVICE:
            gui_run = FALSE; /* close gui */
        case EXIT_ERROR:
            d2dbs_run = FALSE; /* don't restart */
        case EXIT_OK:
            ; /* do nothing */
    }
    
    d2dbs::fprintf(stderr,"Server Stopped\n");
    d2dbs_running = FALSE;
}

}

}

using namespace pvpgn::d2dbs;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    WNDCLASSEX	wc;
    HWND		hwnd;
    MSG			msg;
    
    /* if running as a service skip starting the GUI and go straight to starting d2dbs */
    if (__argc==2 && strcmp(__argv[1],"--service")==0)
    {
        Win32_ServiceRun();
        return 1;
    }
    
    LoadLibrary("RichEd20.dll");

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LPVOID);
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName = MAKEINTRESOURCE(ID_MENU);
    wc.lpszClassName = "BnetWndClass";

    if(!RegisterClassEx( &wc ))
        RegisterClass((LPWNDCLASS)&wc.style);

    hwnd = CreateWindow(TEXT("BnetWndClass"),"Diablo II DataBase Server",
                        WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,CW_USEDEFAULT,
                        CW_USEDEFAULT,CW_USEDEFAULT,
			NULL,
                        LoadMenu(hInstance, MAKEINTRESOURCE(ID_MENU)),
                        hInstance,NULL);
    
    if(hwnd) {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }
	
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
        
        if(!d2dbs_running && d2dbs_run && gui_run) {
            d2dbs_running = TRUE;
            _beginthread(pvpgn::d2dbs::d2dbs, 0, NULL);
        }
        
        if(!gui_run && !d2dbs_running) {
            KillTrayIcon(hwnd);
            exit(0);
        }    
    }
    return ((int) msg.wParam);
}

#endif
