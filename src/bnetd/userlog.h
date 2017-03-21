/*
* Copyright (C) 2014  HarpyWar (harpywar@gmail.com)
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
/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_USERLOGS_PROTOS
#define INCLUDED_USERLOGS_PROTOS

#include <string>
#include <vector>

#define JUST_NEED_TYPES
#include "connection.h"
#undef JUST_NEED_TYPES

namespace pvpgn
{

	namespace bnetd
	{
		extern void userlog_init();

		extern void userlog_append(t_account * account, const char * text);
		extern std::map<long, char*> userlog_read(const char* username, long startline, const char* search_substr = nullptr);
		extern std::map<long, char*> userlog_find_text(const char * username, const char * search_substr, long startline);
		extern std::string userlog_filename(const char * username, bool force_create_path = false);

		extern int handle_log_command(t_connection * c, char const *text);

	}

}

#endif
#endif