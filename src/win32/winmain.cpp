/*
 * Copyright (C) 2001  Erik Latoshek [forester] (laterk@inbox.lv)
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

#pragma warning(disable : 4047)

#include "common/setup_before.h"
#include "winmain.h"

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#include <windows.h>

#if _DEBUG
#include <dbghelp.h>
#endif
#include <windowsx.h>
#include <winuser.h>
#include <process.h>
#include <richedit.h>
#include <commctrl.h>
#include <time.h>

#include "common/addr.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/version.h"
#include "common/gui_printf.h"

#include "bnetd/connection.h"
#include "bnetd/account.h"
#include "bnetd/account_wrap.h"
#include "bnetd/ipban.h"
#include "bnetd/message.h"
#include "bnetd/server.h"
#include "bnetd/cmdline.h"
#include "resource.h"
#include "console_output.h"

#include "common/setup_after.h"

#define WM_SHELLNOTIFY          (WM_USER+1)

extern int app_main(int argc, char **argv); /* bnetd main function in bnetd/main.c */

namespace pvpgn
{

	extern HWND	ghwndConsole;

	namespace bnetd
	{

		extern int server_main(int argc, char *argv[]);

		static void	guiThread(void*);
		static void	guiAddText(const char *, COLORREF);
		static void	guiAddText_user(const char *, COLORREF);
		static void	guiDEAD(const std::wstring& msg);
		static void	guiMoveWindow(HWND, RECT*);
		static void	guiClearLogWindow();
		static void	guiKillTrayIcon();

		long PASCAL guiWndProc(HWND, UINT, WPARAM, LPARAM);
		static void	guiOnCommand(HWND, int, HWND, UINT);
		static void	guiOnMenuSelect(HWND, HMENU, int, HMENU, UINT);
		static int	guiOnShellNotify(int, int);
		static BOOL	guiOnCreate(HWND, LPCREATESTRUCT);
		static void	guiOnClose(HWND);
		static void	guiOnSize(HWND, UINT, int, int);
		static void	guiOnPaint(HWND);
		static void	guiOnCaptureChanged(HWND);
		static BOOL	guiOnSetCursor(HWND, HWND, UINT, UINT);
		static void	guiOnMouseMove(HWND, int, int, UINT);
		static void	guiOnLButtonDown(HWND, BOOL, int, int, UINT);
		static void	guiOnLButtonUp(HWND, int, int, UINT);
		static void	guiOnServerConfig();
		static void	guiOnAbout(HWND);
		static void	guiOnUpdates();
		static void	guiOnAnnounce(HWND);
		static void	guiOnUserStatusChange(HWND);

		BOOL CALLBACK	AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
		BOOL CALLBACK	AnnDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
		BOOL CALLBACK	KickDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

#define MODE_HDIVIDE    1
#define MODE_VDIVIDE    1

		struct gui_struc {
			HWND	hwnd;
			HMENU	hmenuTray;
			HWND	hwndUsers;
			HWND	hwndUserCount;
			HWND	hwndUserEditButton;
			HWND	hwndTree;
			int	y_ratio;
			int	x_ratio;
			HANDLE	event_ready;
			BOOL	main_finished;
			int	mode;
			char	szDefaultStatus[128];
			RECT	rectHDivider,
				rectVDivider,
				rectConsole,
				rectUsers,
				rectConsoleEdge,
				rectUsersEdge;
			WPARAM	wParam;
			LPARAM	lParam;
		};

		static struct gui_struc gui;

		char selected_item[255] = {};

		int fprintf(FILE *stream, const char *format, ...)
		{
			va_list args;
			va_start(args, format);
			if (stream == stderr || stream == stdout)
			{
				char buf[1024] = {};
				std::vsnprintf(buf, sizeof buf, format, args);
				gui_lvprintf(eventlog_level_error, "{}", buf);
				return 1;
			}
			else
				return vfprintf(stream, format, args);
		}

		static void guiThread(void *param)
		{
			HMODULE hRichEd = LoadLibraryW(L"RichEd20.dll");
			if (hRichEd == nullptr)
				guiDEAD(L"Could not load RichEd20.dll");

			WNDCLASSEXW wc = {};
			wc.cbSize = sizeof(WNDCLASSEXW);
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = static_cast<WNDPROC>(guiWndProc);
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = static_cast<HINSTANCE>(param);
			wc.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(IDI_ICON1));
			wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
			wc.hbrBackground = nullptr;
			wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU);
			wc.lpszClassName = L"BnetdWndClass";
			wc.hIconSm = nullptr;

