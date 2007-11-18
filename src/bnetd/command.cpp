/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999  Gediminas (gediminas_lt@mailexcite.com)
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
 * Copyright (C) 2000,2001  Marco Ziech (mmz@gmx.net)
 * Copyright (C) 2000  Dizzy
 * Copyright (C) 2000  Onlyer (onlyer@263.net)
 * Copyright (C) 2003,2004  Aaron
 * Copyright (C) 2004  Donny Redmond (dredmond@linuxmail.org)
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


namespace pvpgn
{

namespace bnetd
{

static char const * bnclass_get_str(unsigned int cclass);
static void do_whisper(t_connection * user_c, char const * dest, char const * text);
static void do_whois(t_connection * c, char const * dest);
static void user_timer_cb(t_connection * c, std::time_t now, t_timer_data str);

char msgtemp[MAX_MESSAGE_LEN];
char msgtemp2[MAX_MESSAGE_LEN];

static char const * bnclass_get_str(unsigned int cclass)
{
    switch (cclass)
    {
    case PLAYERINFO_DRTL_CLASS_WARRIOR:
	return "warrior";
    case PLAYERINFO_DRTL_CLASS_ROGUE:
	return "rogue";
    case PLAYERINFO_DRTL_CLASS_SORCERER:
	return "sorcerer";
    default:
	return "unknown";
    }
}


static void do_whisper(t_connection * user_c, char const * dest, char const * text)
{
    t_connection * dest_c;
    char const *   tname;

    if (!(dest_c = connlist_find_connection_by_name(dest,conn_get_realm(user_c))))
    {
	message_send_text(user_c,message_type_error,user_c,"That user is not logged on.");
	return;
    }

    if (conn_get_dndstr(dest_c))
    {
        snprintf(msgtemp, sizeof(msgtemp), "%.64s is unavailable (%.128s)",conn_get_username(dest_c),conn_get_dndstr(dest_c));
        message_send_text(user_c,message_type_info,user_c,msgtemp);
        return;
    }

    message_send_text(user_c,message_type_whisperack,dest_c,text);

    if (conn_get_awaystr(dest_c))
    {
        snprintf(msgtemp, sizeof(msgtemp), "%.64s is away (%.128s)",conn_get_username(dest_c),conn_get_awaystr(dest_c));
        message_send_text(user_c,message_type_info,user_c,msgtemp);
    }

    message_send_text(dest_c,message_type_whisper,user_c,text);

    if ((tname = conn_get_username(user_c)))
    {
        char username[1+MAX_USERNAME_LEN]; /* '*' + username (including NUL) */

	if (std::strlen(tname)<MAX_USERNAME_LEN)
	{
            std::sprintf(username,"*%s",tname);
	    conn_set_lastsender(dest_c,username);
	}
    }
}


static void do_whois(t_connection * c, char const * dest)
{
    t_connection *    dest_c;
    char              namepart[136]; /* 64 + " (" + 64 + ")" + NUL */
    char const *      verb;
    t_game const *    game;
    t_channel const * channel;

    if ((!(dest_c = connlist_find_connection_by_accountname(dest))) &&
        (!(dest_c = connlist_find_connection_by_name(dest,conn_get_realm(c)))))
    {
	t_account * dest_a;
	t_bnettime btlogin;
	std::time_t ulogin;
	struct std::tm * tmlogin;

	if (!(dest_a = accountlist_find_account(dest))) {
	    message_send_text(c,message_type_error,c,"Unknown user.");
	    return;
	}

	if (conn_get_class(c) == conn_class_bnet) {
	    btlogin = time_to_bnettime((std::time_t)account_get_ll_time(dest_a),0);
	    btlogin = bnettime_add_tzbias(btlogin, conn_get_tzbias(c));
	    ulogin = bnettime_to_time(btlogin);
	    if (!(tmlogin = std::gmtime(&ulogin)))
		std::strcpy(msgtemp, "User was last seen on ?");
	    else
		std::strftime(msgtemp, sizeof(msgtemp), "User was last seen on : %a %b %d %H:%M:%S",tmlogin);
	} else std::strcpy(msgtemp, "User is offline");
	message_send_text(c, message_type_info, c, msgtemp);
	return;
    }

    if (c==dest_c)
    {
	std::strcpy(namepart,"You");
	verb = "are";
    }
    else
    {
	char const * tname;

	std::sprintf(namepart,"%.64s",(tname = conn_get_chatcharname(dest_c,c)));
	conn_unget_chatcharname(dest_c,tname);
	verb = "is";
    }

    if ((game = conn_get_game(dest_c)))
    {
	snprintf(msgtemp, sizeof(msgtemp), "%s %s using %s and %s currently in %s game \"%.64s\".",
		namepart,
		verb,
		clienttag_get_title(conn_get_clienttag(dest_c)),
		verb,
		game_get_flag(game) == game_flag_private ? "private" : "",
		game_get_name(game));
    }
    else if ((channel = conn_get_channel(dest_c)))
    {
        snprintf(msgtemp, sizeof(msgtemp), "%s %s using %s and %s currently in channel \"%.64s\".",
		namepart,
		verb,
		clienttag_get_title(conn_get_clienttag(dest_c)),
		verb,
		channel_get_name(channel));
    }
    else
	snprintf(msgtemp, sizeof(msgtemp), "%s %s using %s.",
		namepart,
		verb,
		clienttag_get_title(conn_get_clienttag(dest_c)));
    message_send_text(c,message_type_info,c,msgtemp);

    if (conn_get_dndstr(dest_c))
    {
        snprintf(msgtemp, sizeof(msgtemp), "%s %s refusing messages (%.128s)",
		namepart,
		verb,
		conn_get_dndstr(dest_c));
	message_send_text(c,message_type_info,c,msgtemp);
    }
    else
        if (conn_get_awaystr(dest_c))
        {
            snprintf(msgtemp, sizeof(msgtemp), "%s away (%.128s)",
		    namepart,
		    conn_get_awaystr(dest_c));
	    message_send_text(c,message_type_info,c,msgtemp);
        }
}


static void user_timer_cb(t_connection * c, std::time_t now, t_timer_data str)
{
    if (!c)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return;
    }
    if (!str.p)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL str");
	return;
    }

    if (now!=(std::time_t)0) /* zero means user logged out before expiration */
	message_send_text(c,message_type_info,c,(char*)str.p);
    xfree(str.p);
}

typedef int (* t_command)(t_connection * c, char const * text);

typedef struct {
	const char * command_string;
	t_command    command_handler;
} t_command_table_row;

static int command_set_flags(t_connection * c); // [Omega]
// command handler prototypes
static int _handle_clan_command(t_connection * c, char const * text);
static int _handle_admin_command(t_connection * c, char const * text);
static int _handle_aop_command(t_connection * c, char const * text);
static int _handle_op_command(t_connection * c, char const * text);
static int _handle_tmpop_command(t_connection * c, char const * text);
static int _handle_deop_command(t_connection * c, char const * text);
static int _handle_voice_command(t_connection * c, char const * text);
static int _handle_devoice_command(t_connection * c, char const * text);
static int _handle_vop_command(t_connection * c, char const * text);
static int _handle_friends_command(t_connection * c, char const * text);
static int _handle_me_command(t_connection * c, char const * text);
static int _handle_whisper_command(t_connection * c, char const * text);
static int _handle_status_command(t_connection * c, char const * text);
static int _handle_who_command(t_connection * c, char const * text);
static int _handle_whois_command(t_connection * c, char const * text);
static int _handle_whoami_command(t_connection * c, char const * text);
static int _handle_announce_command(t_connection * c, char const * text);
static int _handle_beep_command(t_connection * c, char const * text);
static int _handle_nobeep_command(t_connection * c, char const * text);
static int _handle_version_command(t_connection * c, char const * text);
static int _handle_copyright_command(t_connection * c, char const * text);
static int _handle_uptime_command(t_connection * c, char const * text);
static int _handle_stats_command(t_connection * c, char const * text);
static int _handle_time_command(t_connection * c, char const * text);
static int _handle_channel_command(t_connection * c, char const * text);
static int _handle_rejoin_command(t_connection * c, char const * text);
static int _handle_away_command(t_connection * c, char const * text);
static int _handle_dnd_command(t_connection * c, char const * text);
static int _handle_squelch_command(t_connection * c, char const * text);
static int _handle_unsquelch_command(t_connection * c, char const * text);
//static int _handle_designate_command(t_connection * c, char const * text); Obsolete function [Omega]
//static int _handle_resign_command(t_connection * c, char const * text); Obsolete function [Omega]
static int _handle_kick_command(t_connection * c, char const * text);
static int _handle_ban_command(t_connection * c, char const * text);
static int _handle_unban_command(t_connection * c, char const * text);
static int _handle_reply_command(t_connection * c, char const * text);
static int _handle_realmann_command(t_connection * c, char const * text);
static int _handle_watch_command(t_connection * c, char const * text);
static int _handle_unwatch_command(t_connection * c, char const * text);
static int _handle_watchall_command(t_connection * c, char const * text);
static int _handle_unwatchall_command(t_connection * c, char const * text);
static int _handle_lusers_command(t_connection * c, char const * text);
static int _handle_news_command(t_connection * c, char const * text);
static int _handle_games_command(t_connection * c, char const * text);
static int _handle_channels_command(t_connection * c, char const * text);
static int _handle_addacct_command(t_connection * c, char const * text);
static int _handle_chpass_command(t_connection * c, char const * text);
static int _handle_connections_command(t_connection * c, char const * text);
static int _handle_finger_command(t_connection * c, char const * text);
static int _handle_operator_command(t_connection * c, char const * text);
static int _handle_admins_command(t_connection * c, char const * text);
static int _handle_quit_command(t_connection * c, char const * text);
static int _handle_kill_command(t_connection * c, char const * text);
static int _handle_killsession_command(t_connection * c, char const * text);
static int _handle_gameinfo_command(t_connection * c, char const * text);
static int _handle_ladderactivate_command(t_connection * c, char const * text);
static int _handle_rehash_command(t_connection * c, char const * text);
//static int _handle_rank_all_accounts_command(t_connection * c, char const * text);
static int _handle_shutdown_command(t_connection * c, char const * text);
static int _handle_ladderinfo_command(t_connection * c, char const * text);
static int _handle_timer_command(t_connection * c, char const * text);
static int _handle_serverban_command(t_connection * c, char const * text);
static int _handle_netinfo_command(t_connection * c, char const * text);
static int _handle_quota_command(t_connection * c, char const * text);
static int _handle_lockacct_command(t_connection * c, char const * text);
static int _handle_unlockacct_command(t_connection * c, char const * text);
static int _handle_flag_command(t_connection * c, char const * text);
static int _handle_tag_command(t_connection * c, char const * text);
//static int _handle_ipban_command(t_connection * c, char const * text); Redirected to handle_ipban_command() in ipban.c [Omega]
static int _handle_set_command(t_connection * c, char const * text);
static int _handle_motd_command(t_connection * c, char const * text);
static int _handle_ping_command(t_connection * c, char const * text);
static int _handle_commandgroups_command(t_connection * c, char const * text);
static int _handle_topic_command(t_connection * c, char const * text);
static int _handle_moderate_command(t_connection * c, char const * text);
static int _handle_clearstats_command(t_connection * c, char const * text);
static int _handle_tos_command(t_connection * c, char const * text);

static const t_command_table_row standard_command_table[] =
{
	{ "/clan"		, _handle_clan_command },
	{ "/c"			, _handle_clan_command },
	{ "/admin"		, _handle_admin_command },
	{ "/f"                  , _handle_friends_command },
	{ "/friends"            , _handle_friends_command },
	{ "/me"                 , _handle_me_command },
	{ "/msg"                , _handle_whisper_command },
	{ "/whisper"            , _handle_whisper_command },
	{ "/w"                  , _handle_whisper_command },
	{ "/m"                  , _handle_whisper_command },
	{ "/status"             , _handle_status_command },
	{ "/users"              , _handle_status_command },
	{ "/who"                , _handle_who_command },
	{ "/whois"              , _handle_whois_command },
	{ "/whereis"            , _handle_whois_command },
	{ "/where"              , _handle_whois_command },
	{ "/whoami"             , _handle_whoami_command },
	{ "/announce"           , _handle_announce_command },
	{ "/beep"               , _handle_beep_command },
	{ "/nobeep"             , _handle_nobeep_command },
	{ "/version"            , _handle_version_command },
	{ "/copyright"          , _handle_copyright_command },
	{ "/warrenty"           , _handle_copyright_command },
	{ "/license"            , _handle_copyright_command },
	{ "/uptime"             , _handle_uptime_command },
	{ "/stats"              , _handle_stats_command },
	{ "/astat"              , _handle_stats_command },
	{ "/time"               , _handle_time_command },
        { "/channel"            , _handle_channel_command },
	{ "/join"               , _handle_channel_command },
	{ "/rejoin"             , _handle_rejoin_command },
	{ "/away"               , _handle_away_command },
	{ "/dnd"                , _handle_dnd_command },
	{ "/ignore"             , _handle_squelch_command },
	{ "/squelch"            , _handle_squelch_command },
	{ "/unignore"           , _handle_unsquelch_command },
	{ "/unsquelch"          , _handle_unsquelch_command },
//	{ "/designate"          , _handle_designate_command }, Obsotele command [Omega]
//	{ "/resign"             , _handle_resign_command }, Obsolete command [Omega]
	{ "/kick"               , _handle_kick_command },
	{ "/ban"                , _handle_ban_command },
	{ "/unban"              , _handle_unban_command },
	{ "/tos"              , _handle_tos_command },

	{ NULL                  , NULL }

};

static const t_command_table_row extended_command_table[] =
{
	{ "/ann"                , _handle_announce_command },
	{ "/r"                  , _handle_reply_command },
	{ "/reply"              , _handle_reply_command },
	{ "/realmann"           , _handle_realmann_command },
	{ "/watch"              , _handle_watch_command },
	{ "/unwatch"            , _handle_unwatch_command },
	{ "/watchall"           , _handle_watchall_command },
	{ "/unwatchall"         , _handle_unwatchall_command },
	{ "/lusers"             , _handle_lusers_command },
	{ "/news"               , _handle_news_command },
	{ "/games"              , _handle_games_command },
	{ "/channels"           , _handle_channels_command },
	{ "/chs"                , _handle_channels_command },
	{ "/addacct"            , _handle_addacct_command },
	{ "/chpass"             , _handle_chpass_command },
	{ "/connections"        , _handle_connections_command },
	{ "/con"                , _handle_connections_command },
	{ "/finger"             , _handle_finger_command },
	{ "/operator"           , _handle_operator_command },
	{ "/aop"		, _handle_aop_command },
	{ "/op"           	, _handle_op_command },
	{ "/tmpop"           	, _handle_tmpop_command },
	{ "/deop"           	, _handle_deop_command },
	{ "/voice"		, _handle_voice_command },
	{ "/devoice"		, _handle_devoice_command },
	{ "/vop"		, _handle_vop_command },
	{ "/admins"             , _handle_admins_command },
	{ "/logout"             , _handle_quit_command },
	{ "/quit"               , _handle_quit_command },
	{ "/std::exit"               , _handle_quit_command },
	{ "/kill"               , _handle_kill_command },
	{ "/killsession"        , _handle_killsession_command },
	{ "/gameinfo"           , _handle_gameinfo_command },
	{ "/ladderactivate"     , _handle_ladderactivate_command },
	{ "/rehash"             , _handle_rehash_command },
//	{ "/rank_all_accounts"  , _handle_rank_all_accounts_command },
	{ "/shutdown"           , _handle_shutdown_command },
	{ "/ladderinfo"         , _handle_ladderinfo_command },
	{ "/timer"              , _handle_timer_command },
	{ "/serverban"          , _handle_serverban_command },
	{ "/netinfo"            , _handle_netinfo_command },
	{ "/quota"              , _handle_quota_command },
	{ "/lockacct"           , _handle_lockacct_command },
	{ "/unlockacct"         , _handle_unlockacct_command },
	{ "/flag"               , _handle_flag_command },
	{ "/tag"                , _handle_tag_command },
	{ "/help"               , handle_help_command },
	{ "/mail"               , handle_mail_command },
	{ "/ipban"              , handle_ipban_command }, // in ipban.c
	{ "/set"                , _handle_set_command },
	{ "/motd"               , _handle_motd_command },
	{ "/latency"            , _handle_ping_command },
	{ "/ping"               , _handle_ping_command },
	{ "/p"                  , _handle_ping_command },
	{ "/commandgroups"	, _handle_commandgroups_command },
	{ "/cg"			, _handle_commandgroups_command },
	{ "/topic"		, _handle_topic_command },
	{ "/moderate"		, _handle_moderate_command },
	{ "/clearstats"		, _handle_clearstats_command },

        { NULL                  , NULL }

};

char const * skip_command(char const * org_text)
{
   unsigned int i;
   char * text = (char *)org_text;
   for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
   if (text[i]!='\0') text[i++]='\0';             /* \0-terminate command */
   for (; text[i]==' '; i++);
   return &text[i];
}

extern int handle_command(t_connection * c,  char const * text)
{
  t_command_table_row const *p;

  for (p = standard_command_table; p->command_string != NULL; p++)
    {
      if (strstart(text, p->command_string)==0)
        {
	  if (!(command_get_group(p->command_string)))
	    {
	      message_send_text(c,message_type_error,c,"This command has been deactivated");
	      return 0;
	    }
	  if (!((command_get_group(p->command_string) & account_get_command_groups(conn_get_account(c)))))
	    {
	      message_send_text(c,message_type_error,c,"This command is reserved for admins.");
	      return 0;
	    }
	if (p->command_handler != NULL) return ((p->command_handler)(c,text));
	}
    }


    if (prefs_get_extra_commands()==0)
    {
	message_send_text(c,message_type_error,c,"Unknown command.");
	eventlog(eventlog_level_debug,__FUNCTION__,"got unknown standard command \"%s\"",text);
	return 0;
    }

    for (p = extended_command_table; p->command_string != NULL; p++)
      {
      if (strstart(text, p->command_string)==0)
        {
	  if (!(command_get_group(p->command_string)))
	    {
	      message_send_text(c,message_type_error,c,"This command has been deactivated");
	      return 0;
	    }
	  if (!((command_get_group(p->command_string) & account_get_command_groups(conn_get_account(c)))))
	    {
	      message_send_text(c,message_type_error,c,"This command is reserved for admins.");
	      return 0;
	    }
	if (p->command_handler != NULL) return ((p->command_handler)(c,text));
	}
    }

    if (std::strlen(text)>=2 && std::strncmp(text,"//",2)==0)
    {
	handle_alias_command(c,text);
	return 0;
    }

    message_send_text(c,message_type_error,c,"Unknown command.");
    eventlog(eventlog_level_debug,__FUNCTION__,"got unknown command \"%s\"",text);
    return 0;
}

// +++++++++++++++++++++++++++++++++ command implementations +++++++++++++++++++++++++++++++++++++++

