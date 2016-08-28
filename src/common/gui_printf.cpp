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
#include "gui_printf.h"

#include <cstdarg>
#include <cwchar>

#include <windows.h>
#include <richedit.h>

#include "common/eventlog.h"
#include <common/format.h>
#include "common/setup_after.h"

namespace pvpgn
{

	HWND ghwndConsole;

	static void guiAddText(const char *str, COLORREF clr)
	{
		int text_length = SendMessageW(ghwndConsole, WM_GETTEXTLENGTH, 0, 0);

		if (text_length > 30000)
		{
			CHARRANGE ds = {};
			ds.cpMin = 0;
			ds.cpMax = text_length - 30000;
			SendMessageW(ghwndConsole, EM_EXSETSEL, 0, (LPARAM)&ds);
			SendMessageW(ghwndConsole, EM_REPLACESEL, FALSE, 0);
		}

		CHARRANGE cr = {};
		cr.cpMin = text_length;
		cr.cpMax = text_length;
		SendMessageW(ghwndConsole, EM_EXSETSEL, 0, (LPARAM)&cr);

		CHARFORMATW fmt = {};
		fmt.cbSize = sizeof(CHARFORMATW);
		fmt.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC | CFM_STRIKEOUT | CFM_UNDERLINE;
		fmt.yHeight = 160;
		fmt.dwEffects = 0;
		fmt.crTextColor = clr;
		std::swprintf(fmt.szFaceName, sizeof fmt.szFaceName / sizeof *fmt.szFaceName, L"%ls", L"Courier New");

		SendMessageW(ghwndConsole, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
		SendMessageA(ghwndConsole, EM_REPLACESEL, FALSE, (LPARAM)str);
	}

	//template <typename... Args>
	//void gui_lvprintf(t_eventlog_level l, const char* format, const Args& ... args)
	void gui_lvprintf(t_eventlog_level l, const char* format, fmt::ArgList args)
	{
		COLORREF clr;

		switch (l)
		{
		case eventlog_level_none:
			clr = RGB(0, 0, 0);
			break;
		case eventlog_level_trace:
			clr = RGB(255, 0, 255);
			break;
		case eventlog_level_debug:
			clr = RGB(0, 0, 255);
			break;
		case eventlog_level_info:
			clr = RGB(0, 0, 0);
			break;
		case eventlog_level_warn:
			clr = RGB(255, 128, 64);
			break;
		case eventlog_level_error:
			clr = RGB(255, 0, 0);
			break;
		case eventlog_level_fatal:
			clr = RGB(255, 0, 0);
			break;
		default:
			clr = RGB(0, 0, 0);
		}

		guiAddText(fmt::format(format, args).c_str(), clr);
	}
}
#endif