			if (!RegisterClassExW(&wc))
			{
				FreeLibrary(hRichEd);
				guiDEAD(L"cant register WNDCLASS");
			}

			gui.hwnd = CreateWindowExW(
				0,
				wc.lpszClassName,
				L"Player -vs- Player Gaming Network Server",
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				nullptr,
				nullptr,
				static_cast<HINSTANCE>(param),
				nullptr);

			if (!gui.hwnd)
			{
				FreeLibrary(hRichEd);
				guiDEAD(L"cant create window");
			}

			ShowWindow(gui.hwnd, SW_SHOW);
			SetEvent(gui.event_ready);

			MSG msg = {};
			while (GetMessageW(&msg, nullptr, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}

			FreeLibrary(hRichEd);
		}

		long PASCAL guiWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			gui.wParam = wParam;
			gui.lParam = lParam;

			switch (message)
			{
				HANDLE_MSG(hwnd, WM_CREATE, guiOnCreate);
				HANDLE_MSG(hwnd, WM_COMMAND, guiOnCommand);
				HANDLE_MSG(hwnd, WM_MENUSELECT, guiOnMenuSelect);
				HANDLE_MSG(hwnd, WM_SIZE, guiOnSize);
				HANDLE_MSG(hwnd, WM_CLOSE, guiOnClose);
				HANDLE_MSG(hwnd, WM_PAINT, guiOnPaint);
				HANDLE_MSG(hwnd, WM_SETCURSOR, guiOnSetCursor);
				HANDLE_MSG(hwnd, WM_LBUTTONDOWN, guiOnLButtonDown);
				HANDLE_MSG(hwnd, WM_LBUTTONUP, guiOnLButtonUp);
				HANDLE_MSG(hwnd, WM_MOUSEMOVE, guiOnMouseMove);

			case WM_CAPTURECHANGED:
				guiOnCaptureChanged(reinterpret_cast<HWND>(lParam));
				return 0;
			case WM_SHELLNOTIFY:
				return guiOnShellNotify(wParam, lParam);
			}