static int _handle_clan_command(t_connection * c, char const * text)
{
  t_account * acc;
  t_clanmember * member;
  t_clan * clan;

  if((acc = conn_get_account(c)) && (member = account_get_clanmember(acc)) && (clan = clanmember_get_clan(member))) {
    text = skip_command(text);
    if ( text[0] == '\0' ) {
         message_send_text(c,message_type_info,c,"usage:");
         message_send_text(c,message_type_info,c,"/clan msg MESSAGE");
         message_send_text(c,message_type_info,c,"Whispers a message to all your fellow clan members.");
         if (clanmember_get_status(member)>=CLAN_SHAMAN) {
           message_send_text(c,message_type_info,c,"/clan public  /clan pub");
           message_send_text(c,message_type_info,c,"Opens the clan channel up to the public so that anyone may enter.");
           message_send_text(c,message_type_info,c,"/clan private  /clan priv");
           message_send_text(c,message_type_info,c,"Closes the clan channel such that only members of the clan may enter.");
           message_send_text(c,message_type_info,c,"/clan motd MESSAGE");
           message_send_text(c,message_type_info,c,"Update the clan message of the day to MESSAGE.");
        }
        return 0;
    }
    if (strstart(text,"msg")==0 || strstart(text,"m")==0 || strstart(text,"w")==0 || strstart(text,"whisper")==0) {
        char const *msg = skip_command(text);
        if(msg[0]=='\0') {
          message_send_text(c,message_type_info,c,"usage:");
          message_send_text(c,message_type_info,c,"/clan msg MESSAGE");
          message_send_text(c,message_type_info,c,"Whispers a message to all your fellow clan members.");
        }
        else {
          int counter = 0;
          t_list * cl_member_list = clan_get_members(clan);
          t_elem * curr;
          t_connection * dest_conn;
          t_clanmember * dest_member;

          LIST_TRAVERSE(cl_member_list,curr) {
    	        if (!(dest_member = (t_clanmember*)elem_get_data(curr))) {
    	           eventlog(eventlog_level_error,__FUNCTION__,"found NULL entry in list");
    	           continue;
    	        }

                if ((dest_conn = clanmember_get_conn(dest_member)) && (dest_conn != c)) {
                   message_send_text(dest_conn,message_type_whisper,c,msg);
                   counter++;
                }
          }
          if(counter)
             message_send_text(c,message_type_info,c,"Message was sent to all currently available clan members.");
          else
             message_send_text(c,message_type_info,c,"All fellow members of your clan are currently offline.");
        }
    }
    else
    if(clanmember_get_status(member)>=CLAN_SHAMAN)
    {
      if (strstart(text,"public")==0 || strstart(text,"pub")==0) {
        if(clan_get_channel_type(clan)!=0)
        {
          clan_set_channel_type(clan,0);
          message_send_text(c,message_type_info,c,"Clan channel is opened up!");
        }
        else
          message_send_text(c,message_type_error,c,"Clan channel has already been opened up!");
      }
      else
      if (strstart(text,"private")==0 || strstart(text,"priv")==0) {
        if(clan_get_channel_type(clan)!=1)
        {
          clan_set_channel_type(clan,1);
          message_send_text(c,message_type_info,c,"Clan channel is closed!");
        }
        else
          message_send_text(c,message_type_error,c,"Clan channel has already been closed!");
      }
      else
      if (strstart(text,"motd")==0) {
        const char * msg=skip_command(text);
        if(msg[0]=='\0')
        {
          message_send_text(c,message_type_info,c,"usage:");
          message_send_text(c,message_type_info,c,"/clan motd MESSAGE");
          message_send_text(c,message_type_info,c,"Update the clan message of the day to MESSAGE.");
        }
        else
        {
          clan_set_motd(clan, msg);
          message_send_text(c,message_type_info,c,"Clan message of day is updated!");
        }
      }
    }
    else
      message_send_text(c,message_type_error,c,"You are not the chieftain or shaman of clan!");
  }
  else
    message_send_text(c,message_type_error,c,"You are not in a clan!");

  return 0;
}

static int command_set_flags(t_connection * c)
{
  return channel_set_userflags(c);
}

static int _handle_admin_command(t_connection * c, char const * text)
{
    char const *	username;
    char		command;
    t_account *		acc;
    t_connection *	dst_c;
    int			changed=0;

    text = skip_command(text);

    if ((text[0]=='\0') || ((text[0] != '+') && (text[0] != '-'))) {
	message_send_text(c,message_type_info,c,"usage: /admin +username to promote user to Server Admin.");
	message_send_text(c,message_type_info,c,"       /admin -username to demote user from Server Admin.");
	return -1;
    }

    command = text[0];
    username = &text[1];

    if(!*username) {
	message_send_text(c,message_type_info,c,"You need to supply a username.");
      return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }
    dst_c = account_get_conn(acc);

    if (command == '+') {
	if (account_get_auth_admin(acc,NULL) == 1) {
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s is already a Server Admin",username);
	} else {
	    account_set_auth_admin(acc,NULL,1);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been promoted to a Server Admin",username);
	    snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has promoted you to a Server Admin",conn_get_loggeduser(c));
	    changed = 1;
	}
    } else {
	if (account_get_auth_admin(acc,NULL) != 1)
    	    snprintf(msgtemp, sizeof(msgtemp), "%.64s is no Server Admin, so you can't demote him",username);
	else {
	    account_set_auth_admin(acc,NULL,0);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been demoted from a Server Admin",username);
	    snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has demoted you from a Server Admin",conn_get_loggeduser(c));
	    changed = 1;
	}
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);
    command_set_flags(dst_c);
    return 0;
}

static int _handle_operator_command(t_connection * c, char const * text)
{
    char const *	username;
    char		command;
    t_account *		acc;
    t_connection *	dst_c;
    int			changed = 0;

    text = skip_command(text);

    if ((text[0]=='\0') || ((text[0] != '+') && (text[0] != '-'))) {
	message_send_text(c,message_type_info,c,"usage: /operator +username to promote user to Server Operator.");
	message_send_text(c,message_type_info,c,"       /operator -username to demote user from Server Operator.");
	return -1;
    }

    command = text[0];
    username = &text[1];

    if(!*username) {
	message_send_text(c,message_type_info,c,"You need to supply a username.");
      return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }
    dst_c = account_get_conn(acc);

    if (command == '+') {
	if (account_get_auth_operator(acc,NULL) == 1)
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s is already a Server Operator",username);
	else {
	    account_set_auth_operator(acc,NULL,1);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been promoted to a Server Operator",username);
	    snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has promoted you to a Server Operator",conn_get_loggeduser(c));
	    changed = 1;
	}
    } else {
	if (account_get_auth_operator(acc,NULL) != 1)
    	    snprintf(msgtemp, sizeof(msgtemp), "%.64s is no Server Operator, so you can't demote him",username);
	else {
	    account_set_auth_operator(acc,NULL,0);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been demoted from a Server Operator",username);
	    snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has promoted you to a Server Operator",conn_get_loggeduser(c));
	    changed = 1;
	}
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);
    command_set_flags(dst_c);
    return 0;
}

static int _handle_aop_command(t_connection * c, char const * text)
{
    char const *	username;
    char const *	channel;
    t_account *		acc;
    t_connection *	dst_c;
    int			changed = 0;

    if (!(conn_get_channel(c)) || !(channel = channel_get_name(conn_get_channel(c)))) {
	message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
	return -1;
    }

    if (account_get_auth_admin(conn_get_account(c),NULL)!=1 && account_get_auth_admin(conn_get_account(c),channel)!=1) {
	message_send_text(c,message_type_error,c,"You must be at least a Channel Admin to use this command.");
	return -1;
    }

    text = skip_command(text);

    if (!(username = &text[0])) {
	message_send_text(c, message_type_info, c, "You need to supply a username.");
	return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }

    dst_c = account_get_conn(acc);

    if (account_get_auth_admin(acc,channel) == 1)
	snprintf(msgtemp, sizeof(msgtemp), "%.64s is already a Channel Admin",username);
    else {
	account_set_auth_admin(acc,channel,1);
	snprintf(msgtemp, sizeof(msgtemp), "%.64s has been promoted to a Channel Admin",username);
	snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has promoted you to a Channel Admin for channel \"%.128s\"",conn_get_loggeduser(c),channel);
	changed = 1;
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);
    command_set_flags(dst_c);
    return 0;
}

static int _handle_vop_command(t_connection * c, char const * text)
{
    char const *	username;
    char const *	channel;
    t_account *		acc;
    t_connection *	dst_c;
    int			changed = 0;

    if (!(conn_get_channel(c)) || !(channel = channel_get_name(conn_get_channel(c)))) {
	message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
	return -1;
    }

    if (account_get_auth_admin(conn_get_account(c),NULL)!=1 && account_get_auth_admin(conn_get_account(c),channel)!=1) {
	message_send_text(c,message_type_error,c,"You must be at least a Channel Admin to use this command.");
	return -1;
    }

    text = skip_command(text);

    if (!(username = &text[0])) {
	message_send_text(c, message_type_info, c, "You need to supply a username.");
	return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }

    dst_c = account_get_conn(acc);

    if (account_get_auth_voice(acc,channel) == 1)
	snprintf(msgtemp, sizeof(msgtemp), "%.64s is already on VOP list",username);
    else {
	account_set_auth_voice(acc,channel,1);
	snprintf(msgtemp, sizeof(msgtemp), "%.64s has been added to the VOP list",username);
	snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has added you to the VOP list of channel \"%.128s\"",conn_get_loggeduser(c),channel);
	changed = 1;
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);
    command_set_flags(dst_c);
    return 0;
}

static int _handle_voice_command(t_connection * c, char const * text)
{
    char const *	username;
    char const *	channel;
    t_account *		acc;
    t_connection *	dst_c;
    int			changed = 0;

    if (!(conn_get_channel(c)) || !(channel = channel_get_name(conn_get_channel(c)))) {
	message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
	return -1;
    }

    if (!(account_is_operator_or_admin(conn_get_account(c),channel_get_name(conn_get_channel(c))))) {
	message_send_text(c,message_type_error,c,"You must be at least a Channel Operator to use this command.");
	return -1;
    }

    text = skip_command(text);

    if (!(username = &text[0])) {
	message_send_text(c, message_type_info, c, "You need to supply a username.");
	return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }
    dst_c = account_get_conn(acc);
    if (account_get_auth_voice(acc,channel)==1)
	snprintf(msgtemp, sizeof(msgtemp), "%.64s is already on VOP list, no need to Voice him", username);
    else
    {
      if ((!dst_c) || conn_get_channel(c)!=conn_get_channel(dst_c))
      {
	snprintf(msgtemp, sizeof(msgtemp), "%.64s must be on the same channel to voice him",username);
      }
      else
      {
        if (channel_conn_has_tmpVOICE(conn_get_channel(c),dst_c))
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has already Voice in this channel",username);
        else {
	  if (account_is_operator_or_admin(acc,channel))
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s allready is operator or admin, no need to voice him",username);
	  else
	  {
	    conn_set_tmpVOICE_channel(dst_c,channel);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been granted Voice in this channel",username);
	    snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has granted you Voice in this channel",conn_get_loggeduser(c));
	    changed = 1;
	  }
	}
      }
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);
    command_set_flags(dst_c);
    return 0;
}

static int _handle_devoice_command(t_connection * c, char const * text)
{
    char const *	username;
    char const *	channel;
    t_account *		acc;
    t_connection *	dst_c;
    int			done = 0;
    int			changed = 0;

    if (!(conn_get_channel(c)) || !(channel = channel_get_name(conn_get_channel(c)))) {
	message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
	return -1;
    }

    if (!(account_is_operator_or_admin(conn_get_account(c),channel_get_name(conn_get_channel(c))))) {
	message_send_text(c,message_type_error,c,"You must be at least a Channel Operator to use this command.");
	return -1;
    }

    text = skip_command(text);

    if (!(username = &text[0])) {
	message_send_text(c, message_type_info, c, "You need to supply a username.");
	return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }
    dst_c = account_get_conn(acc);

    if (account_get_auth_voice(acc,channel)==1)
    {
	if ((account_get_auth_admin(conn_get_account(c),channel)==1) || (account_get_auth_admin(conn_get_account(c),NULL)==1))
	{
	    account_set_auth_voice(acc,channel,0);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been removed from VOP list.",username);
	    snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has removed you from VOP list of channel \"%.128s\"",conn_get_loggeduser(c),channel);
	    changed = 1;
	}
	else
	{
	    snprintf(msgtemp, sizeof(msgtemp), "You must be at least Channel Admin to remove %.64s from the VOP list",username);
	}
	done = 1;
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);
    changed = 0;

    if ((dst_c) && channel_conn_has_tmpVOICE(conn_get_channel(c),dst_c)==1)
    {
      conn_set_tmpVOICE_channel(dst_c,NULL);
      snprintf(msgtemp, sizeof(msgtemp), "Voice has been taken from %.64s in this channel",username);
      snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has taken your Voice in channel \"%.128s\"",conn_get_loggeduser(c),channel);
      changed = 1;
      done = 1;
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);

    if (!done)
    {
     snprintf(msgtemp, sizeof(msgtemp), "%.64s has no Voice in this channel, so it can't be taken away",username);
     message_send_text(c, message_type_info, c, msgtemp);
    }

    command_set_flags(dst_c);
    return 0;
}

static int _handle_op_command(t_connection * c, char const * text)
{
    char const *	username;
    char const *	channel;
    t_account *		acc;
    int			OP_lvl;
    t_connection * 	dst_c;
    int			changed = 0;

    if (!(conn_get_channel(c)) || !(channel = channel_get_name(conn_get_channel(c)))) {
	message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
	return -1;
    }

    acc = conn_get_account(c);
    OP_lvl = 0;

    if (account_is_operator_or_admin(acc,channel))
      OP_lvl = 1;
    else if (channel_conn_is_tmpOP(conn_get_channel(c),c))
      OP_lvl = 2;

    if (OP_lvl==0)
    {
	message_send_text(c,message_type_error,c,"You must be at least a Channel Operator or tempOP to use this command.");
	return -1;
    }

    text = skip_command(text);

    if (!(username = &text[0])) {
	message_send_text(c, message_type_info, c, "You need to supply a username.");
	return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }

    dst_c = account_get_conn(acc);

    if (OP_lvl==1) // user is full op so he may fully op others
    {
      if (account_get_auth_operator(acc,channel) == 1)
	  snprintf(msgtemp, sizeof(msgtemp), "%.64s is allready a Channel Operator",username);
      else {
	  account_set_auth_operator(acc,channel,1);
	  snprintf(msgtemp, sizeof(msgtemp), "%.64s has been promoted to a Channel Operator",username);
	  snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has promoted you to a Channel Operator in channel \"%.128s\"",conn_get_loggeduser(c),channel);
	  changed = 1;
      }
    }
    else { // user is only tempOP so he may only tempOP others
         if ((!(dst_c)) || (conn_get_channel(c) != conn_get_channel(dst_c)))
          snprintf(msgtemp, sizeof(msgtemp), "%.64s must be on the same channel to tempOP him",username);
         else
         {
	   if (account_is_operator_or_admin(acc,channel))
	     snprintf(msgtemp, sizeof(msgtemp), "%.64s allready is operator or admin, no need to tempOP him",username);
	   else
	   {
             conn_set_tmpOP_channel(dst_c,channel);
	     snprintf(msgtemp, sizeof(msgtemp), "%.64s has been promoted to a tempOP",username);
	     snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has promoted you to a tempOP in this channel",conn_get_loggeduser(c));
	     changed = 1;
	   }
         }
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);
    command_set_flags(dst_c);
    return 0;
}

static int _handle_tmpop_command(t_connection * c, char const * text)
{
    char const *	username;
    char const *	channel;
    t_account *		acc;
    t_connection *	dst_c;
    int			changed = 0;

    if (!(conn_get_channel(c)) || !(channel = channel_get_name(conn_get_channel(c)))) {
	message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
	return -1;
    }

    if (!(account_is_operator_or_admin(conn_get_account(c),channel_get_name(conn_get_channel(c))) || channel_conn_is_tmpOP(conn_get_channel(c),c))) {
	message_send_text(c,message_type_error,c,"You must be at least a Channel Operator or tmpOP to use this command.");
	return -1;
    }

    text = skip_command(text);

    if (!(username = &text[0])) {
	message_send_text(c, message_type_info, c, "You need to supply a username.");
	return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }

    dst_c = account_get_conn(acc);

    if (channel_conn_is_tmpOP(conn_get_channel(c),dst_c))
       snprintf(msgtemp, sizeof(msgtemp), "%.64s has already tmpOP in this channel",username);
    else
    {
       if ((!(dst_c)) || (conn_get_channel(c) != conn_get_channel(dst_c)))
       snprintf(msgtemp, sizeof(msgtemp), "%.64s must be on the same channel to tempOP him",username);
       else
       {
	 if (account_is_operator_or_admin(acc,channel))
	   snprintf(msgtemp, sizeof(msgtemp), "%.64s allready is operator or admin, no need to tempOP him",username);
	 else
	 {
           conn_set_tmpOP_channel(dst_c,channel);
           snprintf(msgtemp, sizeof(msgtemp), "%.64s has been promoted to tmpOP in this channel",username);
	   snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has promoted you to a tempOP in this channel",conn_get_loggeduser(c));
	   changed = 1;
	 }
       }
    }

    if (changed && dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
    message_send_text(c, message_type_info, c, msgtemp);
    command_set_flags(dst_c);
    return 0;
}

static int _handle_deop_command(t_connection * c, char const * text)
{
    char const *	username;
    char const *	channel;
    t_account *		acc;
    int			OP_lvl;
    t_connection *	dst_c;
    int			done = 0;

    if (!(conn_get_channel(c)) || !(channel = channel_get_name(conn_get_channel(c)))) {
	message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
	return -1;
    }

    acc = conn_get_account(c);
    OP_lvl = 0;

    if (account_is_operator_or_admin(acc,channel))
      OP_lvl = 1;
    else if (channel_conn_is_tmpOP(conn_get_channel(c),account_get_conn(acc)))
      OP_lvl = 2;

    if (OP_lvl==0)
    {
	message_send_text(c,message_type_error,c,"You must be at least a Channel Operator or tempOP to use this command.");
	return -1;
    }

    text = skip_command(text);

    if (!(username = &text[0])) {
	message_send_text(c, message_type_info, c, "You need to supply a username.");
	return -1;
    }

    if(!(acc = accountlist_find_account(username))) {
	snprintf(msgtemp, sizeof(msgtemp), "There's no account with username %.64s.", username);
	message_send_text(c, message_type_info, c, msgtemp);
	return -1;
    }

    dst_c = account_get_conn(acc);

    if (OP_lvl==1) // user is real OP and allowed to deOP
      {
	if (account_get_auth_admin(acc,channel) == 1 || account_get_auth_operator(acc,channel) == 1) {
	  if (account_get_auth_admin(acc,channel) == 1) {
	    if (account_get_auth_admin(conn_get_account(c),channel)!=1 && account_get_auth_admin(conn_get_account(c),NULL)!=1)
	      message_send_text(c,message_type_info,c,"You must be at least a Channel Admin to demote another Channel Admin");
	    else {
	      account_set_auth_admin(acc,channel,0);
	      snprintf(msgtemp, sizeof(msgtemp), "%.64s has been demoted from a Channel Admin.", username);
	      message_send_text(c, message_type_info, c, msgtemp);
	      if (dst_c)
	      {
	        snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has demoted you from a Channel Admin of channel \"%.128s\"",conn_get_loggeduser(c),channel);
                message_send_text(dst_c, message_type_info, c, msgtemp2);
	      }
	    }
	  }
	  if (account_get_auth_operator(acc,channel) == 1) {
	    account_set_auth_operator(acc,channel,0);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been demoted from a Channel Operator",username);
	    message_send_text(c, message_type_info, c, msgtemp);
	    if (dst_c)
	    {
	      snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has demoted you from a Channel Operator of channel \"%.128s\"",conn_get_loggeduser(c),channel);
              message_send_text(dst_c, message_type_info, c, msgtemp2);
	    }
	  }
	  done = 1;
	}
	if ((dst_c) && channel_conn_is_tmpOP(conn_get_channel(c),dst_c))
	{
	    conn_set_tmpOP_channel(dst_c,NULL);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been demoted from a tempOP of this channel",username);
	    message_send_text(c, message_type_info, c, msgtemp);
	    if (dst_c)
	    {
	      snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has demoted you from a tmpOP of channel \"%.128s\"",conn_get_loggeduser(c),channel);
              message_send_text(dst_c, message_type_info, c, msgtemp2);
	    }
	    done = 1;
	}
	if (!done) {
	  snprintf(msgtemp, sizeof(msgtemp), "%.64s is no Channel Admin or Channel Operator or tempOP, so you can't demote him.",username);
	  message_send_text(c, message_type_info, c, msgtemp);
	}
      }
    else //user is just a tempOP and may only deOP other tempOPs
      {
	if (dst_c && channel_conn_is_tmpOP(conn_get_channel(c),dst_c))
	  {
	    conn_set_tmpOP_channel(account_get_conn(acc),NULL);
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s has been demoted from a tempOP of this channel",username);
	    message_send_text(c, message_type_info, c, msgtemp);
	    snprintf(msgtemp2, sizeof(msgtemp2), "%.64s has demoted you from a tempOP of channel \"%.128s\"",conn_get_loggeduser(c),channel);
            if (dst_c) message_send_text(dst_c, message_type_info, c, msgtemp2);
	  }
	else
	  {
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s is no tempOP in this channel, so you can't demote him",username);
	    message_send_text(c, message_type_info, c, msgtemp);
	  }
      }

    command_set_flags(connlist_find_connection_by_accountname(username));
    return 0;
}

