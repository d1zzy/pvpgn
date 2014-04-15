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
#include "common/setup_before.h"
#include "command.h"

#include <cctype>
#include <cerrno>
#include <cstring>
#include <cstdlib>

#include "compat/strcasecmp.h"
#include "compat/snprintf.h"
#include "common/tag.h"
#include "common/util.h"
#include "common/version.h"
#include "common/eventlog.h"
#include "common/bnettime.h"
#include "common/addr.h"
#include "common/packet.h"
#include "common/bnethash.h"
#include "common/list.h"
#include "common/proginfo.h"
#include "common/queue.h"
#include "common/bn_type.h"
#include "common/xalloc.h"
#include "common/xstr.h"
#include "common/trans.h"
#include "common/lstr.h"
#include "common/hashtable.h"

#include "connection.h"
#include "message.h"
#include "channel.h"
#include "game.h"
#include "team.h"
#include "account.h"
#include "account_wrap.h"
#include "server.h"
#include "prefs.h"
#include "ladder.h"
#include "timer.h"
#include "helpfile.h"
#include "mail.h"
#include "runprog.h"
#include "alias_command.h"
#include "realm.h"
#include "ipban.h"
#include "command_groups.h"
#include "news.h"
#include "topic.h"
#include "friends.h"
#include "clan.h"
#include "common/setup_after.h"
#include "common/flags.h"
#include "common/util.h"

#include "attrlayer.h"


#include "luawrapper.h"

namespace pvpgn
{

	namespace bnetd
	{
		lua::vm vm;

		void _register_functions();

		int _sum(lua_State* L);
		int __message_send_text(lua_State* L);

		char xmsgtemp[MAX_MESSAGE_LEN];
		char xmsgtemp2[MAX_MESSAGE_LEN];


		/* Reload all the lua scripts */
		extern void lua_unload()
		{
			
		}

		/* Initialize lua, register functions and load scripts */
		extern void lua_load(char const * scriptdir)
		{
			try
			{
				// initialize
				vm.initialize();

				std::vector<std::string> files = dir_getfiles(std::string(scriptdir), ".lua", true);

				// load all files from the script directory
				for (int i = 0; i < files.size(); ++i)
					vm.load_file(files[i].c_str());

				_register_functions();
			}
			catch (const std::exception& e)
			{
				eventlog(eventlog_level_error, __FUNCTION__, e.what());
			}
			catch (...)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "lua exception\n");
			}
		}

		/* Register C++ functions to be able use them from lua scripts */
		void _register_functions()
		{
			// register CFunction
			vm.reg("sum", _sum);
			vm.reg("message_send_text", __message_send_text);


		}

/* Lua Events */
#ifndef _LUA_EVENTS_

		extern int lua_handle_command(t_connection * c, char const * text)
		{
			unsigned int sessionkey; // FIXME: unsigned int
			int result;

			sessionkey = conn_get_sessionkey(c);

			lua::transaction(vm) << lua::lookup("handle_command") << sessionkey << text << lua::invoke >> result << lua::end;

			return result;
		}

#endif

/* --- Lua Functions (called from scripts) */
#ifndef _LUA_FUNCTIONS_

		int __message_send_text(lua_State* L)
		{
			lua::stack st(L);
			unsigned int sessionkey_src, sessionkey_dst;
			const char *text;
			int message_type;
			t_connection *c_src, *c_dst;

			// get vars
			st.at(1, sessionkey_dst);
			st.at(2, message_type);
			st.at(3, sessionkey_src);
			st.at(4, text);

			// get user connections
			c_dst = connlist_find_connection_by_sessionkey(sessionkey_dst);
			c_src = connlist_find_connection_by_sessionkey(sessionkey_src);

			// send message
			message_send_text(c_dst, (t_message_type)message_type, c_src, text);

			return 0;
		}



		int _sum(lua_State* L)
		{
			lua::stack st(L);

			double a = 0, b = 0;

			st.at(1, a);			// begin from 1
			st.at(2, b);

			st.push(a + b);

			return 1;			// number of return values (st.push)
		}

#endif

	}
}