			return DefWindowProcW(hwnd, message, wParam, lParam);
		}

		static BOOL guiOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
		{
			ghwndConsole = CreateWindowExW(
				0,
				RICHEDIT_CLASS,
				nullptr,
				WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE | WS_VSCROLL | WS_HSCROLL | ES_NOHIDESEL,
				0, 0,
				0, 0,
				hwnd,
				0,
				0,
				nullptr);

			if (!ghwndConsole)
				return FALSE;

			gui.hwndUsers = CreateWindowExW(
				WS_EX_CLIENTEDGE,
				L"LISTBOX",
				nullptr,
				WS_CHILD | WS_VISIBLE | LBS_STANDARD | LBS_NOINTEGRALHEIGHT,
				0, 0,
				0, 0,
				hwnd,
				0,
				0,
				nullptr);

			if (!gui.hwndUsers)
				return FALSE;

			//amadeo: temp. button for useredit until rightcklick is working....
			gui.hwndUserEditButton = CreateWindowExW(
				0L,
				L"button",
				L"Edit User Status",
				WS_CHILD | WS_VISIBLE | ES_LEFT,
				0, 0,
				0, 0,
				hwnd,
				reinterpret_cast<HMENU>(881),
				0,
				nullptr);

			if (!gui.hwndUserEditButton)
				return FALSE;

			gui.hwndUserCount = CreateWindowExW(
				WS_EX_CLIENTEDGE,
				L"edit",
				L" 0 user(s) online:",
				WS_CHILD | WS_VISIBLE | ES_CENTER | ES_READONLY,
				0, 0,
				0, 0,
				hwnd,
				0,
				0,
				nullptr);

			if (!gui.hwndUserCount)
				return FALSE;

			SendMessageW(gui.hwndUserCount, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), 0);
			SendMessageW(gui.hwndUsers, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), 0);
			SendMessageW(gui.hwndUserEditButton, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), 0);
			BringWindowToTop(gui.hwndUsers);
			std::snprintf(gui.szDefaultStatus, sizeof(gui.szDefaultStatus), "%s", "Void");

			gui.y_ratio = (100 << 10) / 100;
			gui.x_ratio = (0 << 10) / 100;

			return TRUE;
		}

		static void guiOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
		{
			switch (id)
			{
				case IDM_EXIT:
					guiOnClose(hwnd);
					break;
				case IDM_SAVE:
					server_save_wraper();
					break;
				case IDM_RESTART_LUA:
					server_restart_wraper(restart_mode_lua);
					break;
				case IDM_RESTART:
					server_restart_wraper(restart_mode_all);
					break;
				case IDM_SHUTDOWN:
					server_quit_wraper();
					break;
				case IDM_CLEAR:
					guiClearLogWindow();
					break;
				case IDM_RESTORE:
					guiOnShellNotify(IDI_TRAY, WM_LBUTTONDBLCLK);
					break;
				case IDM_USERLIST:
					guiOnUpdateUserList();
					break;
				case IDM_SERVERCONFIG:
					guiOnServerConfig();
					break;
				case IDM_ABOUT:
					guiOnAbout(hwnd);
					break;
				case ID_HELP_CHECKFORUPDATES:
					guiOnUpdates();
					break;
				case IDM_ANN:
					guiOnAnnounce(hwnd);
					break;
				case ID_USERACTIONS_KICKUSER:
				case 881:
					guiOnUserStatusChange(hwnd);
					break;
			}
		}

		static void guiOnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
		{
			// char str[256];
			//by greini 
			/*if( item == 0 || flags == -1)
				SetWindowText( gui.hwndStatus, gui.szDefaultStatus );
				else
				{
				LoadString( GetWindowInstance(hwnd), item, str, sizeof(str) );
				if( str[0] ) SetWindowText( gui.hwndStatus, str );
				}*/
		}

		static int guiOnShellNotify(int uID, int uMessage)
		{
			if (uID == IDI_TRAY)
			{
				if (uMessage == WM_LBUTTONDBLCLK)
				{
					if (!IsWindowVisible(gui.hwnd))
						ShowWindow(gui.hwnd, SW_RESTORE);

					SetForegroundWindow(gui.hwnd);
				}
				else if (uMessage == WM_RBUTTONDOWN)
				{
					POINT cp;
					GetCursorPos(&cp);
					SetForegroundWindow(gui.hwnd);
					TrackPopupMenu(gui.hmenuTray, TPM_LEFTALIGN | TPM_LEFTBUTTON, cp.x, cp.y, 0, gui.hwnd, NULL);
				}
			}

			return 0;
		}

		static void guiOnPaint(HWND hwnd)
		{
			PAINTSTRUCT ps;
			HDC dc;

			dc = BeginPaint(hwnd, &ps);

			DrawEdge(dc, &gui.rectHDivider, BDR_SUNKEN, BF_MIDDLE);
			DrawEdge(dc, &gui.rectConsoleEdge, BDR_SUNKEN, BF_RECT);
			DrawEdge(dc, &gui.rectUsersEdge, BDR_SUNKEN, BF_RECT);

			EndPaint(hwnd, &ps);

			UpdateWindow(ghwndConsole);
			UpdateWindow(gui.hwndUsers);
			UpdateWindow(gui.hwndUserCount);
			UpdateWindow(gui.hwndUserEditButton);
		}

		static void guiOnClose(HWND hwnd)
		{
			guiKillTrayIcon();
			if (!gui.main_finished)
			{
				eventlog(eventlog_level_debug, __FUNCTION__, "GUI wants server dead...");
				std::exit(0);
			}
			else
			{
				eventlog(eventlog_level_debug, __FUNCTION__, "GUI wants to exit...");
				eventlog_close();
				SetEvent(gui.event_ready);
			}
		}

		static void guiOnSize(HWND hwnd, UINT state, int cx, int cy)
		{
			if (state == SIZE_MINIMIZED)
			{
				NOTIFYICONDATAW dta = {};

				dta.cbSize = sizeof(NOTIFYICONDATAW);
				dta.hWnd = hwnd;
				dta.uID = IDI_TRAY;
				dta.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				dta.uCallbackMessage = WM_SHELLNOTIFY;
				dta.hIcon = LoadIconW(GetWindowInstance(hwnd), MAKEINTRESOURCE(IDI_ICON1));
				std::swprintf(dta.szTip, sizeof dta.szTip / sizeof *dta.szTip, L"%ls %ls", PVPGN_SOFTWAREW, PVPGN_VERSIONW);
				Shell_NotifyIconW(NIM_ADD, &dta);
				ShowWindow(hwnd, SW_HIDE);
				return;
			}

			if (state == SIZE_RESTORED)
			{
				NOTIFYICONDATAW dta = {};

				dta.hWnd = hwnd;
				dta.uID = IDI_TRAY;
				Shell_NotifyIconW(NIM_DELETE, &dta);
			}

			int cy_status = 0;
			int cy_edge = GetSystemMetrics(SM_CYEDGE);
			int cx_edge = GetSystemMetrics(SM_CXEDGE);
			int cy_frame = (cy_edge << 1) + GetSystemMetrics(SM_CYBORDER) + 1;
			int cy_console = ((cy - cy_status - cy_frame - cy_edge * 2) * gui.y_ratio) >> 10;
			gui.rectConsoleEdge.left = 0;
			gui.rectConsoleEdge.right = cx - 140;
			gui.rectConsoleEdge.top = 0;
			gui.rectConsoleEdge.bottom = cy - cy_status;
			gui.rectConsole.left = cx_edge;
			gui.rectConsole.right = cx - 140 - cx_edge;
			gui.rectConsole.top = cy_edge;
			gui.rectConsole.bottom = cy - cy_status;
			gui.rectUsersEdge.left = cx - 140;
			gui.rectUsersEdge.top = 18;
			gui.rectUsersEdge.right = cx;
			gui.rectUsersEdge.bottom = cy - cy_status - 10;
			gui.rectUsers.left = cx - 138;
			gui.rectUsers.right = cx;
			gui.rectUsers.top = 18 + cy_edge;
			gui.rectUsers.bottom = cy - cy_status - 20;
			guiMoveWindow(ghwndConsole, &gui.rectConsole);
			guiMoveWindow(gui.hwndUsers, &gui.rectUsers);
			MoveWindow(gui.hwndUserCount, cx - 140, 0, 140, 18, TRUE);
			MoveWindow(gui.hwndUserEditButton, cx - 140, cy - cy_status - 20, 140, 20, TRUE);
		}

		static BOOL guiOnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
		{
			POINT p;

			if (hwnd == hwndCursor && codeHitTest == HTCLIENT)
			{
				GetCursorPos(&p);
				ScreenToClient(hwnd, &p);
				if (PtInRect(&gui.rectHDivider, p))
					SetCursor(LoadCursorW(0, IDC_SIZENS));

				return TRUE;
			}

			return FORWARD_WM_SETCURSOR(hwnd, hwndCursor, codeHitTest, msg, DefWindowProcW);
		}

		static void guiOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
		{
			POINT p;

			p.x = x;
			p.y = y;

			if (PtInRect(&gui.rectHDivider, p))
			{
				SetCapture(hwnd);
				gui.mode |= MODE_HDIVIDE;
			}
		}

		static void guiOnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
		{
			ReleaseCapture();
			gui.mode &= ~(MODE_HDIVIDE | MODE_VDIVIDE);
		}

		static void guiOnCaptureChanged(HWND hwndNewCapture)
		{
			gui.mode &= ~(MODE_HDIVIDE | MODE_VDIVIDE);
		}

		static void guiOnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
		{
			int offset, cy_console, cy_users;
			RECT r;

			if (gui.mode & MODE_HDIVIDE)
			{
				offset = y - gui.rectHDivider.top;
				if (!offset) return;
				cy_console = gui.rectConsole.bottom - gui.rectConsole.top;
				cy_users = gui.rectUsers.bottom - gui.rectUsers.top;

				if (cy_console + offset <= 0)
					offset = -cy_console;
				else if (cy_users - offset <= 0)
					offset = cy_users;

				cy_console += offset;
				cy_users -= offset;
				if (cy_console + cy_users == 0) return;
				gui.y_ratio = (cy_console << 10) / (cy_console + cy_users);
				GetClientRect(hwnd, &r);
				guiOnSize(hwnd, 0, r.right, r.bottom);
				InvalidateRect(hwnd, NULL, FALSE);
			}
		}

		static void guiOnUpdates()
		{
			ShellExecuteW(nullptr, L"open", L"http://pvpgn.pro/", nullptr, nullptr, SW_SHOW);
		}

		static void guiOnAnnounce(HWND hwnd)
		{
			DialogBoxW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_ANN), hwnd, static_cast<DLGPROC>(AnnDlgProc));
		}

		static void guiOnUserStatusChange(HWND hwnd)
		{
			int  index;
			index = SendMessageW(gui.hwndUsers, LB_GETCURSEL, 0, 0);
			SendMessageW(gui.hwndUsers, LB_GETTEXT, index, reinterpret_cast<LPARAM>(selected_item));
			DialogBoxW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_KICKUSER), hwnd, static_cast<DLGPROC>(KickDlgProc));
			SendMessageW(gui.hwndUsers, LB_SETCURSEL, -1, 0);
		}

		static void guiOnAbout(HWND hwnd)
		{
			DialogBoxW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_ABOUT), hwnd, static_cast<DLGPROC>(AboutDlgProc));
		}

		static void guiOnServerConfig()
		{
			ShellExecuteW(nullptr, nullptr, L"conf\\bnetd.conf", nullptr, nullptr, SW_SHOW);
		}

		extern void guiOnUpdateUserList()
		{
			t_connection * c;
			t_elem const * curr;
			t_account * acc;

			SendMessageW(gui.hwndUsers, LB_RESETCONTENT, 0, 0);

			LIST_TRAVERSE_CONST(connlist(), curr)
			{
				if (!(c = (t_connection *)elem_get_data(curr)))
					continue;
				if (!(acc = conn_get_account(c)))
					continue;

				SendMessageA(gui.hwndUsers, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(account_get_name(acc)));
			}

			std::wstring user_count(std::to_wstring(connlist_login_get_length()) + L" user(s) online:");
			SendMessageW(gui.hwndUserCount, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(user_count.c_str()));
		}

		static void guiAddText(const char *str, COLORREF clr)
		{
			int text_length = SendMessageW(ghwndConsole, WM_GETTEXTLENGTH, 0, 0);

			if (text_length > 30000)
			{
				CHARRANGE ds = {};
				ds.cpMin = 0;
				ds.cpMax = text_length - 30000;
				SendMessageW(ghwndConsole, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&ds));
				SendMessageW(ghwndConsole, EM_REPLACESEL, FALSE, 0);
			}

			CHARRANGE cr = {};
			cr.cpMin = text_length;
			cr.cpMax = text_length;
			SendMessageW(ghwndConsole, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));

			CHARFORMATW fmt = {};
			fmt.cbSize = sizeof(CHARFORMATW);
			fmt.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC | CFM_STRIKEOUT | CFM_UNDERLINE;
			fmt.yHeight = 160;
			fmt.dwEffects = 0;
			fmt.crTextColor = clr;
			std::swprintf(fmt.szFaceName, sizeof fmt.szFaceName / sizeof *fmt.szFaceName, L"%ls", L"Courier New");

			SendMessageW(ghwndConsole, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&fmt));
			SendMessageA(ghwndConsole, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(str));
		}

		static void guiDEAD(const std::wstring& message)
		{
			wchar_t* error_message = nullptr;

			FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&error_message),
				0,
				nullptr);

			wchar_t* nl = std::wcschr(error_message, '\r');
			if (nl)
				*nl = '0';

			MessageBoxW(0, std::wstring(message + L"\nGetLastError() = " + error_message).c_str(), L"guiDEAD", MB_ICONSTOP | MB_OK);

			LocalFree(error_message);

			std::exit(1);
		}

		static void guiMoveWindow(HWND hwnd, RECT* r)
		{
			MoveWindow(hwnd, r->left, r->top, r->right - r->left, r->bottom - r->top, TRUE);
		}

		static void guiClearLogWindow()
		{
			SendMessageW(ghwndConsole, WM_SETTEXT, 0, 0);
		}

		static void guiKillTrayIcon()
		{
			NOTIFYICONDATAW dta = {};

			dta.cbSize = sizeof(NOTIFYICONDATAW);
			dta.hWnd = gui.hwnd;
			dta.uID = IDI_TRAY;
			dta.uFlags = 0;
			Shell_NotifyIconW(NIM_DELETE, &dta);
		}

		BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
		{
			switch (Message)
			{
			case WM_INITDIALOG:
				return TRUE;
			case WM_COMMAND:
				switch (LOWORD(wParam))
				{
				case IDOK:
					EndDialog(hwnd, IDOK);
					break;
				}
				break;
			default:
				return FALSE;
			}

			return TRUE;
		}

		BOOL CALLBACK AnnDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
		{

			switch (Message)
			{
			case WM_INITDIALOG:
				return TRUE;
			case WM_COMMAND:
				switch (LOWORD(wParam))
				{
				case IDOK:
				{
					int len = GetWindowTextLengthW(GetDlgItem(hwnd, IDC_EDIT1));

					if (len > 0)
					{
						std::wstring buff(len, 0);
						GetDlgItemTextW(hwnd, IDC_EDIT1, &buff[0], buff.size());

						auto utf8_encode = [](const std::wstring& wstr)
						{
							if (wstr.empty())
								return std::string();
							int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
							std::string strTo(size_needed, 0);
							WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &strTo[0], size_needed, nullptr, nullptr);
							return strTo;
						};

						t_message* const message = message_create(message_type_error, nullptr, utf8_encode(buff).c_str());
						if (message)
						{
							message_send_all(message);
							message_destroy(message);
						}

						SetDlgItemTextW(hwnd, IDC_EDIT1, L"");
					}
					else
					{
						MessageBoxW(hwnd, L"You didn't enter anything!", L"Warning", MB_OK);
					}
					break;
				}
				}
				break;
			case WM_CLOSE:
				EndDialog(hwnd, IDOK);
				break;
			default:
				return FALSE;
			}
			return TRUE;
		}

		BOOL CALLBACK KickDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
		{
			switch (Message)
			{
			case WM_INITDIALOG:
				if (selected_item[0] != 0)
				{
					SetDlgItemTextA(hwnd, IDC_EDITKICK, selected_item);
				}

				return TRUE;
			case WM_COMMAND:
				switch (LOWORD(wParam))
				{
				case IDC_KICK_EXECUTE:
				{
					GetDlgItemTextA(hwnd, IDC_EDITKICK, selected_item, sizeof selected_item);

					t_connection* const conngui = connlist_find_connection_by_accountname(selected_item);
					t_account* const accountgui = accountlist_find_account(selected_item);

					if (conngui == nullptr)
					{
						MessageBoxA(hwnd, std::string(std::string(selected_item) + " could not be found in Userlist!").c_str(), "Error", MB_OK);
					}
					else
					{
						HWND hButton = GetDlgItem(hwnd, IDC_CHECKBAN);
						HWND hButton1 = GetDlgItem(hwnd, IDC_CHECKKICK);
						HWND hButton2 = GetDlgItem(hwnd, IDC_CHECKADMIN);
						HWND hButton3 = GetDlgItem(hwnd, IDC_CHECKMOD);
						HWND hButton4 = GetDlgItem(hwnd, IDC_CHECKANN);

						BOOL messageq = FALSE;
						BOOL kickq = FALSE;

						if (SendMessageW(hButton2, BM_GETCHECK, 0, 0) == BST_CHECKED)
						{
							account_set_admin(accountgui);
							account_set_command_groups(accountgui, 255);
							messageq = TRUE;
						}

						if (SendMessageW(hButton3, BM_GETCHECK, 0, 0) == BST_CHECKED)
						{
							account_set_auth_operator(accountgui, nullptr, 1);
							messageq = TRUE;
						}

						if (SendMessageW(hButton4, BM_GETCHECK, 0, 0) == BST_CHECKED)
						{
							account_set_strattr(accountgui, "BNET\\auth\\announce", "true");
							messageq = TRUE;
						}

						if (SendMessageW(hButton, BM_GETCHECK, 0, 0) == BST_CHECKED)
						{
							char temp[64];
							std::snprintf(temp, sizeof temp, "%s", addr_num_to_addr_str(conn_get_addr(conngui), 0));

							char ipadr[110];
							unsigned int	i_GUI;

							for (i_GUI = 0; temp[i_GUI] != ':'; i_GUI++)
								ipadr[i_GUI] = temp[i_GUI];

							ipadr[i_GUI] = 0;

							std::strcpy(temp, " a ");
							std::strcat(temp, ipadr);
							handle_ipban_command(nullptr, temp);

							temp[0] = 0;
							std::strcpy(temp, " has been added to IpBanList");
							std::strcat(ipadr, temp);
							if (messageq == TRUE)
							{
								std::strcat(ipadr, " and UserStatus changed");
								MessageBoxA(hwnd, ipadr, "ipBan & StatusChange", MB_OK);
								messageq = FALSE;
								kickq = FALSE;
							}
							else
								MessageBoxA(hwnd, ipadr, "ipBan", MB_OK);
						}

						if (SendMessageW(hButton1, BM_GETCHECK, 0, 0) == BST_CHECKED)
						{
							conn_set_state(conngui, conn_state_destroy);
							kickq = TRUE;
						}

						if ((messageq == TRUE) && (kickq == TRUE))
						{
							std::strcat(selected_item, "has been kicked and Status has changed");
							MessageBoxA(hwnd, selected_item, "UserKick & StatusChange", MB_OK);
						}

						if ((kickq == TRUE) && (messageq == FALSE))
						{
							std::strcat(selected_item, " has been kicked from the server");
							MessageBoxA(hwnd, selected_item, "UserKick", MB_OK);
						}

						if ((kickq == FALSE) && (messageq == TRUE))
						{
							std::strcat(selected_item, "'s Status has been changed");
							MessageBoxA(hwnd, selected_item, "StatusChange", MB_OK);
						}

						selected_item[0] = 0;
					}
					break;
				}
				}
				break;
			case WM_CLOSE:
				EndDialog(hwnd, IDC_EDITKICK);
				break;
			default:
				return FALSE;
			}
			return TRUE;
		}

	}

}