static int _handle_friends_command(t_connection * c, char const * text)
{
    int i;
    t_account *my_acc = conn_get_account(c);

    text = skip_command(text);;

    if (strstart(text,"add")==0 || strstart(text,"a")==0) {
	char msgtemp[MAX_MESSAGE_LEN];
	t_packet 	* rpacket;
	t_connection 	* dest_c;
	t_account    	* friend_acc;
	t_server_friendslistreply_status status;
	t_game * game;
	t_channel * channel;
        char stat;
	t_list * flist;
	t_friend * fr;

	text = skip_command(text);

	if (text[0] == '\0') {
	    message_send_text(c,message_type_info,c,"usage: /f add <username>");
	    return 0;
	}

	if (!(friend_acc = accountlist_find_account(text))) {
	    message_send_text(c,message_type_info,c,"That user does not exist.");
	    return 0;
	}

	switch(account_add_friend(my_acc, friend_acc)) {
	    case -1:
    		message_send_text(c,message_type_error,c,"Server error.");
    		return 0;
	    case -2:
    		message_send_text(c,message_type_info,c,"You can't add yourself to your friends list.");
    		return 0;
	    case -3:
    		snprintf(msgtemp, sizeof(msgtemp), "You can only have a maximum of %d friends.", prefs_get_max_friends());
    		message_send_text(c,message_type_info,c,msgtemp);
    		return 0;
	    case -4:
		snprintf(msgtemp, sizeof(msgtemp), "%.64s is already on your friends list!", text);
		message_send_text(c,message_type_info,c,msgtemp);
		return 0;
	}

	snprintf(msgtemp, sizeof(msgtemp), "Added %.64s to your friends list.", text);
	message_send_text(c,message_type_info,c,msgtemp);
	dest_c = connlist_find_connection_by_account(friend_acc);
	if(dest_c!=NULL) {
    	    snprintf(msgtemp, sizeof(msgtemp), "%.64s added you to his/her friends list.",conn_get_username(c));
    	    message_send_text(dest_c,message_type_info,dest_c,msgtemp);
	}

	if ((conn_get_class(c)!=conn_class_bnet) || (!(rpacket = packet_create(packet_class_bnet))))
    	    return 0;

	packet_set_size(rpacket,sizeof(t_server_friendadd_ack));
	packet_set_type(rpacket,SERVER_FRIENDADD_ACK);

	packet_append_string(rpacket, account_get_name(friend_acc));

	game = NULL;
        channel = NULL;

	if(!(dest_c))
          {
	    bn_byte_set(&status.location,FRIENDSTATUS_OFFLINE);
	    bn_byte_set(&status.status,0);
            bn_int_set(&status.clienttag,0);
          }
	  else
          {
	    bn_int_set(&status.clienttag, conn_get_clienttag(dest_c));
            stat = 0;
	    flist = account_get_friends(my_acc);
	    fr = friendlist_find_account(flist,friend_acc);
            if ((friend_get_mutual(fr)))    stat |= FRIEND_TYPE_MUTUAL;
            if ((conn_get_dndstr(dest_c)))  stat |= FRIEND_TYPE_DND;
	    if ((conn_get_awaystr(dest_c))) stat |= FRIEND_TYPE_AWAY;
	    bn_byte_set(&status.status,stat);
            if((game = conn_get_game(dest_c)))
	      {
	        if (game_get_flag(game) != game_flag_private)
		  bn_byte_set(&status.location,FRIENDSTATUS_PUBLIC_GAME);
		else
                  bn_byte_set(&status.location,FRIENDSTATUS_PRIVATE_GAME);
	      }
	    else if((channel = conn_get_channel(dest_c)))
	      {
	        bn_byte_set(&status.location,FRIENDSTATUS_CHAT);
	      }
	    else
	      {
		bn_byte_set(&status.location,FRIENDSTATUS_ONLINE);
	      }
          }

	packet_append_data(rpacket, &status, sizeof(status));

        if (game) packet_append_string(rpacket,game_get_name(game));
	else if (channel) packet_append_string(rpacket,channel_get_name(channel));
	else packet_append_string(rpacket,"");

	conn_push_outqueue(c,rpacket);
	packet_del_ref(rpacket);
    } else if (strstart(text,"msg")==0 || strstart(text,"w")==0 || strstart(text,"whisper")==0 || strstart(text,"m")==0)
    {
	char const *msg;
	int cnt = 0;
	t_connection * dest_c;
	t_elem  * curr;
	t_friend * fr;
	t_list  * flist;

	msg = skip_command(text);
	/* if the message test is empty then ignore command */
	if (msg[0]=='\0') {
	    message_send_text(c,message_type_info,c,"Did not message any friends. Type some text next time.");
	    return 0;
	}

	flist=account_get_friends(my_acc);
	if(flist==NULL)
    	    return -1;

	LIST_TRAVERSE(flist,curr)
	{
    	    if (!(fr = (t_friend*)elem_get_data(curr))) {
        	eventlog(eventlog_level_error,__FUNCTION__,"found NULL entry in list");
        	continue;
    	    }
    	    if(friend_get_mutual(fr)) {
        	dest_c = connlist_find_connection_by_account(friend_get_account(fr));
		if (!dest_c) continue;
        	message_send_text(dest_c,message_type_whisper,c,msg);
        	cnt++;
    	    }
	}
	if(cnt)
	    message_send_text(c,message_type_friendwhisperack,c,msg);
        else
	    message_send_text(c,message_type_info,c,"All your friends are offline.");
    } else if (strstart(text,"r")==0 || strstart(text,"remove")==0
	|| strstart(text,"del")==0 || strstart(text,"delete")==0) {

	int num;
	char msgtemp[MAX_MESSAGE_LEN];
	t_packet * rpacket;

	text = skip_command(text);

	if (text[0]=='\0') {
	    message_send_text(c,message_type_info,c,"usage: /f remove <username>");
	    return 0;
	}

	switch((num = account_remove_friend2(my_acc, text))) {
	    case -1: return -1;
	    case -2:
		snprintf(msgtemp, sizeof(msgtemp), "%.64s was not found on your friends list.", text);
		message_send_text(c,message_type_info,c,msgtemp);
		return 0;
	    default:
		snprintf(msgtemp, sizeof(msgtemp), "Removed %.64s from your friends list.", text);
		message_send_text(c,message_type_info,c,msgtemp);

		if ((conn_get_class(c)!=conn_class_bnet) || (!(rpacket = packet_create(packet_class_bnet))))
	    	    return 0;

		packet_set_size(rpacket,sizeof(t_server_frienddel_ack));
		packet_set_type(rpacket,SERVER_FRIENDDEL_ACK);

		bn_byte_set(&rpacket->u.server_frienddel_ack.friendnum, num);

		conn_push_outqueue(c,rpacket);
		packet_del_ref(rpacket);

    		return 0;
	}
    } else if (strstart(text,"p")==0 || strstart(text,"promote")==0) {
	int num;
	int n;
	char msgtemp[MAX_MESSAGE_LEN];
	char const * dest_name;
	t_packet * rpacket;
	t_list * flist;
	t_friend * fr;
	t_account * dest_acc;
	unsigned int dest_uid;

	text = skip_command(text);

	if (text[0]=='\0') {
	    message_send_text(c,message_type_info,c,"usage: /f promote <username>");
	    return 0;
	}

	num = account_get_friendcount(my_acc);
	flist = account_get_friends(my_acc);
	for(n = 1; n<num; n++)
	    if( (dest_uid = account_get_friend(my_acc, n)) &&
		(fr = friendlist_find_uid(flist, dest_uid)) &&
		(dest_acc = friend_get_account(fr)) &&
		(dest_name = account_get_name(dest_acc)) &&
		(strcasecmp(dest_name, text) == 0) )
	    {
		account_set_friend(my_acc, n, account_get_friend(my_acc, n-1));
		account_set_friend(my_acc, n-1, dest_uid);
		snprintf(msgtemp, sizeof(msgtemp), "Premoted %.64s in your friends list.", dest_name);
		message_send_text(c,message_type_info,c,msgtemp);

		if ((conn_get_class(c)!=conn_class_bnet) || (!(rpacket = packet_create(packet_class_bnet))))
	    	    return 0;

		packet_set_size(rpacket,sizeof(t_server_friendmove_ack));
		packet_set_type(rpacket,SERVER_FRIENDMOVE_ACK);
    		bn_byte_set(&rpacket->u.server_friendmove_ack.pos1, n-1);
    		bn_byte_set(&rpacket->u.server_friendmove_ack.pos2, n);

		conn_push_outqueue(c,rpacket);
		packet_del_ref(rpacket);
		return 0;
	    }
    } else if (strstart(text,"d")==0 || strstart(text,"demote")==0) {
	int num;
	int n;
	char msgtemp[MAX_MESSAGE_LEN];
	char const * dest_name;
	t_packet * rpacket;
	t_list * flist;
	t_friend * fr;
	t_account * dest_acc;
	unsigned int dest_uid;

	text = skip_command(text);

	if (text[0]=='\0') {
	    message_send_text(c,message_type_info,c,"usage: /f demote <username>");
	    return 0;
	}

	num = account_get_friendcount(my_acc);
	flist = account_get_friends(my_acc);
	for(n = 0; n<num-1; n++)
	    if( (dest_uid = account_get_friend(my_acc, n)) &&
		(fr = friendlist_find_uid(flist, dest_uid)) &&
		(dest_acc = friend_get_account(fr)) &&
		(dest_name = account_get_name(dest_acc)) &&
		(strcasecmp(dest_name, text) == 0) )
	    {
		account_set_friend(my_acc, n, account_get_friend(my_acc, n+1));
		account_set_friend(my_acc, n+1, dest_uid);
		snprintf(msgtemp, sizeof(msgtemp), "Premoted %.64s in your friends list.", dest_name);
		message_send_text(c,message_type_info,c,msgtemp);

		if ((conn_get_class(c)!=conn_class_bnet) || (!(rpacket = packet_create(packet_class_bnet))))
	    	    return 0;

		packet_set_size(rpacket,sizeof(t_server_friendmove_ack));
		packet_set_type(rpacket,SERVER_FRIENDMOVE_ACK);
    		bn_byte_set(&rpacket->u.server_friendmove_ack.pos1, n);
    		bn_byte_set(&rpacket->u.server_friendmove_ack.pos2, n+1);

		conn_push_outqueue(c,rpacket);
		packet_del_ref(rpacket);
		return 0;
	    }
    } else if (strstart(text,"list")==0 || strstart(text,"l")==0) {
	char const * frienduid;
	char status[128];
	char software[64];
	char msgtemp[MAX_MESSAGE_LEN];
	t_connection * dest_c;
	t_account * friend_acc;
	t_game const * game;
	t_channel const * channel;
	t_friend * fr;
	t_list  * flist;
	int num;
	unsigned int uid;

	message_send_text(c,message_type_info,c,"Your PvPGN - Friends List");
	message_send_text(c,message_type_info,c,"=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
	num = account_get_friendcount(my_acc);

	flist=account_get_friends(my_acc);
	if(flist!=NULL) {
	    for (i=0;i<num;i++)
	    {
    		if ((!(uid = account_get_friend(my_acc,i))) || (!(fr = friendlist_find_uid(flist,uid))))
    		{
        	    eventlog(eventlog_level_error,__FUNCTION__,"friend uid in list");
        	    continue;
    		}
    		software[0]='\0';
    		friend_acc=friend_get_account(fr);
		if (!(dest_c = connlist_find_connection_by_account(friend_acc)))
	    	    std::sprintf(status, ", offline");
		else {
	    	    std::sprintf(software," using %s", clienttag_get_title(conn_get_clienttag(dest_c)));

	    	    if(friend_get_mutual(fr)) {
		        if ((game = conn_get_game(dest_c)))
		            std::sprintf(status, ", in game \"%.64s\"", game_get_name(game));
		        else if ((channel = conn_get_channel(dest_c))) {
		            if(strcasecmp(channel_get_name(channel),"Arranged Teams")==0)
		                std::sprintf(status, ", in game AT Preparation");
		            else
		                std::sprintf(status, ", in channel \"%.64s\",", channel_get_name(channel));
		    	    }
		        else
		            std::sprintf(status, ", is in AT Preparation");
	    	    } else {
		        if ((game = conn_get_game(dest_c)))
		            std::sprintf(status, ", is in a game");
		        else if ((channel = conn_get_channel(dest_c)))
		            std::sprintf(status, ", is in a chat channel");
		        else
		            std::sprintf(status, ", is in AT Preparation");
	    	    }
		}

		frienduid=account_get_name(friend_acc);
    		if (software[0]) snprintf(msgtemp, sizeof(msgtemp), "%d: %s%.16s%.128s, %.64s", i+1, friend_get_mutual(fr)?"*":" ", frienduid, status,software);
		else snprintf(msgtemp, sizeof(msgtemp), "%d: %.16s%.128s", i+1, frienduid, status);
		message_send_text(c,message_type_info,c,msgtemp);
	    }
	}
	message_send_text(c,message_type_info,c,"=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
	message_send_text(c,message_type_info,c,"End of Friends List");
    } else {
	message_send_text(c,message_type_info,c,"Friends List (Used in Arranged Teams and finding online friends.)");
	message_send_text(c,message_type_info,c,"Type: /f add <username> (adds a friend to your list)");
	message_send_text(c,message_type_info,c,"Type: /f del <username> (removes a friend from your list)");
	message_send_text(c,message_type_info,c,"Type: /f promote <username> (promote a friend in your list)");
	message_send_text(c,message_type_info,c,"Type: /f demote <username> (demote a friend in your list)");
	message_send_text(c,message_type_info,c,"Type: /f list (shows your full friends list)");
	message_send_text(c,message_type_info,c,"Type: /f msg (whispers a message to all your friends at once)");
    }

    return 0;
}

static int _handle_me_command(t_connection * c, char const * text)
{
  t_channel const * channel;

  if (!(channel = conn_get_channel(c)))
    {
      message_send_text(c,message_type_error,c,"You are not in a channel.");
      return 0;
    }

  text = skip_command(text);

  if ((text[0]!='\0') && (!conn_quota_exceeded(c,text)))
    channel_message_send(channel,message_type_emote,c,text);
  return 0;
}

static int _handle_whisper_command(t_connection * c, char const *text)
{
  char         dest[MAX_USERNAME_LEN+MAX_REALMNAME_LEN]; /* both include NUL, so no need to add one for middle @ or * */
  unsigned int i,j;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  if ((dest[0]=='\0') || (text[i]=='\0'))
    {
      message_send_text(c,message_type_info,c,"usage: /whisper <username> <text to whisper>");
      return 0;
    }

  do_whisper(c,dest,&text[i]);

  return 0;
}

static int _handle_status_command(t_connection * c, char const *text)
{
    char ctag[5];
    unsigned int i,j;
    t_clienttag clienttag;

    for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
    for (; text[i]==' '; i++);
    for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get clienttag */
	if (j<sizeof(ctag)-1) ctag[j++] = text[i];
    ctag[j] = '\0';

    if (ctag[0]=='\0') {
	snprintf(msgtemp, sizeof(msgtemp), "There are currently %d users online, in %d games and %d channels.",
	    connlist_login_get_length(),
	    gamelist_get_length(),
	    channellist_get_length());
	message_send_text(c,message_type_info,c,msgtemp);
	tag_uint_to_str(ctag,conn_get_clienttag(c));
    }

    for (i=0; i<std::strlen(ctag); i++)
	if (isascii((int)ctag[i]) && std::islower((int)ctag[i]))
	    ctag[i] = std::toupper((int)ctag[i]);

    if (std::strcmp(ctag,"ALL") == 0)
	clienttag = 0;
    else
	clienttag = tag_case_str_to_uint(ctag);

    switch (clienttag)
    {
	case 0:
	case CLIENTTAG_WAR3XP_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(CLIENTTAG_WAR3XP_UINT),
		game_get_count_by_clienttag(CLIENTTAG_WAR3XP_UINT),
		clienttag_get_title(CLIENTTAG_WAR3XP_UINT));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (clienttag) break;
	case CLIENTTAG_WARCRAFT3_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(CLIENTTAG_WARCRAFT3_UINT),
		game_get_count_by_clienttag(CLIENTTAG_WARCRAFT3_UINT),
		clienttag_get_title(CLIENTTAG_WARCRAFT3_UINT));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (clienttag) break;
	case CLIENTTAG_DIABLO2XP_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(CLIENTTAG_DIABLO2XP_UINT),
		game_get_count_by_clienttag(CLIENTTAG_DIABLO2XP_UINT),
		clienttag_get_title(CLIENTTAG_DIABLO2XP_UINT));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (clienttag) break;
	case CLIENTTAG_DIABLO2DV_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(CLIENTTAG_DIABLO2DV_UINT),
		game_get_count_by_clienttag(CLIENTTAG_DIABLO2DV_UINT),
		clienttag_get_title(CLIENTTAG_DIABLO2DV_UINT));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (clienttag) break;
	case CLIENTTAG_BROODWARS_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(CLIENTTAG_BROODWARS_UINT),
		game_get_count_by_clienttag(CLIENTTAG_BROODWARS_UINT),
		clienttag_get_title(CLIENTTAG_BROODWARS_UINT));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (clienttag) break;
	case CLIENTTAG_STARCRAFT_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(CLIENTTAG_STARCRAFT_UINT),
		game_get_count_by_clienttag(CLIENTTAG_STARCRAFT_UINT),
		clienttag_get_title(CLIENTTAG_STARCRAFT_UINT));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (clienttag) break;
	case CLIENTTAG_WARCIIBNE_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(CLIENTTAG_WARCIIBNE_UINT),
		game_get_count_by_clienttag(CLIENTTAG_WARCIIBNE_UINT),
		clienttag_get_title(CLIENTTAG_WARCIIBNE_UINT));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (clienttag) break;
	case CLIENTTAG_DIABLORTL_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(CLIENTTAG_DIABLORTL_UINT),
		game_get_count_by_clienttag(CLIENTTAG_DIABLORTL_UINT),
		clienttag_get_title(CLIENTTAG_DIABLORTL_UINT));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (clienttag) break;
	default:
	    snprintf(msgtemp, sizeof(msgtemp), "There are currently %u user(s) in %u games of %.128s",
		conn_get_user_count_by_clienttag(conn_get_clienttag(c)),
		game_get_count_by_clienttag(conn_get_clienttag(c)),
		clienttag_get_title(conn_get_clienttag(c)));
	    message_send_text(c,message_type_info,c,msgtemp);
    }

    return 0;
}

static int _handle_who_command(t_connection * c, char const *text)
{
  t_connection const * conn;
  t_channel const *    channel;
  unsigned int         i;
  char const *         tname;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
  {
	message_send_text(c,message_type_info,c,"usage: /who <channel>");
	return 0;
  }

  if (!(channel = channellist_find_channel_by_name(&text[i],conn_get_country(c),realm_get_name(conn_get_realm(c)))))
    {
      message_send_text(c,message_type_error,c,"That channel does not exist.");
      message_send_text(c,message_type_error,c,"(If you are trying to search for a user, use the /whois command.)");
      return 0;
    }
  if (channel_check_banning(channel,c)==1)
    {
      message_send_text(c,message_type_error,c,"You are banned from that channel.");
      return 0;
    }

  snprintf(msgtemp, sizeof(msgtemp), "Users in channel %.64s:",&text[i]);
  i = std::strlen(msgtemp);
  for (conn=channel_get_first(channel); conn; conn=channel_get_next())
    {
      if (i+std::strlen((tname = conn_get_username(conn)))+2>sizeof(msgtemp)) /* " ", name, '\0' */
	{
	  message_send_text(c,message_type_info,c,msgtemp);
	  i = 0;
	}
      std::sprintf(&msgtemp[i]," %s",tname);
      i += std::strlen(&msgtemp[i]);
    }
  if (i>0)
    message_send_text(c,message_type_info,c,msgtemp);

  return 0;
}

static int _handle_whois_command(t_connection * c, char const * text)
{
  unsigned int i;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
  {
    message_send_text(c,message_type_info,c,"usage: /whois <username>");
    return 0;
  }

  do_whois(c,&text[i]);

  return 0;
}

static int _handle_whoami_command(t_connection * c, char const *text)
{
  char const * tname;

  if (!(tname = conn_get_username(c)))
    {
      message_send_text(c,message_type_error,c,"Unable to obtain your account name.");
      return 0;
    }

  do_whois(c,tname);

  return 0;
}

static int _handle_announce_command(t_connection * c, char const *text)
{
  unsigned int i;
  t_message *  message;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
  {
	message_send_text(c,message_type_info,c,"usage: /announce <announcement>");
	return 0;
  }

  snprintf(msgtemp, sizeof(msgtemp), "Announcement from %.64s: %.128s",conn_get_username(c),&text[i]);
  if (!(message = message_create(message_type_broadcast,c,NULL,msgtemp)))
    message_send_text(c,message_type_info,c,"Could not broadcast message.");
  else
    {
      if (message_send_all(message)<0)
	message_send_text(c,message_type_info,c,"Could not broadcast message.");
      message_destroy(message);
    }

  return 0;
}

static int _handle_beep_command(t_connection * c, char const *text)
{
  message_send_text(c,message_type_info,c,"Audible notification on."); /* FIXME: actually do something */
  return 0; /* FIXME: these only affect CHAT clients... I think they prevent ^G from being sent */
}

static int _handle_nobeep_command(t_connection * c, char const *text)
{
  message_send_text(c,message_type_info,c,"Audible notification off."); /* FIXME: actually do something */
  return 0;
}

