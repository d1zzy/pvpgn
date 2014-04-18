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
#define GAME_INTERNAL_ACCESS

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
#include "luainterface.h"

namespace pvpgn
{

	namespace bnetd
	{
		lua::vm vm;

		void _register_functions();

		std::map<std::string, std::string> get_account_object(const char *username);
		std::map<std::string, std::string> get_account_object(t_connection * c);
		std::map<std::string, std::string> get_game_object(t_game * game);


		template <class T, class A>
		T join(const A &begin, const A &end, const T &t);

		int _sum(lua_State* L);
		int __message_send_text(lua_State* L);
		int __eventlog(lua_State* L);
		int __get_account(lua_State* L);

		char _msgtemp[MAX_MESSAGE_LEN];
		char _msgtemp2[MAX_MESSAGE_LEN];


		/* Unload all the lua scripts */
		extern void lua_unload()
		{
			// nothing to do, "vm.initialize()" already destroys lua vm before initialize
		}

		/* Initialize lua, register functions and load scripts */
		extern void lua_load(char const * scriptdir)
		{
			eventlog(eventlog_level_info, __FUNCTION__, "Loading Lua interface...");

			try
			{
				vm.initialize();

				std::vector<std::string> files = dir_getfiles(std::string(scriptdir), ".lua", true);
				
				// load all files from the script directory
				for (int i = 0; i < files.size(); ++i)
				{
					vm.load_file(files[i].c_str());

					snprintf(_msgtemp, sizeof(_msgtemp), "%s", files[i].c_str());
					eventlog(eventlog_level_info, __FUNCTION__, _msgtemp);
				}

				_register_functions();

				snprintf(_msgtemp, sizeof(_msgtemp), "Lua sripts were successfully loaded (%u files)", files.size());
				eventlog(eventlog_level_info, __FUNCTION__, _msgtemp);
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
			// global variables
			lua::table g(vm);
			g.update("PVPGN_SOFTWARE", PVPGN_SOFTWARE);
			g.update("PVPGN_VERSION", PVPGN_VERSION);

			// register CFunction
			vm.reg("sum", _sum);
			vm.reg("message_send_text", __message_send_text);
			vm.reg("eventlog", __eventlog);
			vm.reg("get_account", __get_account); // FIXME:

			// register package 'event'
			static const luaL_Reg event[] =
			{
				{ 0, 0 }
			};
			vm.reg("event", event);
		}


		/* Lua Events (called from scripts) */
#ifndef _LUA_EVENTS_

		extern int lua_handle_command(t_connection * c, char const * text)
		{
			int result = 0;
			std::map<std::string, std::string> o_account = get_account_object(c);
			try
			{
				// invoke lua method
				lua::transaction(vm) << lua::lookup("handle_command") << o_account << text << lua::invoke >> result << lua::end;
			}
			catch (const std::exception& e)
			{
				eventlog(eventlog_level_error, __FUNCTION__, e.what());
			}
			catch (...)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "lua exception\n");
			}
			return result;
		}

		extern void lua_handle_game(t_game * game, t_luaevent_type luaevent)
		{
			const char * func_name;
			switch (luaevent)
			{
				case luaevent_game_create:
					func_name = "handle_game_create";
					break;
				case luaevent_game_report:
					func_name = "handle_game_report";
					break;
				case luaevent_game_end:
					func_name = "handle_game_end";
					break;
				case luaevent_game_destroy:
					func_name = "handle_game_destroy";
					break;
				case luaevent_game_changestatus:
					func_name = "handle_game_changestatus";
					break;
			}

			try
			{
				std::map<std::string, std::string> o_game = get_game_object(game);
				lua::transaction(vm) << lua::lookup(func_name) << o_game << lua::invoke << lua::end;
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

		extern void lua_handle_user(t_connection * c, t_game * game, t_luaevent_type luaevent)
		{
			const char * func_name;
			switch (luaevent)
			{
			case luaevent_user_joingame:
				func_name = "handle_user_joingame";
				break;

			case luaevent_user_leftgame:
				func_name = "handle_user_leftgame";
				break;
			}

			std::map<std::string, std::string> o_account = get_account_object(c);
			std::map<std::string, std::string> o_game = get_game_object(game);
			try
			{
				lua::transaction(vm) << lua::lookup("handle_user_leftgame") << o_account << o_game << lua::invoke << lua::end;
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

#endif

		/* --- Lua Functions (called from scripts) */
#ifndef _LUA_FUNCTIONS_

		/* Send message text to user */
		int __message_send_text(lua_State* L)
		{
			const char *text;
			const char *username_src, *username_dst;
			int message_type;
			t_connection *c_src = NULL, *c_dst = NULL;

			try
			{
				lua::stack st(L);
				// get args
				st.at(1, username_dst);
				st.at(2, message_type);
				st.at(3, username_src);
				st.at(4, text);
			}
			catch (const std::exception& e)
			{
				eventlog(eventlog_level_error, __FUNCTION__, e.what());
			}
			catch (...)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "lua exception\n");
			}

			// get user connections
			if (t_account * account = accountlist_find_account(username_dst))
				c_dst = account_get_conn(account);

			if (username_src)
			if (t_account * account = accountlist_find_account(username_src))
				c_src = account_get_conn(account);

			// send message
			// (source can be NULL, but destination cant)
			if (c_dst)
				message_send_text(c_dst, (t_message_type)message_type, c_src, text);

			return 0;
		}

		/* Log text into logfile */
		int __eventlog(lua_State* L)
		{
			int loglevel;
			const char *text, *function;

			try
			{
				lua::stack st(L);
				// get args
				st.at(1, loglevel);
				st.at(2, function);
				st.at(3, text);
			}
			catch (const std::exception& e)
			{
				eventlog(eventlog_level_error, __FUNCTION__, e.what());
			}
			catch (...)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "lua exception\n");
			}
			eventlog(t_eventlog_level(loglevel), function, text);

			return 0;
		}