#if _DEBUG
void make_minidump(EXCEPTION_POINTERS* e)
{
	HMODULE hDbgHelp = LoadLibraryW(L"dbghelp.dll");
	if (hDbgHelp == nullptr)
		return;

	auto pMiniDumpWriteDump = reinterpret_cast<decltype(&MiniDumpWriteDump)>(GetProcAddress(hDbgHelp, "MiniDumpWriteDump"));
	if (pMiniDumpWriteDump == nullptr)
	{
		FreeLibrary(hDbgHelp);
		return;
	}

	wchar_t name[MAX_PATH] = {};
	{
		auto nameEnd = name + GetModuleFileNameW(GetModuleHandleW(nullptr), name, MAX_PATH);
		SYSTEMTIME t;
		GetSystemTime(&t);
		wsprintfW(nameEnd - std::wcslen(L".exe"), L"_%4d%02d%02d_%02d%02d%02d.dmp", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	}

	auto hFile = CreateFileW(name, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		FreeLibrary(hDbgHelp);
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = {};
	exceptionInfo.ThreadId = GetCurrentThreadId();
	exceptionInfo.ExceptionPointers = e;
	exceptionInfo.ClientPointers = FALSE;

	auto dumped = pMiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
		e ? &exceptionInfo : nullptr,
		nullptr,
		nullptr);

	CloseHandle(hFile);

	FreeLibrary(hDbgHelp);

	return;
}

LONG CALLBACK unhandled_handler(EXCEPTION_POINTERS* e)
{
	make_minidump(e);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif


using namespace pvpgn;
using namespace pvpgn::bnetd;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE reserved, LPSTR lpCmdLine, int nCmdShow)
{
#if _DEBUG
	SetUnhandledExceptionFilter(unhandled_handler);
#endif
	Console     console;

	if (cmdline_load(__argc, __argv) != 1)
	{
		return -1;
	}

	if (cmdline_get_console())
	{
		console.RedirectIOToConsole();
		return app_main(__argc, __argv);
	}

	pvpgn::bnetd::gui.main_finished = FALSE;
	pvpgn::bnetd::gui.event_ready = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	_beginthread(pvpgn::bnetd::guiThread, 0, (void*)hInstance);
	WaitForSingleObject(pvpgn::bnetd::gui.event_ready, INFINITE);

	auto result = app_main(__argc, __argv);

	pvpgn::bnetd::gui.main_finished = TRUE;
	eventlog(pvpgn::eventlog_level_debug, __FUNCTION__, "server exited ( return : {} )", result);
	WaitForSingleObject(pvpgn::bnetd::gui.event_ready, INFINITE);

	return 0;
}

#endif