static int _handle_version_command(t_connection * c, char const *text)
{
  message_send_text(c,message_type_info,c,PVPGN_SOFTWARE" "PVPGN_VERSION);
  return 0;
}

static int _handle_copyright_command(t_connection * c, char const *text)
{
  static char const * const info[] =
    {
      " Copyright (C) 2002  See source for details",
      " ",
      " PvPGN is free software; you can redistribute it and/or",
      " modify it under the terms of the GNU General Public License",
      " as published by the Free Software Foundation; either version 2",
      " of the License, or (at your option) any later version.",
      " ",
      " This program is distributed in the hope that it will be useful,",
      " but WITHOUT ANY WARRANTY; without even the implied warranty of",
      " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the",
      " GNU General Public License for more details.",
      " ",
      " You should have received a copy of the GNU General Public License",
      " along with this program; if not, write to the Free Software",
      " Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.",
      NULL
    };
  unsigned int i;

  for (i=0; info[i]; i++)
    message_send_text(c,message_type_info,c,info[i]);

  return 0;
}

static int _handle_uptime_command(t_connection * c, char const *text)
{

  snprintf(msgtemp, sizeof(msgtemp), "Uptime: %s",seconds_to_timestr(server_get_uptime()));
  message_send_text(c,message_type_info,c,msgtemp);

  return 0;
}

static int _handle_stats_command(t_connection * c, char const *text)
{
    char         dest[MAX_USERNAME_LEN];
    unsigned int i,j;
    t_account *  account;
    char const * clienttag=NULL;
    t_clienttag  clienttag_uint;
    char         clienttag_str[5];

    for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
    for (; text[i]==' '; i++);
    for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
	if (j<sizeof(dest)-1) dest[j++] = text[i];
    dest[j] = '\0';
    for (; text[i]==' '; i++);

    if (!dest[0]) {
	account = conn_get_account(c);
    } else if (!(account = accountlist_find_account(dest))) {
	message_send_text(c,message_type_error,c,"Invalid user.");
	return 0;
    }

    if (text[i]!='\0')
	clienttag = &text[i];
    else if (!(clienttag = tag_uint_to_str(clienttag_str,conn_get_clienttag(c)))) {
	message_send_text(c,message_type_error,c,"Unable to determine client game.");
	return 0;
    }

    if (std::strlen(clienttag)!=4) {
	snprintf(msgtemp, sizeof(msgtemp), "You must supply a user name and a valid program ID. (Program ID \"%.32s\" is invalid.)",clienttag);
	message_send_text(c,message_type_error,c,msgtemp);
	message_send_text(c,message_type_error,c,"Example: /stats joe STAR");
	return 0;
    }

    clienttag_uint = tag_case_str_to_uint(clienttag);

    switch (clienttag_uint)
    {
	case CLIENTTAG_BNCHATBOT_UINT:
	    message_send_text(c,message_type_error,c,"This game does not support win/loss records.");
	    message_send_text(c,message_type_error,c,"You must supply a user name and a valid program ID.");
	    message_send_text(c,message_type_error,c,"Example: /stats joe STAR");
	    return 0;
	case CLIENTTAG_DIABLORTL_UINT:
	case CLIENTTAG_DIABLOSHR_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s's record:",account_get_name(account));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "level: %u",account_get_normal_level(account,clienttag_uint));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "class: %.16s",bnclass_get_str(account_get_normal_class(account,clienttag_uint)));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "stats: %u str  %u mag  %u dex  %u vit  %u gld",
		account_get_normal_strength(account,clienttag_uint),
		account_get_normal_magic(account,clienttag_uint),
		account_get_normal_dexterity(account,clienttag_uint),
		account_get_normal_vitality(account,clienttag_uint),
		account_get_normal_gold(account,clienttag_uint));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "Diablo kills: %u",account_get_normal_diablo_kills(account,clienttag_uint));
	    message_send_text(c,message_type_info,c,msgtemp);
	    return 0;
	case CLIENTTAG_WARCIIBNE_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s's record:",account_get_name(account));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "Normal games: %u-%u-%u",
		account_get_normal_wins(account,clienttag_uint),
		account_get_normal_losses(account,clienttag_uint),
		account_get_normal_disconnects(account,clienttag_uint));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (account_get_ladder_rating(account,clienttag_uint,ladder_id_normal)>0)
		snprintf(msgtemp, sizeof(msgtemp), "Ladder games: %u-%u-%u (rating %d)",
		    account_get_ladder_wins(account,clienttag_uint,ladder_id_normal),
		    account_get_ladder_losses(account,clienttag_uint,ladder_id_normal),
		    account_get_ladder_disconnects(account,clienttag_uint,ladder_id_normal),
		    account_get_ladder_rating(account,clienttag_uint,ladder_id_normal));
	    else
		std::strcpy(msgtemp,"Ladder games: 0-0-0");
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (account_get_ladder_rating(account,clienttag_uint,ladder_id_ironman)>0)
		snprintf(msgtemp, sizeof(msgtemp), "IronMan games: %u-%u-%u (rating %d)",
		    account_get_ladder_wins(account,clienttag_uint,ladder_id_ironman),
		    account_get_ladder_losses(account,clienttag_uint,ladder_id_ironman),
		    account_get_ladder_disconnects(account,clienttag_uint,ladder_id_ironman),
		    account_get_ladder_rating(account,clienttag_uint,ladder_id_ironman));
	    else
		std::strcpy(msgtemp,"IronMan games: 0-0-0");
	    message_send_text(c,message_type_info,c,msgtemp);
	    return 0;
	case CLIENTTAG_WARCRAFT3_UINT:
	case CLIENTTAG_WAR3XP_UINT:
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s's Ladder Record's:",account_get_name(account));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "Users Solo Level: %u, Experience: %u",
		account_get_ladder_level(account,clienttag_uint,ladder_id_solo),
		account_get_ladder_xp(account,clienttag_uint,ladder_id_solo));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "SOLO Ladder Record: %u-%u-0",
		account_get_ladder_wins(account,clienttag_uint,ladder_id_solo),
		account_get_ladder_losses(account,clienttag_uint,ladder_id_solo));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "SOLO Rank: %u",
		account_get_ladder_rank(account,clienttag_uint,ladder_id_solo));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "Users Team Level: %u, Experience: %u",
		account_get_ladder_level(account,clienttag_uint,ladder_id_team),
		account_get_ladder_xp(account,clienttag_uint,ladder_id_team));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "TEAM Ladder Record: %u-%u-0",
		account_get_ladder_wins(account,clienttag_uint,ladder_id_team),
		account_get_ladder_losses(account,clienttag_uint,ladder_id_team));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "TEAM Rank: %u",
		account_get_ladder_rank(account,clienttag_uint,ladder_id_team));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "Users FFA Level: %u, Experience: %u",
		account_get_ladder_level(account,clienttag_uint,ladder_id_ffa),
		account_get_ladder_xp(account,clienttag_uint,ladder_id_ffa));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "FFA Ladder Record: %u-%u-0",
		account_get_ladder_wins(account,clienttag_uint,ladder_id_ffa),
		account_get_ladder_losses(account,clienttag_uint,ladder_id_ffa));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "FFA Rank: %u",
		account_get_ladder_rank(account,clienttag_uint,ladder_id_ffa));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (account_get_teams(account)) {
		t_elem * curr;
		t_list * list;
		t_team * team;
		int teamcount = 0;

		list = account_get_teams(account);

		LIST_TRAVERSE(list,curr)
		{
	    	  if (!(team = (t_team*)elem_get_data(curr)))
	    	  {
	      	    eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
	      	    continue;
	    	    }

	          if (team_get_clienttag(team) != clienttag_uint)
	            continue;

		    teamcount++;
		    snprintf(msgtemp, sizeof(msgtemp), "Users AT Team No. %u",teamcount);
		    message_send_text(c,message_type_info,c,msgtemp);
		    snprintf(msgtemp, sizeof(msgtemp), "Users AT TEAM Level: %u, Experience: %u",
			team_get_level(team),team_get_xp(team));
		    message_send_text(c,message_type_info,c,msgtemp);
		    snprintf(msgtemp, sizeof(msgtemp), "AT TEAM Ladder Record: %u-%u-0",
			team_get_wins(team),team_get_losses(team));
		    message_send_text(c,message_type_info,c,msgtemp);
		    snprintf(msgtemp, sizeof(msgtemp), "AT TEAM Rank: %u",
			team_get_rank(team));
		    message_send_text(c,message_type_info,c,msgtemp);
		}
	    }
	    return 0;
	default:
	    snprintf(msgtemp, sizeof(msgtemp), "%.64s's record:",account_get_name(account));
	    message_send_text(c,message_type_info,c,msgtemp);
	    snprintf(msgtemp, sizeof(msgtemp), "Normal games: %u-%u-%u",
		account_get_normal_wins(account,clienttag_uint),
		account_get_normal_losses(account,clienttag_uint),
		account_get_normal_disconnects(account,clienttag_uint));
	    message_send_text(c,message_type_info,c,msgtemp);
	    if (account_get_ladder_rating(account,clienttag_uint,ladder_id_normal)>0)
		snprintf(msgtemp, sizeof(msgtemp), "Ladder games: %u-%u-%u (rating %d)",
		    account_get_ladder_wins(account,clienttag_uint,ladder_id_normal),
		    account_get_ladder_losses(account,clienttag_uint,ladder_id_normal),
		    account_get_ladder_disconnects(account,clienttag_uint,ladder_id_normal),
		    account_get_ladder_rating(account,clienttag_uint,ladder_id_normal));
	    else
		std::strcpy(msgtemp,"Ladder games: 0-0-0");
	    message_send_text(c,message_type_info,c,msgtemp);
	    return 0;
    }
}

static int _handle_time_command(t_connection * c, char const *text)
{
  t_bnettime  btsystem;
  t_bnettime  btlocal;
  std::time_t      now;
  struct std::tm * tmnow;

  btsystem = bnettime();

  /* Battle.net time: Wed Jun 23 15:15:29 */
  btlocal = bnettime_add_tzbias(btsystem,local_tzbias());
  now = bnettime_to_time(btlocal);
  if (!(tmnow = std::gmtime(&now)))
    std::strcpy(msgtemp,"PvPGN Server Time: ?");
  else
    std::strftime(msgtemp,sizeof(msgtemp),"PvPGN Server Time: %a %b %d %H:%M:%S",tmnow);
  message_send_text(c,message_type_info,c,msgtemp);
  if (conn_get_class(c)==conn_class_bnet)
    {
      btlocal = bnettime_add_tzbias(btsystem,conn_get_tzbias(c));
      now = bnettime_to_time(btlocal);
      if (!(tmnow = std::gmtime(&now)))
	std::strcpy(msgtemp,"Your local time: ?");
      else
	std::strftime(msgtemp,sizeof(msgtemp),"Your local time: %a %b %d %H:%M:%S",tmnow);
      message_send_text(c,message_type_info,c,msgtemp);
    }

  return 0;
}

static int _handle_channel_command(t_connection * c, char const *text)
 {
   t_channel * channel;

   text = skip_command(text);

   if (text[0]=='\0')
     {
       message_send_text(c,message_type_info,c,"usage /channel <channel>");
       return 0;
     }

   if (!conn_get_game(c)) {
	if(strcasecmp(text,"Arranged Teams")==0)
	{
	   message_send_text(c,message_type_error,c,"Channel Arranged Teams is a RESTRICTED Channel!");
	   return 0;
	}

	if (!(std::strlen(text) < MAX_CHANNELNAME_LEN))
	{
    		snprintf(msgtemp, sizeof(msgtemp), "max channel name length exceeded (max %d symbols)", MAX_CHANNELNAME_LEN-1);
		message_send_text(c,message_type_error,c,msgtemp);
		return 0;
	}

	if ((channel = conn_get_channel(c)) && (strcasecmp(channel_get_name(channel),text)==0))
		return 0; // we don't have to do anything, we are allready in this channel

	if (conn_set_channel(c,text)<0)
		conn_set_channel(c,CHANNEL_NAME_BANNED); /* should not fail */
	if ((conn_get_clienttag(c) == CLIENTTAG_WARCRAFT3_UINT) || (conn_get_clienttag(c) == CLIENTTAG_WAR3XP_UINT))
		conn_update_w3_playerinfo(c);
	command_set_flags(c);
   } else
   	message_send_text(c,message_type_error,c,"Command disabled while inside a game.");

   return 0;
 }

static int _handle_rejoin_command(t_connection * c, char const *text)
{

  if (channel_rejoin(c)!=0)
      message_send_text(c,message_type_error,c,"You are not in a channel.");
  if ((conn_get_clienttag(c) == CLIENTTAG_WARCRAFT3_UINT) || (conn_get_clienttag(c) ==  CLIENTTAG_WAR3XP_UINT))
    conn_update_w3_playerinfo(c);
  command_set_flags(c);

  return 0;
}

static int _handle_away_command(t_connection * c, char const *text)
{

  text = skip_command(text);

  if (text[0]=='\0') /* toggle away mode */
    {
      if (!conn_get_awaystr(c))
      {
	message_send_text(c,message_type_info,c,"You are now marked as being away.");
	conn_set_awaystr(c,"Currently not available");
      }
      else
      {
        message_send_text(c,message_type_info,c,"You are no longer marked as away.");
        conn_set_awaystr(c,NULL);
      }
    }
  else
    {
      message_send_text(c,message_type_info,c,"You are now marked as being away.");
      conn_set_awaystr(c,text);
    }

  return 0;
}

static int _handle_dnd_command(t_connection * c, char const *text)
{

  text = skip_command(text);

  if (text[0]=='\0') /* toggle dnd mode */
    {
      if (!conn_get_dndstr(c))
      {
	message_send_text(c,message_type_info,c,"Do Not Diturb mode engaged.");
	conn_set_dndstr(c,"Not available");
      }
      else
      {
        message_send_text(c,message_type_info,c,"Do Not Disturb mode cancelled.");
        conn_set_dndstr(c,NULL);
      }
    }
  else
    {
      message_send_text(c,message_type_info,c,"Do Not Disturb mode engaged.");
      conn_set_dndstr(c,text);
    }

  return 0;
}

static int _handle_squelch_command(t_connection * c, char const *text)
{
  t_account *  account;

  text = skip_command(text);

  /* D2 std::puts * before username */
  if (text[0]=='*')
    text++;

  if (text[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /squelch <username>");
      return 0;
    }

  if (!(account = accountlist_find_account(text)))
    {
      message_send_text(c,message_type_error,c,"No such user.");
      return 0;
    }

  if (conn_get_account(c)==account)
    {
      message_send_text(c,message_type_error,c,"You can't squelch yourself.");
      return 0;
    }

  if (conn_add_ignore(c,account)<0)
    message_send_text(c,message_type_error,c,"Could not squelch user.");
  else
    {
      snprintf(msgtemp, sizeof(msgtemp), "%-.20s has been squelched.",account_get_name(account));
      message_send_text(c,message_type_info,c,msgtemp);
    }

  return 0;
}

static int _handle_unsquelch_command(t_connection * c, char const *text)
{
  t_account * account;
  t_connection * dest_c;

  text = skip_command(text);

  /* D2 std::puts * before username */
  if (text[0]=='*')
    text++;

  if (text[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /unsquelch <username>");
      return 0;
    }

  if (!(account = accountlist_find_account(text)))
    {
      message_send_text(c,message_type_info,c,"No such user.");
      return 0;
    }

  if (conn_del_ignore(c,account)<0)
    message_send_text(c,message_type_info,c,"User was not being ignored.");
  else
    {
      t_message * message;

      message_send_text(c,message_type_info,c,"No longer ignoring.");

      if ((dest_c = account_get_conn(account)))
      {
        if (!(message = message_create(message_type_userflags,dest_c,NULL,NULL))) /* handles NULL text */
	    return 0;
        message_send(message,c);
        message_destroy(message);
      }
    }

  return 0;
}

static int _handle_kick_command(t_connection * c, char const *text)
{
  char              dest[MAX_USERNAME_LEN];
  unsigned int      i,j;
  t_channel const * channel;
  t_connection *    kuc;
  t_account *	    acc;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  if (dest[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /kick <username>");
      return 0;
    }

  if (!(channel = conn_get_channel(c)))
    {
      message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
      return 0;
    }

  acc = conn_get_account(c);
  if (account_get_auth_admin(acc,NULL)!=1 && /* default to false */
      account_get_auth_admin(acc,channel_get_name(channel))!=1 && /* default to false */
      account_get_auth_operator(acc,NULL)!=1 && /* default to false */
      account_get_auth_operator(acc,channel_get_name(channel))!=1 && /* default to false */
      !channel_conn_is_tmpOP(channel,account_get_conn(acc)))
    {
      message_send_text(c,message_type_error,c,"You have to be at least a Channel Operator or tempOP to use this command.");
      return 0;
    }
  if (!(kuc = connlist_find_connection_by_accountname(dest)))
    {
      message_send_text(c,message_type_error,c,"That user is not logged in.");
      return 0;
    }
  if (conn_get_channel(kuc)!=channel)
    {
      message_send_text(c,message_type_error,c,"That user is not in this channel.");
      return 0;
    }
  if (account_get_auth_admin(conn_get_account(kuc),NULL)==1 ||
    account_get_auth_admin(conn_get_account(kuc),channel_get_name(channel))==1)
    {
      message_send_text(c,message_type_error,c,"You cannot kick administrators.");
      return 0;
    }
  else if (account_get_auth_operator(conn_get_account(kuc),NULL)==1 ||
    account_get_auth_operator(conn_get_account(kuc),channel_get_name(channel))==1)
    {
      message_send_text(c,message_type_error,c,"You cannot kick operators.");
      return 0;
    }

  {
    char const * tname1;
    char const * tname2;

    tname1 = conn_get_loggeduser(kuc);
    tname2 = conn_get_loggeduser(c);
    if (!tname1 || !tname2) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL username");
	return -1;
    }

    if (text[i]!='\0')
      snprintf(msgtemp, sizeof(msgtemp), "%-.20s has been kicked by %-.20s (%.128s).",tname1,tname2,&text[i]);
    else
      snprintf(msgtemp, sizeof(msgtemp), "%-.20s has been kicked by %-.20s.",tname1,tname2);
    channel_message_send(channel,message_type_info,c,msgtemp);
  }
  conn_kick_channel(kuc,"Bye");
  if (conn_get_class(kuc) == conn_class_bnet)
      conn_set_channel(kuc,CHANNEL_NAME_KICKED); /* should not fail */

  return 0;
}

static int _handle_ban_command(t_connection * c, char const *text)
{
  char           dest[MAX_USERNAME_LEN];
  unsigned int   i,j;
  t_channel *    channel;
  t_connection * buc;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  if (dest[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage. /ban <username>");
      return 0;
    }

  if (!(channel = conn_get_channel(c)))
    {
      message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
      return 0;
    }
  if (account_get_auth_admin(conn_get_account(c),NULL)!=1 && /* default to false */
      account_get_auth_admin(conn_get_account(c),channel_get_name(channel))!=1 && /* default to false */
      account_get_auth_operator(conn_get_account(c),NULL)!=1 && /* default to false */
      account_get_auth_operator(conn_get_account(c),channel_get_name(channel))!=1) /* default to false */
    {
      message_send_text(c,message_type_error,c,"You have to be at least a Channel Operator to use this command.");
      return 0;
    }
  {
    t_account * account;

    if (!(account = accountlist_find_account(dest)))
      message_send_text(c,message_type_info,c,"That account doesn't currently exist, banning anyway.");
    else if (account_get_auth_admin(account,NULL)==1 || account_get_auth_admin(account,channel_get_name(channel))==1)
      {
        message_send_text(c,message_type_error,c,"You cannot ban administrators.");
        return 0;
      }
    else if (account_get_auth_operator(account,NULL)==1 ||
	account_get_auth_operator(account,channel_get_name(channel))==1)
      {
        message_send_text(c,message_type_error,c,"You cannot ban operators.");
        return 0;
      }
  }

  if (channel_ban_user(channel,dest)<0)
    {
      snprintf(msgtemp, sizeof(msgtemp), "Unable to ban %-.20s.",dest);
      message_send_text(c,message_type_error,c,msgtemp);
    }
  else
    {
      char const * tname;

      tname = conn_get_loggeduser(c);
      if (text[i]!='\0')
	snprintf(msgtemp, sizeof(msgtemp), "%-.20s has been banned by %-.20s (%.128s).",dest,tname?tname:"unknown",&text[i]);
      else
	snprintf(msgtemp, sizeof(msgtemp), "%-.20s has been banned by %-.20s.",dest,tname?tname:"unknown");
      channel_message_send(channel,message_type_info,c,msgtemp);
    }
  if ((buc = connlist_find_connection_by_accountname(dest)) &&
      conn_get_channel(buc)==channel)
    conn_set_channel(buc,CHANNEL_NAME_BANNED);

  return 0;
}

static int _handle_unban_command(t_connection * c, char const *text)
{
  t_channel *  channel;
  unsigned int i;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /unban <username>");
      return 0;
    }

  if (!(channel = conn_get_channel(c)))
    {
      message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
      return 0;
    }
  if (account_get_auth_admin(conn_get_account(c),NULL)!=1 && /* default to false */
      account_get_auth_admin(conn_get_account(c),channel_get_name(channel))!=1 && /* default to false */
      account_get_auth_operator(conn_get_account(c),NULL)!=1 && /* default to false */
      account_get_auth_operator(conn_get_account(c),channel_get_name(channel))!=1) /* default to false */
    {
      message_send_text(c,message_type_error,c,"You are not a channel operator.");
      return 0;
    }

  if (channel_unban_user(channel,&text[i])<0)
    message_send_text(c,message_type_error,c,"That user is not banned.");
  else
    {
      snprintf(msgtemp, sizeof(msgtemp), "%.64s is no longer banned from this channel.",&text[i]);
      message_send_text(c,message_type_info,c,msgtemp);
    }

  return 0;
}

static int _handle_reply_command(t_connection * c, char const *text)
{
  unsigned int i;
  char const * dest;

  if (!(dest = conn_get_lastsender(c)))
    {
      message_send_text(c,message_type_error,c,"No one messaged you, use /m instead");
      return 0;
    }

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /reply <replytext>");
      return 0;
    }
  do_whisper(c,dest,&text[i]);
  return 0;
}

static int _handle_realmann_command(t_connection * c, char const *text)
{
  unsigned int i;
  t_realm * realm;
  t_realm * trealm;
  t_connection * tc;
  t_elem const * curr;
  t_message    * message;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (!(realm=conn_get_realm(c))) {
    message_send_text(c,message_type_info,c,"You must join a realm first");
    return 0;
  }

  if (text[i]=='\0')
  {
    message_send_text(c,message_type_info,c,"usage: /realmann <announcement text>");
    return 0;
  }

  snprintf(msgtemp, sizeof(msgtemp), "Announcement from %.32s@%.32s: %.128s",conn_get_username(c),realm_get_name(realm),&text[i]);
  if (!(message = message_create(message_type_broadcast,c,NULL,msgtemp)))
    {
      message_send_text(c,message_type_info,c,"Could not broadcast message.");
    }
  else
    {
      LIST_TRAVERSE_CONST(connlist(),curr)
	{
	  tc = (t_connection*)elem_get_data(curr);
	  if (!tc)
	    continue;
	  if ((trealm = conn_get_realm(tc)) && (trealm==realm))
	    {
	      message_send(message,tc);
	    }
	}
    }
  return 0;
}

static int _handle_watch_command(t_connection * c, char const *text)
{
  unsigned int i;
  t_account *  account;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /watch <username>");
      return 0;
    }
  if (!(account = accountlist_find_account(&text[i])))
    {
      message_send_text(c,message_type_info,c,"That user does not exist.");
      return 0;
    }

  if (conn_add_watch(c,account,0)<0) /* FIXME: adds all events for now */
    message_send_text(c,message_type_error,c,"Add to watch list failed.");
  else
    {
      snprintf(msgtemp, sizeof(msgtemp), "User %.64s added to your watch list.",&text[i]);
      message_send_text(c,message_type_info,c,msgtemp);
    }

  return 0;
}