		/* Log text into logfile */
		int __get_account(lua_State* L)
		{
			const char *username;
			std::map<std::string, std::string> o_account;

			try
			{
				lua::stack st(L);
				// get args
				st.at(1, username);
				o_account = get_account_object(username);

				st.push(o_account);
			}
			catch (const std::exception& e)
			{
				eventlog(eventlog_level_error, __FUNCTION__, e.what());
			}
			catch (...)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "lua exception\n");
			}

			return 1;
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


		std::map<std::string, std::string> get_account_object(const char *username)
		{
			std::map<std::string, std::string> o_account;
			if (t_connection *c = connlist_find_connection_by_accountname(username))
				o_account = get_account_object(c);

			return o_account;
		}

		std::map<std::string, std::string> get_account_object(t_connection * c)
		{
			std::map<std::string, std::string> o_account;

			t_account * account = conn_get_account(c);
			if (!account)
				return o_account;

			o_account["id"] = std::to_string(account_get_uid(account));
			o_account["name"] = account_get_name(account);
			o_account["email"] = account_get_email(account);
			o_account["commandgroups"] = std::to_string(account_get_command_groups(account));
			o_account["locked"] = account_get_auth_lock(account) ? "true" : "false";
			o_account["muted"] = account_get_auth_mute(account) ? "true" : "false";
			o_account["country"] = conn_get_country(c);
			o_account["clientver"] = conn_get_clientver(c);
			o_account["latency"] = std::to_string(conn_get_latency(c));
			if (t_clienttag clienttag = conn_get_clienttag(c))
				o_account["clienttag"] = clienttag_uint_to_str(clienttag);
			if (t_game *game = conn_get_game(c))
				o_account["game_id"] = std::to_string(game_get_id(game));
			if (t_channel *channel = conn_get_channel(c))
				o_account["channel_id"] = std::to_string(channel_get_channelid(channel));

			return o_account;
		}


		std::map<std::string, std::string> get_game_object(t_game * game)
		{
			std::map<std::string, std::string> o_game;

			if (!game)
				return o_game;

			o_game["id"] = std::to_string(game->id);
			o_game["name"] = game->name;
			o_game["pass"] = game->pass;
			o_game["info"] = game->info;
			o_game["type"] = std::to_string(game->type);
			o_game["flag"] = std::to_string(game->flag);

			o_game["address"] = addr_num_to_ip_str(game->addr);
			o_game["port"] = std::to_string(game->port);
			o_game["status"] = std::to_string(game->status);
			o_game["currentplayers"] = std::to_string(game->ref);
			o_game["totalplayers"] = std::to_string(game->count);
			o_game["maxplayers"] = std::to_string(game->maxplayers);
			o_game["mapname"] = game->mapname;
			o_game["option"] = std::to_string(game->option);
			o_game["maptype"] = std::to_string(game->maptype);
			o_game["tileset"] = std::to_string(game->tileset);
			o_game["speed"] = std::to_string(game->speed);
			o_game["mapsize_x"] = std::to_string(game->mapsize_x);
			o_game["mapsize_y"] = std::to_string(game->mapsize_y);
			if (t_connection *c = game->owner)
			{
				if (t_account *account = conn_get_account(c))
					o_game["owner"] = account_get_name(account);
			}

			std::vector<std::string> players;
			for (int i = 0; i < game->ref; i++)
			{
				if (t_account *account = game->players[i])
					players.push_back(account_get_name(account));
			}
			o_game["players"] = join(players.begin(), players.end(), std::string(","));


			o_game["bad"] = std::to_string(game->bad); // if 1, then the results will be ignored 

			std::vector<std::string> results;
			if (game->results)
			{
				for (int i = 0; i < game->count; i++)
					results.push_back(std::to_string(game->results[i]));
			}
			o_game["results"] = join(results.begin(), results.end(), std::string(","));
			// UNDONE: add report_heads and report_bodies: they are XML strings

			o_game["create_time"] = std::to_string(game->create_time);
			o_game["start_time"] = std::to_string(game->start_time);
			o_game["lastaccess_time"] = std::to_string(game->lastaccess_time);

			o_game["difficulty"] = std::to_string(game->difficulty);
			o_game["version"] = vernum_to_verstr(game->version);
			o_game["startver"] = std::to_string(game->startver);

			if (t_clienttag clienttag = game->clienttag)
				o_game["clienttag"] = clienttag_uint_to_str(clienttag);


			if (game->description)
				o_game["description"] = game->description;
			if (game->realmname)
				o_game["realmname"] = game->realmname;

			return o_game;
		}




		/* Join two vector objects to string by delimeter */
		template <class T, class A>
		T join(const A &begin, const A &end, const T &t)
		{
			T result;
			for (A it = begin;
				it != end;
				it++)
			{
				if (!result.empty())
					result.append(t);
				result.append(*it);
			}
			return result;
		}
	}
}