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
#ifndef INCLUDED_LUA_PROTOS
#define INCLUDED_LUA_PROTOS


// FIXME: include one time on pvpgn load
#include "luawrapper.h"
#include <stdio.h>

#ifdef WIN32
#pragma comment(lib, "lua5.1.lib")
#endif


#define JUST_NEED_TYPES
#include "connection.h"
#undef JUST_NEED_TYPES

namespace pvpgn
{

	namespace bnetd
	{
		typedef enum {
			luaevent_game_create,
			luaevent_game_report,
			luaevent_game_end,
			luaevent_game_destroy,
			luaevent_game_changestatus,
			luaevent_game_userjoin,
			luaevent_game_userleft,

			luaevent_channel_message, // user-to-channel
			luaevent_channel_userjoin,
			luaevent_channel_userleft,

			luaevent_clan_userjoin,
			luaevent_clan_userleft,

			luaevent_user_whisper, // user-to-user
			luaevent_user_login,
			luaevent_user_disconnect

		} t_luaevent_type;


		extern void lua_load(char const * scriptdir);
		extern void lua_unload();

		extern int lua_handle_command(t_connection * c, char const * text);
		extern void lua_handle_game(t_game * game, t_connection * c, t_luaevent_type luaevent);
		extern void lua_handle_channel(t_channel * channel, t_connection * c, char const * message_text, t_message_type message_type, t_luaevent_type luaevent);
		extern int lua_handle_user(t_connection * c, t_connection * c_dst, char const * message_text, t_luaevent_type luaevent);

			// TODO:
		//extern void lua_handle_user(t_connection * c, t_connection * c_dst, const char * text, t_luaevent_type luaevent);
		//extern void lua_handle_user(t_connection * c, t_channel * channel, const char * text, t_luaevent_type luaevent);
		//extern void lua_handle_user(t_connection * c, t_channel * channel, t_luaevent_type luaevent);
		//extern void lua_handle_user(t_connection * c, t_luaevent_type luaevent);

	}

}

#endif
#endif