static int _handle_unwatch_command(t_connection * c, char const *text)
 {
   unsigned int i;
   t_account *  account;

   for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
   for (; text[i]==' '; i++);

   if (text[i]=='\0')
     {
       message_send_text(c,message_type_info,c,"usage: /unwatch <username>");
       return 0;
     }
   if (!(account = accountlist_find_account(&text[i])))
     {
       message_send_text(c,message_type_info,c,"That user does not exist.");
       return 0;
     }

   if (conn_del_watch(c,account,0)<0) /* FIXME: deletes all events for now */
     message_send_text(c,message_type_error,c,"Removal from watch list failed.");
   else
     {
       snprintf(msgtemp, sizeof(msgtemp), "User %.64s removed from your watch list.",&text[i]);
       message_send_text(c,message_type_info,c,msgtemp);
     }

   return 0;
 }

static int _handle_watchall_command(t_connection * c, char const *text)
{
    t_clienttag clienttag=0;
    char clienttag_str[5];

    text = skip_command(text);

    if(text[0] != '\0') {
	if (std::strlen(text) != 4) {
    	    message_send_text(c,message_type_error,c,"You must supply a rank and a valid program ID.");
    	    message_send_text(c,message_type_error,c,"Example: /watchall STAR");
    	    return 0;
	}
	clienttag = tag_case_str_to_uint(text);
    }

    if (conn_add_watch(c,NULL,clienttag)<0) /* FIXME: adds all events for now */
	message_send_text(c,message_type_error,c,"Add to watch list failed.");
    else
	if(clienttag) {
	    char msgtemp[MAX_MESSAGE_LEN];
	    snprintf(msgtemp, sizeof(msgtemp), "All %.128s users added to your watch list.", tag_uint_to_str(clienttag_str,clienttag));
	    message_send_text(c,message_type_info,c,msgtemp);
	}
	else
	    message_send_text(c,message_type_info,c,"All users added to your watch list.");

    return 0;
}

static int _handle_unwatchall_command(t_connection * c, char const *text)
{
    t_clienttag clienttag=0;
    char clienttag_str[5];

    text = skip_command(text);

    if(text[0] != '\0') {
	if (std::strlen(text) != 4) {
    	    message_send_text(c,message_type_error,c,"You must supply a rank and a valid program ID.");
    	    message_send_text(c,message_type_error,c,"Example: /unwatchall STAR");
	}
	clienttag = tag_case_str_to_uint(text);
    }

    if (conn_del_watch(c,NULL,clienttag)<0) /* FIXME: deletes all events for now */
	message_send_text(c,message_type_error,c,"Removal from watch list failed.");
    else
	if(clienttag) {
	    char msgtemp[MAX_MESSAGE_LEN];
	    snprintf(msgtemp, sizeof(msgtemp), "All %.128s users removed from your watch list.", tag_uint_to_str(clienttag_str,clienttag));
	    message_send_text(c,message_type_info,c,msgtemp);
	}
	else
	    message_send_text(c,message_type_info,c,"All users removed from your watch list.");

    return 0;
}

static int _handle_lusers_command(t_connection * c, char const *text)
{
  t_channel *    channel;
  t_elem const * curr;
  char const *   banned;
  unsigned int   i;

  if (!(channel = conn_get_channel(c)))
    {
      message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
      return 0;
    }

  std::strcpy(msgtemp,"Banned users:");
  i = std::strlen(msgtemp);
  LIST_TRAVERSE_CONST(channel_get_banlist(channel),curr)
    {
      banned = (char*)elem_get_data(curr);
      if (i+std::strlen(banned)+2>sizeof(msgtemp)) /* " ", name, '\0' */
	{
	  message_send_text(c,message_type_info,c,msgtemp);
	  i = 0;
	}
      std::sprintf(&msgtemp[i]," %s",banned);
      i += std::strlen(&msgtemp[i]);
    }
  if (i>0)
    message_send_text(c,message_type_info,c,msgtemp);

  return 0;
}

static int _news_cb(std::time_t date, t_lstr *lstr, void *data)
{
    char	strdate[64];
    struct std::tm 	*tm;
    char	save, *p, *q;
    t_connection *c = (t_connection*)data;

    tm = std::localtime(&date);
    if (tm) std::strftime(strdate, 64,"%B %d, %Y", tm);
    else std::strcpy(strdate, "(invalid date)");
    message_send_text(c,message_type_info,c,strdate);

    for (p = lstr_get_str(lstr); *p;) {
	for(q = p; *q && *q != '\r' && *q != '\n';q++);
	save = *q;
	*q = '\0';
	message_send_text(c,message_type_info,c,p);
	*q = save;
	p = q;
	for(;*p == '\n' || *p == '\r';p++);
    }

    return 0;
}

static int _handle_news_command(t_connection * c, char const *text)
{
    news_traverse(_news_cb,c);
    return 0;
}

struct glist_cb_struct {
    t_game_difficulty diff;
    t_clienttag tag;
    t_connection *c;
};

static int _glist_cb(t_game *game, void *data)
{
    struct glist_cb_struct *cbdata = (struct glist_cb_struct*)data;

    if ((!cbdata->tag || !prefs_get_hide_pass_games() || game_get_flag(game) != game_flag_private) &&
	(!cbdata->tag || game_get_clienttag(game)==cbdata->tag) &&
        (cbdata->diff==game_difficulty_none || game_get_difficulty(game)==cbdata->diff))
    {
	snprintf(msgtemp, sizeof(msgtemp), " %-16.16s %1.1s %-8.8s %-21.21s %5u ",
	    game_get_name(game),
	    game_get_flag(game) != game_flag_private ? "n":"y",
	    game_status_get_str(game_get_status(game)),
	    game_type_get_str(game_get_type(game)),
	    game_get_ref(game));

	if (!cbdata->tag)
	{

	  std::strcat(msgtemp,clienttag_uint_to_str(game_get_clienttag(game)));
	  std::strcat(msgtemp," ");
	}

	if ((!prefs_get_hide_addr()) || (account_get_command_groups(conn_get_account(cbdata->c)) & command_get_group("/admin-addr"))) /* default to false */
	  std::strcat(msgtemp, addr_num_to_addr_str(game_get_addr(game),game_get_port(game)));

	message_send_text(cbdata->c,message_type_info,cbdata->c,msgtemp);
    }

    return 0;
}

static int _handle_games_command(t_connection * c, char const *text)
{
  unsigned int   i;
  unsigned int   j;
  char           clienttag_str[5];
  char           dest[5];
  struct glist_cb_struct cbdata;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  cbdata.c = c;

  if(std::strcmp(&text[i],"norm")==0)
    cbdata.diff = game_difficulty_normal;
  else if(std::strcmp(&text[i],"night")==0)
    cbdata.diff = game_difficulty_nightmare;
  else if(std::strcmp(&text[i],"hell")==0)
    cbdata.diff = game_difficulty_hell;
  else
    cbdata.diff = game_difficulty_none;

  if (dest[0]=='\0')
    {
      cbdata.tag = conn_get_clienttag(c);
      message_send_text(c,message_type_info,c,"Currently accessable games:");
    }
  else if (strcasecmp(&dest[0],"all")==0)
    {
      cbdata.tag = 0;
      message_send_text(c,message_type_info,c,"All current games:");
    }
  else
    {
      cbdata.tag = tag_case_str_to_uint(&dest[0]);

      if (!tag_check_client(cbdata.tag))
      {
	message_send_text(c,message_type_error,c,"No valid clienttag specified.");
	return -1;
      }

      if(cbdata.diff==game_difficulty_none)
        snprintf(msgtemp, sizeof(msgtemp), "Current games of type %.64s",tag_uint_to_str(clienttag_str,cbdata.tag));
      else
        snprintf(msgtemp, sizeof(msgtemp), "Current games of type %.64s %.128s",tag_uint_to_str(clienttag_str,cbdata.tag),&text[i]);
      message_send_text(c,message_type_info,c,msgtemp);
    }

  snprintf(msgtemp, sizeof(msgtemp), " ------name------ p -status- --------type--------- count ");
  if (!cbdata.tag)
    std::strcat(msgtemp,"ctag ");
  if ((!prefs_get_hide_addr()) || (account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-addr"))) /* default to false */
    std::strcat(msgtemp,"--------addr--------");
  message_send_text(c,message_type_info,c,msgtemp);
  gamelist_traverse(_glist_cb,&cbdata);

  return 0;
}

static int _handle_channels_command(t_connection * c, char const *text)
{
  unsigned int      i;
  t_elem const *    curr;
  t_channel const * channel;
  char const *      tag;
  t_connection const * conn;
  t_account * acc;
  char const * name;
  int first;


  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
    {
      tag = clienttag_uint_to_str(conn_get_clienttag(c));
      message_send_text(c,message_type_info,c,"Currently accessable channels:");
    }
  else if (std::strcmp(&text[i],"all")==0)
    {
      tag = NULL;
      message_send_text(c,message_type_info,c,"All current channels:");
    }
  else
    {
      tag = &text[i];
      snprintf(msgtemp, sizeof(msgtemp), "Current channels of type %.64s",tag);
      message_send_text(c,message_type_info,c,msgtemp);
    }

  snprintf(msgtemp, sizeof(msgtemp), " -----------name----------- users ----admin/operator----");
  message_send_text(c,message_type_info,c,msgtemp);
  LIST_TRAVERSE_CONST(channellist(),curr)
    {
      channel = (t_channel*)elem_get_data(curr);
      if ((!(channel_get_flags(channel) & channel_flags_clan)) && (!tag || !prefs_get_hide_temp_channels() || channel_get_permanent(channel)) &&
	  (!tag || !channel_get_clienttag(channel) ||
	   strcasecmp(channel_get_clienttag(channel),tag)==0) &&
	   ((channel_get_max(channel)!=0) || //only show restricted channels to OPs and Admins
	    ((channel_get_max(channel)==0 && account_is_operator_or_admin(conn_get_account(c),NULL)))) &&
	    (!(channel_get_flags(channel) & channel_flags_thevoid)) // don't list TheVoid
	)
	{

	  snprintf(msgtemp, sizeof(msgtemp), " %-26.26s %5u - ",
		  channel_get_name(channel),
		  channel_get_length(channel));

	  first = 1;

	  for (conn = channel_get_first(channel);conn;conn=channel_get_next())
	  {
		acc = conn_get_account(conn);
		if (account_is_operator_or_admin(acc,channel_get_name(channel)) ||
		    channel_conn_is_tmpOP(channel,account_get_conn(acc)))
		{
		  name = conn_get_loggeduser(conn);
		  if (std::strlen(msgtemp) + std::strlen(name) +6 >= MAX_MESSAGE_LEN) break;
		  if (!first) std::strcat(msgtemp," ,");
		  std::strcat(msgtemp,name);
		  if (account_get_auth_admin(acc,NULL)==1) std::strcat(msgtemp,"(A)");
		  else if (account_get_auth_operator(acc,NULL)==1) std::strcat(msgtemp,"(O)");
		  else if (account_get_auth_admin(acc,channel_get_name(channel))==1) std::strcat(msgtemp,"(a)");
		  else if (account_get_auth_operator(acc,channel_get_name(channel))==1) std::strcat(msgtemp,"(o)");
		  first = 0;
		}
	  }

	  message_send_text(c,message_type_info,c,msgtemp);
	}
    }

  return 0;
}

static int _handle_addacct_command(t_connection * c, char const *text)
{
    unsigned int i,j;
    t_account  * temp;
    t_hash       passhash;
    char         username[MAX_USERNAME_LEN];
    char         pass[256];

    for (i=0; text[i]!=' ' && text[i]!='\0'; i++);
    for (; text[i]==' '; i++);

    for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get username */
	if (j<sizeof(username)-1) username[j++] = text[i];
    username[j] = '\0';

    for (; text[i]==' '; i++); /* skip spaces */
    for (j=0; text[i]!='\0'; i++) /* get pass (spaces are allowed) */
	if (j<sizeof(pass)-1) pass[j++] = text[i];
    pass[j] = '\0';

    if (username[0]=='\0' || pass[0]=='\0') {
	message_send_text(c,message_type_info,c,"usage: /addacct <username> <password>");
        return 0;
    }

    if (account_check_name(username)<0) {
        message_send_text(c,message_type_error,c,"Account name contains some invalid symbol!");
        return 0;
    }

    /* FIXME: truncate or err on too long password */
    for (i=0; i<std::strlen(pass); i++)
	if (std::isupper((int)pass[i])) pass[i] = std::tolower((int)pass[i]);

    bnet_hash(&passhash,std::strlen(pass),pass);

    snprintf(msgtemp, sizeof(msgtemp), "Trying to add account \"%.64s\" with password \"%.128s\"",username,pass);
    message_send_text(c,message_type_info,c,msgtemp);

    snprintf(msgtemp, sizeof(msgtemp), "Hash is: %.128s",hash_get_str(passhash));
    message_send_text(c,message_type_info,c,msgtemp);

    temp = accountlist_create_account(username,hash_get_str(passhash));
    if (!temp) {
	message_send_text(c,message_type_error,c,"Failed to create account!");
        eventlog(eventlog_level_debug,__FUNCTION__,"[%d] account \"%s\" not created (failed)",conn_get_socket(c),username);
	return 0;
    }

    snprintf(msgtemp, sizeof(msgtemp), "Account "UID_FORMAT" created.",account_get_uid(temp));
    message_send_text(c,message_type_info,c,msgtemp);
    eventlog(eventlog_level_debug,__FUNCTION__,"[%d] account \"%s\" created",conn_get_socket(c),username);

    return 0;
}

static int _handle_chpass_command(t_connection * c, char const *text)
{
  unsigned int i,j;
  t_account  * account;
  t_account  * temp;
  t_hash       passhash;
  char         arg1[256];
  char         arg2[256];
  char const * username;
  char *       pass;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++);
  for (; text[i]==' '; i++);

  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get username/pass */
    if (j<sizeof(arg1)-1) arg1[j++] = text[i];
  arg1[j] = '\0';

  for (; text[i]==' '; i++); /* skip spaces */
  for (j=0; text[i]!='\0'; i++) /* get pass (spaces are allowed) */
    if (j<sizeof(arg2)-1) arg2[j++] = text[i];
  arg2[j] = '\0';

  if (arg2[0]=='\0')
    {
      username = conn_get_username(c);
      pass     = arg1;
    }
  else
    {
      username = arg1;
      pass     = arg2;
    }

  if (pass[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /chpass [username] <password>");
      return 0;
    }

  temp = accountlist_find_account(username);

  account = conn_get_account(c);

  if ((temp==account && account_get_auth_changepass(account)==0) || /* default to true */
      (temp!=account && !(account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-chpass")))) /* default to false */
    {
      eventlog(eventlog_level_info,__FUNCTION__,"[%d] password change for \"%s\" refused (no change access)",conn_get_socket(c),username);
      message_send_text(c,message_type_error,c,"Only admins may change passwords for other accounts.");
      return 0;
    }

  if (!temp)
    {
      message_send_text(c,message_type_error,c,"Account does not exist.");
      return 0;
    }

  if (std::strlen(pass) > MAX_USERPASS_LEN)
  {
    snprintf(msgtemp, sizeof(msgtemp), "Maximum password length allowed is %d",MAX_USERPASS_LEN);
    message_send_text(c,message_type_error,c,msgtemp);
    return 0;
  }

  for (i=0; i<std::strlen(pass); i++)
    if (std::isupper((int)pass[i])) pass[i] = std::tolower((int)pass[i]);

  bnet_hash(&passhash,std::strlen(pass),pass);

  snprintf(msgtemp, sizeof(msgtemp), "Trying to change password for account \"%.64s\" to \"%.128s\"",username,pass);
  message_send_text(c,message_type_info,c,msgtemp);

  if (account_set_pass(temp,hash_get_str(passhash))<0)
    {
      message_send_text(c,message_type_error,c,"Unable to set password.");
      return 0;
    }

  if (account_get_auth_admin(account,NULL) == 1 ||
      account_get_auth_operator(account,NULL) == 1) {
    snprintf(msgtemp, sizeof(msgtemp), 
      "Password for account "UID_FORMAT" updated.",account_get_uid(temp));
    message_send_text(c,message_type_info,c,msgtemp);

    snprintf(msgtemp, sizeof(msgtemp), "Hash is: %.128s",hash_get_str(passhash));
    message_send_text(c,message_type_info,c,msgtemp);
  } else {
    snprintf(msgtemp, sizeof(msgtemp), 
      "Password for account %.64s updated.",username);
    message_send_text(c,message_type_info,c,msgtemp);
  }

  return 0;
}

static int _handle_connections_command(t_connection *c, char const *text)
{
  t_elem const * curr;
  t_connection * conn;
  char           name[19];
  unsigned int   i; /* for loop */
  char const *   channel_name;
  char const *   game_name;
  char           clienttag_str[5];

  if (!prefs_get_enable_conn_all() && !(account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-con"))) /* default to false */
    {
      message_send_text(c,message_type_error,c,"This command is only enabled for admins.");
      return 0;
    }

  message_send_text(c,message_type_info,c,"Current connections:");
  /* addon */
  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
    {
      snprintf(msgtemp, sizeof(msgtemp), " -class -tag -----name------ -lat(ms)- ----channel---- --game--");
      message_send_text(c,message_type_info,c,msgtemp);
    }
  else
    if (std::strcmp(&text[i],"all")==0) /* print extended info */
      {
	if (prefs_get_hide_addr() && !(account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-addr")))
	  snprintf(msgtemp, sizeof(msgtemp), " -#- -class ----state--- -tag -----name------ -session-- -flag- -lat(ms)- ----channel---- --game--");
	else
	  snprintf(msgtemp, sizeof(msgtemp), " -#- -class ----state--- -tag -----name------ -session-- -flag- -lat(ms)- ----channel---- --game-- ---------addr--------");
	message_send_text(c,message_type_info,c,msgtemp);
      }
    else
      {
	message_send_text(c,message_type_error,c,"Unknown option.");
	return 0;
	  }

  LIST_TRAVERSE_CONST(connlist(),curr)
  {
      conn = (t_connection*)elem_get_data(curr);
      if (conn_get_account(conn))
	  std::sprintf(name,"\"%.16s\"",conn_get_username(conn));
      else
	std::strcpy(name,"(none)");

      if (conn_get_channel(conn)!=NULL)
	channel_name = channel_get_name(conn_get_channel(conn));
      else channel_name = "none";
      if (conn_get_game(conn)!=NULL)
	game_name = game_get_name(conn_get_game(conn));
      else game_name = "none";

      if (text[i]=='\0')
	snprintf(msgtemp, sizeof(msgtemp), " %-6.6s %4.4s %-15.15s %9u %-16.16s %-8.8s",
		conn_class_get_str(conn_get_class(conn)),
		tag_uint_to_str(clienttag_str,conn_get_fake_clienttag(conn)),
		name,
		conn_get_latency(conn),
		channel_name,
		game_name);
      else
	if (prefs_get_hide_addr() && !(account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-addr"))) /* default to false */
	  snprintf(msgtemp, sizeof(msgtemp), " %3d %-6.6s %-12.12s %4.4s %-15.15s 0x%08x 0x%04x %9u %-16.16s %-8.8s",
		  conn_get_socket(conn),
		  conn_class_get_str(conn_get_class(conn)),
		  conn_state_get_str(conn_get_state(conn)),
		  tag_uint_to_str(clienttag_str,conn_get_fake_clienttag(conn)),
		  name,
		  conn_get_sessionkey(conn),
		  conn_get_flags(conn),
		  conn_get_latency(conn),
		  channel_name,
		  game_name);
	else
	  snprintf(msgtemp, sizeof(msgtemp), " %3u %-6.6s %-12.12s %4.4s %-15.15s 0x%08x 0x%04x %9u %-16.16s %-8.8s %.16s",
		  conn_get_socket(conn),
		  conn_class_get_str(conn_get_class(conn)),
		  conn_state_get_str(conn_get_state(conn)),
		  tag_uint_to_str(clienttag_str,conn_get_fake_clienttag(conn)),
		  name,
		  conn_get_sessionkey(conn),
		  conn_get_flags(conn),
		  conn_get_latency(conn),
		  channel_name,
		  game_name,
		  addr_num_to_addr_str(conn_get_addr(conn),conn_get_port(conn)));

      message_send_text(c,message_type_info,c,msgtemp);
    }

  return 0;
}

static int _handle_finger_command(t_connection * c, char const *text)
{
  char           dest[MAX_USERNAME_LEN];
  unsigned int   i,j;
  t_account *    account;
  t_connection * conn;
  char const *   ip;
  char *         tok;
  t_clanmember * clanmemb;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  if (dest[0]=='\0')
  {
    message_send_text(c,message_type_info,c,"usage: /finger <account>");
    return 0;
  }

  if (!(account = accountlist_find_account(dest)))
    {
      message_send_text(c,message_type_error,c,"Invalid user.");
      return 0;
    }
  snprintf(msgtemp, sizeof(msgtemp), "Login: %-16.16s "UID_FORMAT" Sex: %.14s",
	  account_get_name(account),
	  account_get_uid(account),
	  account_get_sex(account));
  message_send_text(c,message_type_info,c,msgtemp);

  if ((clanmemb = account_get_clanmember(account)))
  {
    t_clan *	 clan;
    char	 status;

    if ((clan = clanmember_get_clan(clanmemb)))
    {
	snprintf(msgtemp, sizeof(msgtemp), "Clan : %-64.64s",clan_get_name(clan));
	if ((status = clanmember_get_status(clanmemb)))
	{
	    switch (status)
	    {
		case CLAN_CHIEFTAIN:
		   std::strcat(msgtemp,"  Rank: Chieftain");
		   break;
		case CLAN_SHAMAN:
		   std::strcat(msgtemp,"  Rank: Shaman");
		   break;
		case CLAN_GRUNT:
		   std::strcat(msgtemp,"  Rank: Grunt");
		   break;
		case CLAN_PEON:
		   std::strcat(msgtemp,"  Rank: Peon");
		   break;
		default:;
	    }
	}
	message_send_text(c,message_type_info,c,msgtemp);

    }
  }

  snprintf(msgtemp, sizeof(msgtemp), "Location: %-23.23s Age: %.14s",
	  account_get_loc(account),
	  account_get_age(account));
  message_send_text(c,message_type_info,c,msgtemp);

  if((conn = connlist_find_connection_by_accountname(dest)))
  {
	  snprintf(msgtemp, sizeof(msgtemp), "Client: %.64s    Ver: %.32s   Country: %.128s",
		  clienttag_get_title(conn_get_clienttag(conn)),
		  conn_get_clientver(conn),
		  conn_get_country(conn));
	  message_send_text(c,message_type_info,c,msgtemp);
  }

  if (!(ip=account_get_ll_ip(account)) ||
      !(account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-addr"))) /* default to false */
    ip = "unknown";

  {
    std::time_t      then;
    struct std::tm * tmthen;

    then = account_get_ll_time(account);
    tmthen = std::localtime(&then); /* FIXME: determine user's timezone */
    if (!(conn))
      if (tmthen)
	std::strftime(msgtemp,sizeof(msgtemp),"Last login %a %b %d %H:%M %Y from ",tmthen);
      else
	std::strcpy(msgtemp,"Last login ? from ");
    else
      if (tmthen)
	std::strftime(msgtemp,sizeof(msgtemp),"On since %a %b %d %H:%M %Y from ",tmthen);
      else
	std::strcpy(msgtemp,"On since ? from ");
  }
  std::strncat(msgtemp,ip,32);
  message_send_text(c,message_type_info,c,msgtemp);

  /* check /admin-addr for admin privileges */
  if ( (account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-addr")))
  {
      /* the player who requested /finger has admin privileges
         give him more info about the one he querys;
         is_admin, is_operator, is_locked, email */
         snprintf(msgtemp, sizeof(msgtemp), "email:%.128s , is_operator: %d , is_admin: %d , is_acc_locked: %d",
         account_get_email(account),
         account_get_auth_operator(account,NULL),
         account_get_auth_admin(account,NULL),
         account_get_auth_lock(account));
         message_send_text(c,message_type_info,c,msgtemp);
  }


  if (conn)
    {
      snprintf(msgtemp, sizeof(msgtemp), "Idle %.128s",seconds_to_timestr(conn_get_idletime(conn)));
      message_send_text(c,message_type_info,c,msgtemp);
    }

  std::strncpy(msgtemp,account_get_desc(account),sizeof(msgtemp));
  msgtemp[sizeof(msgtemp)-1] = '\0';
  for (tok=std::strtok(msgtemp,"\r\n"); tok; tok=std::strtok(NULL,"\r\n"))
    message_send_text(c,message_type_info,c,tok);
  message_send_text(c,message_type_info,c,"");

  return 0;
}

/*
 * rewrote command /operator to add and remove operator status [Omega]
 *
 * Fixme: rewrite /operators to show Currently logged on Server and/or Channel operators ...??
 */
/*
static int _handle_operator_command(t_connection * c, char const *text)
{
  t_connection const * opr;
  t_channel const *    channel;

  if (!(channel = conn_get_channel(c)))
    {
      message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
      return 0;
    }

  if (!(opr = channel_get_operator(channel)))
    std::strcpy(msgtemp,"There is no operator.");
  else
      snprintf(msgtemp, sizeof(msgtemp), "%.64s is the operator.",conn_get_username(opr));
  message_send_text(c,message_type_info,c,msgtemp);
  return 0;
}
*/

/* FIXME: do we want to show just Server Admin or Channel Admin Also? [Omega] */
static int _handle_admins_command(t_connection * c, char const *text)
{
  unsigned int    i;
  t_elem const *  curr;
  t_connection *  tc;
  char const *    nick;

  std::strcpy(msgtemp,"Currently logged on Administrators:");
  i = std::strlen(msgtemp);
  LIST_TRAVERSE_CONST(connlist(),curr)
    {
      tc = (t_connection*)elem_get_data(curr);
      if (!tc)
	continue;
      if (!conn_get_account(tc))
        continue;
      if (account_get_auth_admin(conn_get_account(tc),NULL)==1)
	{
	  if ((nick = conn_get_username(tc)))
	    {
	      if (i+std::strlen(nick)+2>sizeof(msgtemp)) /* " ", name, '\0' */
		{
		  message_send_text(c,message_type_info,c,msgtemp);
		  i = 0;
		}
	      std::sprintf(&msgtemp[i]," %s", nick);
	      i += std::strlen(&msgtemp[i]);
	    }
	}
    }
  if (i>0)
    message_send_text(c,message_type_info,c,msgtemp);

  return 0;
}

static int _handle_quit_command(t_connection * c, char const *text)
{
    if (conn_get_game(c))
	eventlog(eventlog_level_warn, __FUNCTION__,"[%d] user '%s' tried to disconnect while in game, cheat attempt ?", conn_get_socket(c), conn_get_loggeduser(c));
    else {
	message_send_text(c,message_type_info,c,"Connection closed.");
	conn_set_state(c,conn_state_destroy);
    }

    return 0;
}

static int _handle_kill_command(t_connection * c, char const *text)
{
  unsigned int	i,j;
  t_connection *	user;
  char		usrnick[MAX_USERNAME_LEN]; /* max length of nick + \0 */  /* FIXME: Is it somewhere defined? */

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get nick */
    if (j<sizeof(usrnick)-1) usrnick[j++] = text[i];
  usrnick[j]='\0';
  for (; text[i]==' '; i++);

  if (usrnick[0]=='\0' || (usrnick[0]=='#' && (usrnick[1] < '0' || usrnick[1] > '9')))
    {
      message_send_text(c,message_type_info,c,"usage: /kill {<username>|#<socket>} [<min>]");
      return 0;
    }

  if (usrnick[0] == '#') {
     if (!(user = connlist_find_connection_by_socket(std::atoi(usrnick + 1)))) {
        message_send_text(c,message_type_error,c,"That connection doesnt exist.");
        return 0;
     }
  } else {
     if (!(user = connlist_find_connection_by_accountname(usrnick))) {
        message_send_text(c,message_type_error,c,"That user is not logged in?");
        return 0;
     }
  }

  if (text[i]!='\0' && ipbanlist_add(c,addr_num_to_ip_str(conn_get_addr(user)),ipbanlist_str_to_time_t(c,&text[i]))==0)
    {
      ipbanlist_save(prefs_get_ipbanfile());
      message_send_text(user,message_type_info,user,"Connection closed by admin and banned your ip.");
    }
  else
    message_send_text(user,message_type_info,user,"Connection closed by admin.");
  conn_set_state(user,conn_state_destroy);

  message_send_text(c,message_type_info,c,"Operation successful.");

  return 0;
}

static int _handle_killsession_command(t_connection * c, char const *text)
{
  unsigned int	i,j;
  t_connection *	user;
  char		session[16];

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get nick */
    if (j<sizeof(session)-1) session[j++] = text[i];
  session[j]='\0';
  for (; text[i]==' '; i++);

  if (session[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /killsession <session> [min]");
      return 0;
    }
  if (!std::isxdigit((int)session[0]))
    {
      message_send_text(c,message_type_error,c,"That is not a valid session.");
      return 0;
    }
  if (!(user = connlist_find_connection_by_sessionkey((unsigned int)std::strtoul(session,NULL,16))))
    {
      message_send_text(c,message_type_error,c,"That session does not exist.");
      return 0;
    }
  if (text[i]!='\0' && ipbanlist_add(c,addr_num_to_ip_str(conn_get_addr(user)),ipbanlist_str_to_time_t(c,&text[i]))==0)
    {
      ipbanlist_save(prefs_get_ipbanfile());
      message_send_text(user,message_type_info,user,"Connection closed by admin and banned your ip's.");
    }
  else
    message_send_text(user,message_type_info,user,"Connection closed by admin.");
  conn_set_state(user,conn_state_destroy);
  return 0;
}

static int _handle_gameinfo_command(t_connection * c, char const *text)
{
  unsigned int   i;
  t_game const * game;
  char clienttag_str[5];

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
    {
      if (!(game = conn_get_game(c)))
	{
	  message_send_text(c,message_type_error,c,"You are not in a game.");
	  return 0;
	}
    }
  else
    if (!(game = gamelist_find_game(&text[i],conn_get_clienttag(c),game_type_all)))
      {
	message_send_text(c,message_type_error,c,"That game does not exist.");
	return 0;
      }

  snprintf(msgtemp, sizeof(msgtemp), "Name: %-20.20s    ID: "GAMEID_FORMAT" (%.20s)",game_get_name(game),game_get_id(game),game_get_flag(game) != game_flag_private ? "public":"private");
  message_send_text(c,message_type_info,c,msgtemp);

  {
    t_account *  owner;
    char const * tname;
    char const * namestr;

    if (!(owner = conn_get_account(game_get_owner(game))))
      {
	tname = NULL;
	namestr = "none";
      }
    else
      if (!(tname = conn_get_loggeduser(game_get_owner(game))))
	namestr = "unknown";
      else
	namestr = tname;

    snprintf(msgtemp, sizeof(msgtemp), "Owner: %-20.20s",namestr);

  }
  message_send_text(c,message_type_info,c,msgtemp);

  if (!prefs_get_hide_addr() || (account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-addr"))) /* default to false */
    {
      unsigned int   addr;
      unsigned short port;
      unsigned int   taddr;
      unsigned short tport;

      taddr=addr = game_get_addr(game);
      tport=port = game_get_port(game);
      trans_net(conn_get_addr(c),&taddr,&tport);

      if (taddr==addr && tport==port)
	snprintf(msgtemp, sizeof(msgtemp), "Address: %.64s",
		addr_num_to_addr_str(addr,port));
      else
	snprintf(msgtemp, sizeof(msgtemp), "Address: %.64s (trans %.64s)",
		addr_num_to_addr_str(addr,port),
		addr_num_to_addr_str(taddr,tport));
      message_send_text(c,message_type_info,c,msgtemp);
    }

  snprintf(msgtemp, sizeof(msgtemp), "Client: %4s (version %.64s, startver %u)",tag_uint_to_str(clienttag_str,game_get_clienttag(game)),vernum_to_verstr(game_get_version(game)),game_get_startver(game));
  message_send_text(c,message_type_info,c,msgtemp);

  {
    std::time_t      gametime;
    struct std::tm * gmgametime;

    gametime = game_get_create_time(game);
    if (!(gmgametime = std::localtime(&gametime)))
      std::strcpy(msgtemp,"Created: ?");
    else
      std::strftime(msgtemp,sizeof(msgtemp),"Created: "GAME_TIME_FORMAT,gmgametime);
    message_send_text(c,message_type_info,c,msgtemp);

    gametime = game_get_start_time(game);
    if (gametime!=(std::time_t)0)
      {
	if (!(gmgametime = std::localtime(&gametime)))
	  std::strcpy(msgtemp,"Started: ?");
	else
	  std::strftime(msgtemp,sizeof(msgtemp),"Started: "GAME_TIME_FORMAT,gmgametime);
      }
    else
      std::strcpy(msgtemp,"Started: ");
    message_send_text(c,message_type_info,c,msgtemp);
  }

  snprintf(msgtemp, sizeof(msgtemp), "Status: %.128s",game_status_get_str(game_get_status(game)));
  message_send_text(c,message_type_info,c,msgtemp);

  snprintf(msgtemp, sizeof(msgtemp), "Type: %-20.20s",game_type_get_str(game_get_type(game)));
  message_send_text(c,message_type_info,c,msgtemp);

  snprintf(msgtemp, sizeof(msgtemp), "Speed: %.128s",game_speed_get_str(game_get_speed(game)));
  message_send_text(c,message_type_info,c,msgtemp);

  snprintf(msgtemp, sizeof(msgtemp), "Difficulty: %.128s",game_difficulty_get_str(game_get_difficulty(game)));
  message_send_text(c,message_type_info,c,msgtemp);

  snprintf(msgtemp, sizeof(msgtemp), "Option: %.128s",game_option_get_str(game_get_option(game)));
  message_send_text(c,message_type_info,c,msgtemp);

  {
    char const * mapname;

    if (!(mapname = game_get_mapname(game)))
      mapname = "unknown";
    snprintf(msgtemp, sizeof(msgtemp), "Map: %-20.20s",mapname);
    message_send_text(c,message_type_info,c,msgtemp);
  }

  snprintf(msgtemp, sizeof(msgtemp), "Map Size: %ux%u",game_get_mapsize_x(game),game_get_mapsize_y(game));
  message_send_text(c,message_type_info,c,msgtemp);
  snprintf(msgtemp, sizeof(msgtemp), "Map Tileset: %.128s",game_tileset_get_str(game_get_tileset(game)));
  message_send_text(c,message_type_info,c,msgtemp);
  snprintf(msgtemp, sizeof(msgtemp), "Map Type: %.128s",game_maptype_get_str(game_get_maptype(game)));
  message_send_text(c,message_type_info,c,msgtemp);

  snprintf(msgtemp, sizeof(msgtemp), "Players: %u current, %u total, %u max",game_get_ref(game),game_get_count(game),game_get_maxplayers(game));
  message_send_text(c,message_type_info,c,msgtemp);

  {
    char const * description;

    if (!(description = game_get_description(game)))
      description = "";
    snprintf(msgtemp, sizeof(msgtemp), "Description: %-20.20s",description);
  }

  return 0;
}

static int _handle_ladderactivate_command(t_connection * c, char const *text)
{
  ladders.activate();
  message_send_text(c,message_type_info,c,"Copied current scores to active scores on all ladders.");
  return 0;
}

static int _handle_rehash_command(t_connection * c, char const *text)
{
  server_restart_wraper();
  return 0;
}

/*
static int _handle_rank_all_accounts_command(t_connection * c, char const *text)
{
  // rank all accounts here
  accounts_rank_all();
  return 0;
}
*/

static int _handle_shutdown_command(t_connection * c, char const *text)
{
  char         dest[32];
  unsigned int i,j;
  unsigned int delay;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  if (dest[0]=='\0')
    delay = prefs_get_shutdown_delay();
  else
    if (clockstr_to_seconds(dest,&delay)<0)
      {
	message_send_text(c,message_type_error,c,"Invalid delay.");
	return 0;
      }

  server_quit_delay(delay);

  if (delay)
    message_send_text(c,message_type_info,c,"You initialized the shutdown sequence.");
  else
    message_send_text(c,message_type_info,c,"You canceled the shutdown sequence.");

  return 0;
}

static int _handle_ladderinfo_command(t_connection * c, char const *text)
{
  char         dest[32];
  unsigned int rank;
  unsigned int i,j;
  t_account *  account;
  t_team * team;
  t_clienttag clienttag;
  const LadderReferencedObject* referencedObject;
  LadderList* ladderList;

  text = skip_command(text);
  for (i=0,j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  if (dest[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /ladderinfo <rank> [clienttag]");
      return 0;
    }
  if (str_to_uint(dest,&rank)<0 || rank<1)
    {
      message_send_text(c,message_type_error,c,"Invalid rank.");
      return 0;
    }

  if (text[i]!='\0') {
    if (std::strlen(&text[i])!=4) {
      message_send_text(c,message_type_error,c,text);
      message_send_text(c,message_type_error,c,"You must supply a rank and a valid program ID.");
      message_send_text(c,message_type_error,c,"Example: /ladderinfo 1 STAR not");
      return 0;
    }
    clienttag = tag_case_str_to_uint(&text[i]);
  } else if (!(clienttag = conn_get_clienttag(c)))
    {
      message_send_text(c,message_type_error,c,"Unable to determine client game.");
      return 0;
    }
  if (clienttag==CLIENTTAG_STARCRAFT_UINT)
    {
      ladderList = ladders.getLadderList(LadderKey(ladder_id_normal, clienttag, ladder_sort_highestrated, ladder_time_active));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "Starcraft active  %5u: %-20.20s %u/%u/%u rating %u",
		  rank,
		  account_get_name(account),
		  account_get_ladder_active_wins(account,CLIENTTAG_STARCRAFT_UINT,ladder_id_normal),
		  account_get_ladder_active_losses(account,CLIENTTAG_STARCRAFT_UINT,ladder_id_normal),
		  account_get_ladder_active_disconnects(account,CLIENTTAG_STARCRAFT_UINT,ladder_id_normal),
		  account_get_ladder_active_rating(account,CLIENTTAG_STARCRAFT_UINT,ladder_id_normal));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "Starcraft active  %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);

      ladderList = ladders.getLadderList(LadderKey(ladder_id_normal, clienttag, ladder_sort_highestrated, ladder_time_current));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "Starcraft current %5u: %-20.20s %u/%u/%u rating %u",
		  rank,
		  account_get_name(account),
		  account_get_ladder_wins(account,CLIENTTAG_STARCRAFT_UINT,ladder_id_normal),
		  account_get_ladder_losses(account,CLIENTTAG_STARCRAFT_UINT,ladder_id_normal),
		  account_get_ladder_disconnects(account,CLIENTTAG_STARCRAFT_UINT,ladder_id_normal),
		  account_get_ladder_rating(account,CLIENTTAG_STARCRAFT_UINT,ladder_id_normal));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "Starcraft current %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);
    }
  else if (clienttag==CLIENTTAG_BROODWARS_UINT)
    {
      ladderList = ladders.getLadderList(LadderKey(ladder_id_normal, clienttag, ladder_sort_highestrated, ladder_time_active));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "Brood War active  %5u: %-20.20s %u/%u/%u rating %u",
		  rank,
		  account_get_name(account),
		  account_get_ladder_active_wins(account,CLIENTTAG_BROODWARS_UINT,ladder_id_normal),
		  account_get_ladder_active_losses(account,CLIENTTAG_BROODWARS_UINT,ladder_id_normal),
		  account_get_ladder_active_disconnects(account,CLIENTTAG_BROODWARS_UINT,ladder_id_normal),
		  account_get_ladder_active_rating(account,CLIENTTAG_BROODWARS_UINT,ladder_id_normal));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "Brood War active  %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);

      ladderList = ladders.getLadderList(LadderKey(ladder_id_normal, clienttag, ladder_sort_highestrated, ladder_time_current));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "Brood War current %5u: %-20.20s %u/%u/%u rating %u",
		  rank,
		  account_get_name(account),
		  account_get_ladder_wins(account,CLIENTTAG_BROODWARS_UINT,ladder_id_normal),
		  account_get_ladder_losses(account,CLIENTTAG_BROODWARS_UINT,ladder_id_normal),
		  account_get_ladder_disconnects(account,CLIENTTAG_BROODWARS_UINT,ladder_id_normal),
		  account_get_ladder_rating(account,CLIENTTAG_BROODWARS_UINT,ladder_id_normal));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "Brood War current %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);
    }
  else if (clienttag==CLIENTTAG_WARCIIBNE_UINT)
    {
      ladderList = ladders.getLadderList(LadderKey(ladder_id_normal, clienttag, ladder_sort_highestrated, ladder_time_active));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "Warcraft II standard active  %5u: %-20.20s %u/%u/%u rating %u",
		  rank,
		  account_get_name(account),
		  account_get_ladder_active_wins(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_normal),
		  account_get_ladder_active_losses(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_normal),
		  account_get_ladder_active_disconnects(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_normal),
		  account_get_ladder_active_rating(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_normal));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "Warcraft II standard active  %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);

      ladderList = ladders.getLadderList(LadderKey(ladder_id_ironman, clienttag, ladder_sort_highestrated, ladder_time_active));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "Warcraft II IronMan active   %5u: %-20.20s %u/%u/%u rating %u",
		  rank,
		  account_get_name(account),
		  account_get_ladder_active_wins(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_ironman),
		  account_get_ladder_active_losses(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_ironman),
		  account_get_ladder_active_disconnects(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_ironman),
		  account_get_ladder_active_rating(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_ironman));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "Warcraft II IronMan active   %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);

      ladderList = ladders.getLadderList(LadderKey(ladder_id_normal, clienttag, ladder_sort_highestrated, ladder_time_current));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "Warcraft II standard current %5u: %-20.20s %u/%u/%u rating %u",
		  rank,
		  account_get_name(account),
		  account_get_ladder_wins(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_normal),
		  account_get_ladder_losses(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_normal),
		  account_get_ladder_disconnects(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_normal),
		  account_get_ladder_rating(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_normal));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "Warcraft II standard current %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);

      ladderList = ladders.getLadderList(LadderKey(ladder_id_ironman, clienttag, ladder_sort_highestrated, ladder_time_current));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "Warcraft II IronMan current  %5u: %-20.20s %u/%u/%u rating %u",
		  rank,
		  account_get_name(account),
		  account_get_ladder_wins(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_ironman),
		  account_get_ladder_losses(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_ironman),
		  account_get_ladder_disconnects(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_ironman),
		  account_get_ladder_rating(account,CLIENTTAG_WARCIIBNE_UINT,ladder_id_ironman));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "Warcraft II IronMan current  %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);
    }
  // --> aaron
  else if (clienttag==CLIENTTAG_WARCRAFT3_UINT || clienttag==CLIENTTAG_WAR3XP_UINT)
    {
      ladderList = ladders.getLadderList(LadderKey(ladder_id_solo, clienttag, ladder_sort_default, ladder_time_default));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "WarCraft3 Solo   %5u: %-20.20s %u/%u/0",
		  rank,
		  account_get_name(account),
		  account_get_ladder_wins(account,clienttag,ladder_id_solo),
		  account_get_ladder_losses(account,clienttag,ladder_id_solo));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "WarCraft3 Solo   %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);

      ladderList = ladders.getLadderList(LadderKey(ladder_id_team, clienttag, ladder_sort_default, ladder_time_default));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "WarCraft3 Team   %5u: %-20.20s %u/%u/0",
		  rank,
		  account_get_name(account),
		  account_get_ladder_wins(account,clienttag,ladder_id_team),
		  account_get_ladder_losses(account,clienttag,ladder_id_team));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "WarCraft3 Team   %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);

      ladderList = ladders.getLadderList(LadderKey(ladder_id_ffa, clienttag, ladder_sort_default, ladder_time_default));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (account = referencedObject->getAccount()))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "WarCraft3 FFA   %5u: %-20.20s %u/%u/0",
		  rank,
		  account_get_name(account),
		  account_get_ladder_wins(account,clienttag,ladder_id_ffa),
		  account_get_ladder_losses(account,clienttag,ladder_id_ffa));
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "WarCraft3 FFA   %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);

      ladderList = ladders.getLadderList(LadderKey(ladder_id_ateam, clienttag, ladder_sort_default, ladder_time_default));
      referencedObject = ladderList->getReferencedObject(rank);
      if ((referencedObject) && (team = referencedObject->getTeam()))
      {
	    t_xstr * membernames = xstr_alloc();
	    for (unsigned char i=0; i<team_get_size(team);i++){
		    xstr_cat_str(membernames, account_get_name(team_get_member(team,i)));
		    if ((i)) xstr_cat_char(membernames,',');
	    }
	    snprintf(msgtemp, sizeof(msgtemp), "WarCraft3 AT Team   %5u: %-80.80s %u/%u/0",
		    rank,
		    xstr_get_str(membernames),
		    team_get_wins(team),
		    team_get_losses(team));
	    xstr_free(membernames);
	}
      else
	snprintf(msgtemp, sizeof(msgtemp), "WarCraft3 AT Team  %5u: <none>",rank);
      message_send_text(c,message_type_info,c,msgtemp);
    }
  //<---
  else
    {
      message_send_text(c,message_type_error,c,"This game does not support win/loss records.");
      message_send_text(c,message_type_error,c,"You must supply a rank and a valid program ID.");
      message_send_text(c,message_type_error,c,"Example: /ladderinfo 1 STAR");
    }

  return 0;
}

static int _handle_timer_command(t_connection * c, char const *text)
{
  unsigned int i,j;
  unsigned int delta;
  char         deltastr[64];
  t_timer_data data;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get comm */
    if (j<sizeof(deltastr)-1) deltastr[j++] = text[i];
  deltastr[j] = '\0';
  for (; text[i]==' '; i++);

  if (deltastr[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /timer <duration>");
      return 0;
    }

  if (clockstr_to_seconds(deltastr,&delta)<0)
    {
      message_send_text(c,message_type_error,c,"Invalid duration.");
      return 0;
    }

  if (text[i]=='\0')
    data.p = xstrdup("Your timer has expired.");
  else
    data.p = xstrdup(&text[i]);

  if (timerlist_add_timer(c,std::time(NULL)+(std::time_t)delta,user_timer_cb,data)<0)
    {
      eventlog(eventlog_level_error,__FUNCTION__,"could not add timer");
      xfree(data.p);
      message_send_text(c,message_type_error,c,"Could not set timer.");
    }
  else
    {
      snprintf(msgtemp, sizeof(msgtemp), "Timer set for %.128s",seconds_to_timestr(delta));
      message_send_text(c,message_type_info,c,msgtemp);
    }

  return 0;
}

static int _handle_serverban_command(t_connection *c, char const *text)
{
  char dest[MAX_USERNAME_LEN];
  t_connection * dest_c;
  unsigned int i,j;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); // skip command
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) // get dest
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

      if (dest[0]=='\0')
      {
	message_send_text(c,message_type_info,c,"usage: /serverban <account>");
	return 0;
      }

      if (!(dest_c = connlist_find_connection_by_accountname(dest)))
	{
	  message_send_text(c,message_type_error,c,"That user is not logged on.");
	  return 0;
	}
      snprintf(msgtemp, sizeof(msgtemp), "Banning User %.64s who is using IP %.64s",conn_get_username(dest_c),addr_num_to_ip_str(conn_get_game_addr(dest_c)));
      message_send_text(c,message_type_info,c,msgtemp);
      message_send_text(c,message_type_info,c,"Users Account is also LOCKED! Only a Admin can Unlock it!");
      snprintf(msgtemp, sizeof(msgtemp), "/ipban a %.64s",addr_num_to_ip_str(conn_get_game_addr(dest_c)));
      handle_ipban_command(c,msgtemp);
      account_set_auth_lock(conn_get_account(dest_c),1);
      //now kill the connection
      snprintf(msgtemp, sizeof(msgtemp), "You have been banned by Admin: %.64s",conn_get_username(c));
      message_send_text(dest_c,message_type_error,dest_c,msgtemp);
      message_send_text(dest_c,message_type_error,dest_c,"Your account is also LOCKED! Only a admin can UNLOCK it!");
      conn_set_state(dest_c, conn_state_destroy);
      return 0;
}

static int _handle_netinfo_command(t_connection * c, char const *text)
{
  char           dest[MAX_USERNAME_LEN];
  unsigned int   i,j;
  t_connection * conn;
  t_game const * game;
  unsigned int   addr;
  unsigned short port;
  unsigned int   taddr;
  unsigned short tport;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); // skip command
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) // get dest
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  if (dest[0]=='\0')
      std::strcpy(dest,conn_get_username(c));

  if (!(conn = connlist_find_connection_by_accountname(dest)))
    {
      message_send_text(c,message_type_error,c,"That user is not logged on.");
      return 0;
    }

  if (conn_get_account(conn)!=conn_get_account(c) &&
      prefs_get_hide_addr() && !(account_get_command_groups(conn_get_account(c)) & command_get_group("/admin-addr"))) // default to false
    {
      message_send_text(c,message_type_error,c,"Address information for other users is only available to admins.");
      return 0;
    }

  snprintf(msgtemp, sizeof(msgtemp), "Server TCP: %.64s (bind %.64s)",addr_num_to_addr_str(conn_get_real_local_addr(conn),conn_get_real_local_port(conn)),addr_num_to_addr_str(conn_get_local_addr(conn),conn_get_local_port(conn)));
  message_send_text(c,message_type_info,c,msgtemp);

  snprintf(msgtemp, sizeof(msgtemp), "Client TCP: %.64s",addr_num_to_addr_str(conn_get_addr(conn),conn_get_port(conn)));
  message_send_text(c,message_type_info,c,msgtemp);

  taddr=addr = conn_get_game_addr(conn);
  tport=port = conn_get_game_port(conn);
  trans_net(conn_get_addr(c),&taddr,&tport);

  if (taddr==addr && tport==port)
    snprintf(msgtemp, sizeof(msgtemp), "Client UDP: %.64s",
	    addr_num_to_addr_str(addr,port));
  else
    snprintf(msgtemp, sizeof(msgtemp), "Client UDP: %.64s (trans %.64s)",
	    addr_num_to_addr_str(addr,port),
	    addr_num_to_addr_str(taddr,tport));
  message_send_text(c,message_type_info,c,msgtemp);

  if ((game = conn_get_game(conn)))
    {
      taddr=addr = game_get_addr(game);
      tport=port = game_get_port(game);
      trans_net(conn_get_addr(c),&taddr,&tport);

      if (taddr==addr && tport==port)
	snprintf(msgtemp, sizeof(msgtemp), "Game UDP:  %.64s",
		addr_num_to_addr_str(addr,port));
      else
	snprintf(msgtemp, sizeof(msgtemp), "Game UDP:  %.64s (trans %.64s)",
		addr_num_to_addr_str(addr,port),
		addr_num_to_addr_str(taddr,tport));
    }
  else
    std::strcpy(msgtemp,"Game UDP:  none");
  message_send_text(c,message_type_info,c,msgtemp);

  return 0;
}

static int _handle_quota_command(t_connection * c, char const * text)
{
  snprintf(msgtemp, sizeof(msgtemp), "Your quota allows you to write %u lines per %u seconds.",prefs_get_quota_lines(),prefs_get_quota_time());
  message_send_text(c,message_type_info,c,msgtemp);
  snprintf(msgtemp, sizeof(msgtemp), "Long lines will be considered to wrap every %u characters.",prefs_get_quota_wrapline());
  message_send_text(c,message_type_info,c,msgtemp);
  snprintf(msgtemp, sizeof(msgtemp), "You are not allowed to send lines with more than %u characters.",prefs_get_quota_maxline());
  message_send_text(c,message_type_info,c,msgtemp);

  return 0;
}

static int _handle_lockacct_command(t_connection * c, char const *text)
{
  t_connection * user;
  t_account *    account;

  text = skip_command(text);

  if (text[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /lockacct <username>");
      return 0;
    }

  if (!(account = accountlist_find_account(text)))
    {
      message_send_text(c,message_type_error,c,"Invalid user.");
      return 0;
    }
  if ((user = connlist_find_connection_by_accountname(text)))
    message_send_text(user,message_type_info,user,"Your account has just been locked by admin.");

  account_set_auth_lock(account,1);
  message_send_text(c,message_type_error,c,"That user account is now locked.");
  return 0;
}

static int _handle_unlockacct_command(t_connection * c, char const *text)
{
  t_connection * user;
  t_account *    account;

  text = skip_command(text);

  if (text[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /unlockacct <username>");
      return 0;
    }
  if (!(account = accountlist_find_account(text)))
    {
      message_send_text(c,message_type_error,c,"Invalid user.");
      return 0;
    }

  if ((user = connlist_find_connection_by_accountname(text)))
    message_send_text(user,message_type_info,user,"Your account has just been unlocked by admin.");

  account_set_auth_lock(account,0);
  message_send_text(c,message_type_error,c,"That user account is now unlocked.");
  return 0;
}

static int _handle_flag_command(t_connection * c, char const *text)
{
  char         dest[32];
  unsigned int i,j;
  unsigned int newflag;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);
  for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
  dest[j] = '\0';
  for (; text[i]==' '; i++);

  if (dest[0]=='\0')
    {
      message_send_text(c,message_type_info,c,"usage: /flag <flag>");
      return 0;
    }

  newflag = std::strtoul(dest,NULL,0);
  conn_set_flags(c,newflag);

  snprintf(msgtemp, sizeof(msgtemp), "Flags set to 0x%08x.",newflag);
  message_send_text(c,message_type_info,c,msgtemp);
  return 0;
}

static int _handle_tag_command(t_connection * c, char const *text)
{
    char         dest[8];
    unsigned int i,j;
    unsigned int newtag;

    for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
    for (; text[i]==' '; i++);
    for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
    if (j<sizeof(dest)-1) dest[j++] = text[i];
    dest[j] = '\0';
    for (; text[i]==' '; i++);

    if (dest[0]=='\0')
    {
	message_send_text(c,message_type_info,c,"usage: /tag <clienttag>");
	return 0;
    }
    if (std::strlen(dest)!=4)
    {
	message_send_text(c,message_type_error,c,"Client tag should be four characters long.");
	return 0;
    }
    newtag = tag_case_str_to_uint(dest);
    if (tag_check_client(newtag))
    {
        unsigned int oldflags = conn_get_flags(c);
        conn_set_clienttag(c,newtag);
	if ((newtag==CLIENTTAG_WARCRAFT3_UINT) || (newtag==CLIENTTAG_WAR3XP_UINT))
	  conn_update_w3_playerinfo(c);
	channel_rejoin(c);
	conn_set_flags(c,oldflags);
	channel_update_userflags(c);
        snprintf(msgtemp, sizeof(msgtemp), "Client tag set to %.128s.",dest);
    }
    else
    snprintf(msgtemp, sizeof(msgtemp), "Invalid clienttag %.128s specified",dest);
    message_send_text(c,message_type_info,c,msgtemp);
    return 0;
}

static int _handle_set_command(t_connection * c, char const *text)
{
  t_account * account;
  char *accname;
  char *key;
  char *value;
  char t[MAX_MESSAGE_LEN];
  unsigned int i,j;
  char         arg1[256];
  char         arg2[256];
  char         arg3[256];

  std::strncpy(t, text, MAX_MESSAGE_LEN - 1);
  for (i=0; t[i]!=' ' && t[i]!='\0'; i++); /* skip command /set */

  for (; t[i]==' '; i++); /* skip spaces */
  for (j=0; t[i]!=' ' && t[i]!='\0'; i++) /* get username */
    if (j<sizeof(arg1)-1) arg1[j++] = t[i];
  arg1[j] = '\0';

  for (; t[i]==' '; i++); /* skip spaces */
  for (j=0; t[i]!=' ' && t[i]!='\0'; i++) /* get key */
    if (j<sizeof(arg2)-1) arg2[j++] = t[i];
  arg2[j] = '\0';

  for (; t[i]==' '; i++); /* skip spaces */
  for (j=0; t[i]!='\0'; i++) /* get value */
    if (j<sizeof(arg3)-1) arg3[j++] = t[i];
  arg3[j] = '\0';

  accname = arg1;
  key     = arg2;
  value   = arg3;

  if ((arg1[0] =='\0') || (arg2[0]=='\0'))
  {
	message_send_text(c,message_type_info,c,"usage: /set <username> <key> [value]");
  }

  if (!(account = accountlist_find_account(accname)))
    {
      message_send_text(c,message_type_error,c,"Invalid user.");
      return 0;
    }

  if (*value == '\0')
    {
      if (account_get_strattr(account,key))
	{
	  snprintf(msgtemp, sizeof(msgtemp), "current value of %.64s is \"%.128s\"",key,account_get_strattr(account,key));
	  message_send_text(c,message_type_error,c,msgtemp);
	}
      else
	message_send_text(c,message_type_error,c,"value currently not set");
      return 0;
    }

  if (account_set_strattr(account,key,value)<0)
    message_send_text(c,message_type_error,c,"Unable to set key");
  else
    message_send_text(c,message_type_error,c,"Key set succesfully");

  return 0;
}

static int _handle_motd_command(t_connection * c, char const *text)
{
  char const * filename;
  std::FILE *       fp;

  if ((filename = prefs_get_motdfile())) {
    if ((fp = std::fopen(filename,"r")))
      {
	message_send_file(c,fp);
	if (std::fclose(fp)<0)
	  eventlog(eventlog_level_error,__FUNCTION__,"could not close motd file \"%s\" after reading (std::fopen: %s)",filename,std::strerror(errno));
      }
    else
      {
	eventlog(eventlog_level_error,__FUNCTION__,"could not open motd file \"%s\" for reading (std::fopen: %s)",filename,std::strerror(errno));
	message_send_text(c,message_type_error,c,"Unable to open motd.");
      }
    return 0;
  } else {
    message_send_text(c,message_type_error,c,"No motd.");
    return 0;
  }
}

static int _handle_tos_command(t_connection * c, char const * text)
{
/* handle /tos - shows terms of service by user request -raistlinthewiz */

  char * filename=NULL;
  std::FILE * fp;

  filename = buildpath(prefs_get_filedir(),prefs_get_tosfile());

  /* FIXME: if user enters relative path to tos file in config,
     above routine will fail */

    if ((fp = std::fopen(filename,"r")))
    {

         char * buff;
         unsigned len;

         while ((buff = file_get_line(fp)))
         {

               if ((len=std::strlen(buff)) < MAX_MESSAGE_LEN)
                  message_send_text(c,message_type_info,c,buff);
               else {
               /*  lines in TOS file can be > MAX_MESSAGE_LEN, so split them
               truncating is not an option for TOS -raistlinthewiz
               */

                  while ( len  > MAX_MESSAGE_LEN - 1)
                  {
                        std::strncpy(msgtemp,buff,MAX_MESSAGE_LEN-1);
                        msgtemp[MAX_MESSAGE_LEN]='\0';
                        buff += MAX_MESSAGE_LEN - 1;
                        len -= MAX_MESSAGE_LEN - 1;
                        message_send_text(c,message_type_info,c,msgtemp);
                  }

                  if ( len > 0 ) /* does it exist a small last part ? */
                     message_send_text(c,message_type_info,c,buff);

               }
         }


       	if (std::fclose(fp)<0)
	       eventlog(eventlog_level_error,__FUNCTION__,"could not close tos file \"%s\" after reading (std::fopen: %s)",filename,std::strerror(errno));
    }
    else
    {
	    eventlog(eventlog_level_error,__FUNCTION__,"could not open tos file \"%s\" for reading (std::fopen: %s)",filename,std::strerror(errno));
        message_send_text(c,message_type_error,c,"Unable to send TOS (terms of service).");
    }
    xfree((void *)filename);
    return 0;

}


static int _handle_ping_command(t_connection * c, char const *text)
{
  unsigned int i;
  t_connection *	user;
  t_game 	*	game;

  for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
  for (; text[i]==' '; i++);

  if (text[i]=='\0')
    {
      if ((game=conn_get_game(c)))
	{
	  for (i=0; i<game_get_count(game); i++)
	    {
	      if ((user = game_get_player_conn(game, i)))
		{
		  snprintf(msgtemp, sizeof(msgtemp), "%.64s latency: %9u",conn_get_username(user),conn_get_latency(user));
		  message_send_text(c,message_type_info,c,msgtemp);
		}
	    }
	  return 0;
	}
      snprintf(msgtemp, sizeof(msgtemp), "Your latency %9u",conn_get_latency(c));
    }
  else if ((user = connlist_find_connection_by_accountname(&text[i])))
    snprintf(msgtemp, sizeof(msgtemp), "%.64s latency %9u",&text[i],conn_get_latency(user));
  else
    snprintf(msgtemp, sizeof(msgtemp), "Invalid user");

  message_send_text(c,message_type_info,c,msgtemp);
  return 0;
}

/* Redirected to handle_ipban_command in ipban.c [Omega]
static int _handle_ipban_command(t_connection * c, char const *text)
{
  handle_ipban_command(c,text);
  return 0;
}
*/

static int _handle_commandgroups_command(t_connection * c, char const * text)
{
    t_account *	account;
    char *	command;
    char *	username;
    unsigned int usergroups;	// from user account
    unsigned int groups = 0;	// converted from arg3
    char	tempgroups[9];	// converted from usergroups
    char 	t[MAX_MESSAGE_LEN];
    unsigned int i,j;
    char	arg1[256];
    char	arg2[256];
    char	arg3[256];

    std::strncpy(t, text, MAX_MESSAGE_LEN - 1);
    for (i=0; t[i]!=' ' && t[i]!='\0'; i++); /* skip command /groups */

    for (; t[i]==' '; i++); /* skip spaces */
    for (j=0; t[i]!=' ' && t[i]!='\0'; i++) /* get command */
	if (j<sizeof(arg1)-1) arg1[j++] = t[i];
    arg1[j] = '\0';

    for (; t[i]==' '; i++); /* skip spaces */
    for (j=0; t[i]!=' ' && t[i]!='\0'; i++) /* get username */
	if (j<sizeof(arg2)-1) arg2[j++] = t[i];
    arg2[j] = '\0';

    for (; t[i]==' '; i++); /* skip spaces */
    for (j=0; t[i]!='\0'; i++) /* get groups */
	if (j<sizeof(arg3)-1) arg3[j++] = t[i];
    arg3[j] = '\0';

    command = arg1;
    username = arg2;

    if (arg1[0] =='\0') {
	message_send_text(c,message_type_info,c,"usage: /cg <command> <username> [<group(s)>]");
	return 0;
    }

    if (!std::strcmp(command,"help") || !std::strcmp(command,"h")) {
	message_send_text(c,message_type_info,c,"Command Groups (Defines the Groups of Commands a User Can Use.)");
	message_send_text(c,message_type_info,c,"Type: /cg add <username> <group(s)> - adds group(s) to user profile");
	message_send_text(c,message_type_info,c,"Type: /cg del <username> <group(s)> - deletes group(s) from user profile");
	message_send_text(c,message_type_info,c,"Type: /cg list <username> - shows current groups user can use");
	return 0;
    }

    if (arg2[0] =='\0') {
	message_send_text(c,message_type_info,c,"usage: /cg <command> <username> [<group(s)>]");
	return 0;
    }

    if (!(account = accountlist_find_account(username))) {
	message_send_text(c,message_type_error,c,"Invalid user.");
	return 0;
    }

    usergroups = account_get_command_groups(account);

    if (!std::strcmp(command,"list") || !std::strcmp(command,"l")) {
	if (usergroups & 1) tempgroups[0] = '1'; else tempgroups[0] = ' ';
	if (usergroups & 2) tempgroups[1] = '2'; else tempgroups[1] = ' ';
	if (usergroups & 4) tempgroups[2] = '3'; else tempgroups[2] = ' ';
	if (usergroups & 8) tempgroups[3] = '4'; else tempgroups[3] = ' ';
	if (usergroups & 16) tempgroups[4] = '5'; else tempgroups[4] = ' ';
	if (usergroups & 32) tempgroups[5] = '6'; else tempgroups[5] = ' ';
	if (usergroups & 64) tempgroups[6] = '7'; else tempgroups[6] = ' ';
	if (usergroups & 128) tempgroups[7] = '8'; else tempgroups[7] = ' ';
	tempgroups[8] = '\0';
	snprintf(msgtemp, sizeof(msgtemp), "%.64s's command group(s): %.64s", username, tempgroups);
	message_send_text(c,message_type_info,c,msgtemp);
	return 0;
    }

    if (arg3[0] =='\0') {
	message_send_text(c,message_type_info,c,"usage: /cg <command> <username> [<group(s)>]");
	return 0;
    }

    for (i=0; arg3[i] != '\0'; i++) {
	if (arg3[i] == '1') groups |= 1;
	else if (arg3[i] == '2') groups |= 2;
	else if (arg3[i] == '3') groups |= 4;
	else if (arg3[i] == '4') groups |= 8;
	else if (arg3[i] == '5') groups |= 16;
	else if (arg3[i] == '6') groups |= 32;
	else if (arg3[i] == '7') groups |= 64;
	else if (arg3[i] == '8') groups |= 128;
	else {
	    snprintf(msgtemp, sizeof(msgtemp), "got bad group: %c", arg3[i]);
	    message_send_text(c,message_type_info,c,msgtemp);
	    return 0;
	}
    }

    if (!std::strcmp(command,"add") || !std::strcmp(command,"a")) {
	account_set_command_groups(account, usergroups | groups);
	snprintf(msgtemp, sizeof(msgtemp), "groups %.64s has been added to user: %.64s", arg3, username);
	message_send_text(c,message_type_info,c,msgtemp);
	return 0;
    }

    if (!std::strcmp(command,"del") || !std::strcmp(command,"d")) {
	account_set_command_groups(account, usergroups & (255 - groups));
	snprintf(msgtemp, sizeof(msgtemp), "groups %.64s has been deleted from user: %.64s", arg3, username);
	message_send_text(c,message_type_info,c,msgtemp);
	return 0;
    }

    snprintf(msgtemp, sizeof(msgtemp), "got unknown command: %.128s", command);
    message_send_text(c,message_type_info,c,msgtemp);
    return 0;
}

static int _handle_topic_command(t_connection * c, char const * text)
{
  char const * channel_name;
  char const * topic;
  char * tmp;
  t_channel * channel;
  int  do_save = NO_SAVE_TOPIC;

  channel_name = skip_command(text);

  if ((topic = std::strchr(channel_name,'"')))
  {
    tmp = (char *)topic;
    for (tmp--;tmp[0]==' ';tmp--);
    tmp[1]='\0';
    topic++;
    tmp  = std::strchr((char *)topic,'"');
    if (tmp) tmp[0]='\0';
  }

  if (!(conn_get_channel(c))) {
    message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
    return -1;
  }

  if (channel_name[0]=='\0')
  {
    if (channel_get_topic(channel_get_name(conn_get_channel(c))))
    {
      snprintf(msgtemp, sizeof(msgtemp), "%.64s topic: %.128s",channel_get_name(conn_get_channel(c)),channel_get_topic(channel_get_name(conn_get_channel(c))));
    }
    else
    {
      snprintf(msgtemp, sizeof(msgtemp), "%.64s topic: no topic",channel_get_name(conn_get_channel(c)));
    }
    message_send_text(c,message_type_info,c,msgtemp);

    return 0;
  }

  if (!(topic))
  {
    if (channel_get_topic(channel_name))
    {
      snprintf(msgtemp, sizeof(msgtemp), "%.64s topic: %.128s",channel_name, channel_get_topic(channel_name));
    }
    else
    {
      snprintf(msgtemp, sizeof(msgtemp), "%.64s topic: no topic",channel_name);
    }
    message_send_text(c,message_type_info,c,msgtemp);
    return 0;
  }

  if (!(channel = channellist_find_channel_by_name(channel_name,conn_get_country(c),realm_get_name(conn_get_realm(c)))))
  {
    snprintf(msgtemp, sizeof(msgtemp), "no such channel, can't set topic");
    message_send_text(c,message_type_error,c,msgtemp);
    return -1;
  }

  if (std::strlen(topic) >= MAX_TOPIC_LEN)
  {
    snprintf(msgtemp, sizeof(msgtemp), "max topic length exceeded (max %d symbols)", MAX_TOPIC_LEN);
    message_send_text(c,message_type_error,c,msgtemp);
    return -1;
  }

  channel_name = channel_get_name(channel);

  if (!(account_is_operator_or_admin(conn_get_account(c),channel_name))) {
	snprintf(msgtemp, sizeof(msgtemp), "You must be at least a Channel Operator of %.64s to set the topic",channel_name);
	message_send_text(c,message_type_error,c,msgtemp);
	return -1;
  }

  if (channel_get_permanent(channel))
    do_save = DO_SAVE_TOPIC;

  channel_set_topic(channel_name, topic, do_save);

  snprintf(msgtemp, sizeof(msgtemp), "%.64s topic: %.128s",channel_name, topic);
  message_send_text(c,message_type_info,c,msgtemp);

  return 0;
}

static int _handle_moderate_command(t_connection * c, char const * text)
{
  unsigned oldflags;
  t_channel * channel;

  if (!(channel = conn_get_channel(c))) {
    message_send_text(c,message_type_error,c,"This command can only be used inside a channel.");
    return -1;
  }

  if (!(account_is_operator_or_admin(conn_get_account(c),channel_get_name(channel)))) {
	message_send_text(c,message_type_error,c,"You must be at least a Channel Operator to use this command.");
	return -1;
  }

  oldflags = channel_get_flags(channel);

  if (channel_set_flags(channel, oldflags ^ channel_flags_moderated)) {
	eventlog(eventlog_level_error,__FUNCTION__,"could not set channel %s flags",channel_get_name(channel));
	message_send_text(c,message_type_error,c,"Unable to change channel flags.");
	return -1;
  }
  else {
  	if (oldflags & channel_flags_moderated)
		channel_message_send(channel,message_type_info,c,"Channel is now unmoderated");
	else
		channel_message_send(channel,message_type_info,c,"Channel is now moderated");
  }

  return 0;
}

static void _reset_d1_stats(t_account *account, t_clienttag ctag, t_connection *c)
{
    account_set_normal_level(account,ctag,0);
    account_set_normal_strength(account,ctag,0),
    account_set_normal_magic(account,ctag,0),
    account_set_normal_dexterity(account,ctag,0),
    account_set_normal_vitality(account,ctag,0),
    account_set_normal_gold(account,ctag,0);

    snprintf(msgtemp, sizeof(msgtemp), "Resetted %.64s's %.64s Stats",account_get_name(account),clienttag_get_title(ctag));
    message_send_text(c,message_type_info,c,msgtemp);
}

static void _reset_scw2_stats(t_account *account, t_clienttag ctag, t_connection *c)
{
    LadderList* ladderList;
    unsigned int uid = account_get_uid(account);

    account_set_normal_wins(account,ctag,0);
    account_set_normal_losses(account,ctag,0);
    account_set_normal_draws(account,ctag,0);
    account_set_normal_disconnects(account,ctag,0);

    // normal, current
    if (account_get_ladder_rating(account,ctag,ladder_id_normal)>0) {
	account_set_ladder_wins(account,ctag,ladder_id_normal,0);
	account_set_ladder_losses(account,ctag,ladder_id_normal,0);
	account_set_ladder_draws(account,ctag,ladder_id_normal,0);
	account_set_ladder_disconnects(account,ctag,ladder_id_normal,0);
	account_set_ladder_rating(account,ctag,ladder_id_normal,0);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_normal,ctag,ladder_sort_highestrated,ladder_time_current));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_normal,ctag,ladder_sort_mostwins,ladder_time_current));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_normal,ctag,ladder_sort_mostgames,ladder_time_current));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
    }

    // ironman, current
    if (account_get_ladder_rating(account,ctag,ladder_id_ironman)>0) {
	account_set_ladder_wins(account,ctag,ladder_id_ironman,0);
	account_set_ladder_losses(account,ctag,ladder_id_ironman,0);
	account_set_ladder_draws(account,ctag,ladder_id_ironman,0);
	account_set_ladder_disconnects(account,ctag,ladder_id_ironman,0);
	account_set_ladder_rating(account,ctag,ladder_id_ironman,0);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_ironman,ctag,ladder_sort_highestrated,ladder_time_current));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_ironman,ctag,ladder_sort_mostwins,ladder_time_current));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_ironman,ctag,ladder_sort_mostgames,ladder_time_current));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
    }

    // normal, active
    if (account_get_ladder_active_rating(account,ctag,ladder_id_normal)>0) {
	account_set_ladder_active_wins(account,ctag,ladder_id_normal,0);
	account_set_ladder_active_losses(account,ctag,ladder_id_normal,0);
	account_set_ladder_active_draws(account,ctag,ladder_id_normal,0);
	account_set_ladder_active_disconnects(account,ctag,ladder_id_normal,0);
	account_set_ladder_active_rating(account,ctag,ladder_id_normal,0);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_normal,ctag,ladder_sort_highestrated,ladder_time_active));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_normal,ctag,ladder_sort_mostwins,ladder_time_active));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_normal,ctag,ladder_sort_mostgames,ladder_time_active));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
    }

    // ironman, active
    if (account_get_ladder_active_rating(account,ctag,ladder_id_ironman)>0) {
	account_set_ladder_active_wins(account,ctag,ladder_id_ironman,0);
	account_set_ladder_active_losses(account,ctag,ladder_id_ironman,0);
	account_set_ladder_active_draws(account,ctag,ladder_id_ironman,0);
	account_set_ladder_active_disconnects(account,ctag,ladder_id_ironman,0);
	account_set_ladder_active_rating(account,ctag,ladder_id_ironman,0);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_ironman,ctag,ladder_sort_highestrated,ladder_time_active));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_ironman,ctag,ladder_sort_mostwins,ladder_time_active));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
	ladderList = ladders.getLadderList(LadderKey(ladder_id_ironman,ctag,ladder_sort_mostgames,ladder_time_active));
	if (ladderList != NULL)
		ladderList->delEntry(uid);
    }

    snprintf(msgtemp, sizeof(msgtemp), "Resetted %.64s's %.64s Stats",account_get_name(account),clienttag_get_title(ctag));
    message_send_text(c,message_type_info,c,msgtemp);
}

static void _reset_w3_stats(t_account *account, t_clienttag ctag, t_connection *c)
{
    LadderList* ladderList;
    unsigned int uid = account_get_uid(account);

    account_set_ladder_level(account,ctag,ladder_id_solo,0);
    account_set_ladder_xp(account,ctag,ladder_id_solo,0);
    account_set_ladder_wins(account,ctag,ladder_id_solo,0);
    account_set_ladder_losses(account,ctag,ladder_id_solo,0);
    account_set_ladder_rank(account,ctag,ladder_id_solo,0);
    ladderList = ladders.getLadderList(LadderKey(ladder_id_solo,ctag,ladder_sort_default,ladder_time_default));
    if (ladderList != NULL)
	ladderList->delEntry(uid);

    account_set_ladder_level(account,ctag,ladder_id_team,0);
    account_set_ladder_xp(account,ctag,ladder_id_team,0);
    account_set_ladder_wins(account,ctag,ladder_id_team,0);
    account_set_ladder_losses(account,ctag,ladder_id_team,0);
    account_set_ladder_rank(account,ctag,ladder_id_team,0);
    ladderList = ladders.getLadderList(LadderKey(ladder_id_team,ctag,ladder_sort_default,ladder_time_default));
    if (ladderList != NULL)
	ladderList->delEntry(uid);

    account_set_ladder_level(account,ctag,ladder_id_ffa,0);
    account_set_ladder_xp(account,ctag,ladder_id_ffa,0);
    account_set_ladder_wins(account,ctag,ladder_id_ffa,0);
    account_set_ladder_losses(account,ctag,ladder_id_ffa,0);
    account_set_ladder_rank(account,ctag,ladder_id_ffa,0);
    ladderList = ladders.getLadderList(LadderKey(ladder_id_ffa,ctag,ladder_sort_default,ladder_time_default));
    if (ladderList != NULL)
	ladderList->delEntry(uid);
    // this would now need a way to delete the team for all members now
    //account_set_atteamcount(account,ctag,0);

    snprintf(msgtemp, sizeof(msgtemp), "Resetted %.64s's %.64s Stats",account_get_name(account),clienttag_get_title(ctag));
    message_send_text(c,message_type_info,c,msgtemp);
}

static int _handle_clearstats_command(t_connection *c, char const *text)
{
    char         dest[MAX_USERNAME_LEN];
    unsigned int i,j,all;
    t_account *  account;
    t_clienttag  ctag = 0;

    for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
    for (; text[i]==' '; i++);

    if (!text[i]) {
	message_send_text(c,message_type_error,c,"Missing user, syntax error.");
	message_send_text(c,message_type_error,c,"Usage example: /clearstats <username> <clienttag>");
	message_send_text(c,message_type_error,c,"  where <clienttag> can be any valid client or ALL for all clients");
	return 0;
    }

    for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get dest */
	if (j<sizeof(dest)-1) dest[j++] = text[i];
    dest[j] = '\0';

    account = accountlist_find_account(dest);
    if (!account) {
	message_send_text(c,message_type_error,c,"Invalid user.");
	return 0;
    }

    for (; text[i]==' '; i++);
    if (!text[i]) {
	message_send_text(c,message_type_error,c,"Missing clienttag, syntax error.");
	message_send_text(c,message_type_error,c,"Usage example: /clearstats <username> <clienttag>");
	message_send_text(c,message_type_error,c,"  where <clienttag> can be any valid client or ALL for all clients");
	return 0;
    }

    if (strcasecmp(text + i,"all")) {
	if (std::strlen(text + i) != 4) {
	    message_send_text(c,message_type_error,c,"Invalid clienttag, syntax error.");
	    return 0;
	}
	ctag = tag_case_str_to_uint(text + i);
	all = 0;
    } else all = 1;

    if (all || ctag == CLIENTTAG_DIABLORTL_UINT)
	_reset_d1_stats(account,CLIENTTAG_DIABLORTL_UINT,c);

    if (all || ctag == CLIENTTAG_DIABLOSHR_UINT)
	_reset_d1_stats(account,CLIENTTAG_DIABLOSHR_UINT,c);

    if (all || ctag == CLIENTTAG_WARCIIBNE_UINT)
	_reset_scw2_stats(account,CLIENTTAG_WARCIIBNE_UINT,c);

    if (all || ctag == CLIENTTAG_STARCRAFT_UINT)
	_reset_scw2_stats(account,CLIENTTAG_STARCRAFT_UINT,c);

    if (all || ctag == CLIENTTAG_BROODWARS_UINT)
	_reset_scw2_stats(account,CLIENTTAG_BROODWARS_UINT,c);

    if (all || ctag == CLIENTTAG_SHAREWARE_UINT)
	_reset_scw2_stats(account,CLIENTTAG_SHAREWARE_UINT,c);

    if (all || ctag == CLIENTTAG_WARCRAFT3_UINT)
	_reset_w3_stats(account,CLIENTTAG_WARCRAFT3_UINT,c);

    if (all || ctag == CLIENTTAG_WAR3XP_UINT)
	_reset_w3_stats(account,CLIENTTAG_WAR3XP_UINT,c);

    ladders.update();

    return 0;
}

}

}
