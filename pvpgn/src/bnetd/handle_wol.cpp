/*
 * Copyright (C) 2001  Marco Ziech (mmz@gmx.net)
 * Copyright (C) 2005  Bryan Biedenkapp (gatekeep@gmail.com)
 * Copyright (C) 2006,2007,2008  Pelish (pelish@gmail.com)
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
#include "handle_wol.h"

#include <cstring>
#include <cctype>
#include <cstdlib>

#include "compat/strcasecmp.h"
#include "common/irc_protocol.h"
#include "common/eventlog.h"
#include "common/bnethash.h"
#include "common/tag.h"
#include "common/util.h"
#include "common/list.h"
#include "common/addr.h"
#include "common/trans.h"

#include "compat/snprintf.h"

#include "prefs.h"
#include "command.h"
#include "irc.h"
#include "account.h"
#include "account_wrap.h"
#include "command_groups.h"
#include "channel.h"
#include "message.h"
#include "tick.h"
#include "topic.h"
#include "server.h"
#include "friends.h"
#include "clan.h"
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

typedef int (* t_wol_command)(t_connection * conn, int numparams, char ** params, char * text);

typedef struct {
	const char     * wol_command_string;
	t_wol_command    wol_command_handler;
} t_wol_command_table_row;

static int _handle_user_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_pass_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_privmsg_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_quit_command(t_connection * conn, int numparams, char ** params, char * text);

static int _handle_list_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_part_command(t_connection * conn, int numparams, char ** params, char * text);

static int _handle_cvers_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_verchk_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_apgar_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_setopt_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_serial_command(t_connection * conn, int numparams, char ** params, char * text);

static int _handle_squadinfo_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_clanbyname_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_setcodepage_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_getcodepage_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_setlocale_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_getlocale_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_getinsider_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_joingame_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_gameopt_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_finduser_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_finduserex_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_page_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_startg_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_advertr_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_advertc_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_chanchk_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_getbuddy_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_addbuddy_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_delbuddy_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_host_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_invmsg_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_invdel_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_userip_command(t_connection * conn, int numparams, char ** params, char * text);

/* Ladder server commands (we will probalby move this commands to any another handle file */
static int _handle_listsearch_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_rungsearch_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_highscore_command(t_connection * conn, int numparams, char ** params, char * text);

/* state "connected" handlers */
static const t_wol_command_table_row wol_con_command_table[] =
{
	{ "NICK"		, _handle_nick_command },
	{ "USER"		, _handle_user_command },
	{ "PING"		, _handle_ping_command },
	{ "PONG"		, _handle_pong_command },
	{ "PASS"		, _handle_pass_command },
	{ "PRIVMSG"		, _handle_privmsg_command },
	{ "QUIT"		, _handle_quit_command },

	{ "CVERS"		, _handle_cvers_command },
	{ "VERCHK"		, _handle_verchk_command },
	{ "APGAR"		, _handle_apgar_command },
	{ "SETOPT"		, _handle_setopt_command },
	{ "SERIAL"		, _handle_serial_command },
	
    /* Ladder server commands */
	{ "LISTSEARCH"	, _handle_listsearch_command },
	{ "RUNGSEARCH"	, _handle_rungsearch_command },
	{ "HIGHSCORE"	, _handle_highscore_command },

	{ NULL			, NULL }
};

/* state "logged in" handlers */
static const t_wol_command_table_row wol_log_command_table[] =
{
	{ "LIST"		, _handle_list_command },
	{ "TOPIC"		, _handle_topic_command },
	{ "JOIN"		, _handle_join_command },
	{ "NAMES"	    , _handle_names_command },
	{ "PART"		, _handle_part_command },

	{ "SQUADINFO"	, _handle_squadinfo_command },
	{ "CLANBYNAME"	, _handle_clanbyname_command },
	{ "SETCODEPAGE"	, _handle_setcodepage_command },
	{ "SETLOCALE"	, _handle_setlocale_command },
	{ "GETCODEPAGE"	, _handle_getcodepage_command },
	{ "GETLOCALE"	, _handle_getlocale_command },
	{ "GETINSIDER"	, _handle_getinsider_command },
	{ "JOINGAME"	, _handle_joingame_command },
	{ "GAMEOPT"		, _handle_gameopt_command },
    { "FINDUSER"	, _handle_finduser_command },
	{ "FINDUSEREX"	, _handle_finduserex_command },
	{ "PAGE"		, _handle_page_command },
	{ "STARTG"		, _handle_startg_command },
    { "ADVERTR"	    , _handle_advertr_command },
	{ "ADVERTC"		, _handle_advertc_command },
    { "CHANCHK"	    , _handle_chanchk_command },
    { "GETBUDDY"    , _handle_getbuddy_command },
    { "ADDBUDDY"    , _handle_addbuddy_command },
    { "DELBUDDY"    , _handle_delbuddy_command },
    { "TIME"        , _handle_time_command },
	{ "KICK"		, _handle_kick_command },
	{ "MODE"		, _handle_mode_command },
	{ "HOST"		, _handle_host_command },
	{ "INVMSG"      , _handle_invmsg_command },
	{ "INVDEL"      , _handle_invdel_command },
	{ "USERIP"      , _handle_userip_command },

	{ NULL			, NULL }
};

extern int handle_wol_con_command(t_connection * conn, char const * command, int numparams, char ** params, char * text)
{
  t_wol_command_table_row const *p;

  for (p = wol_con_command_table; p->wol_command_string != NULL; p++) {
    if (strcasecmp(command, p->wol_command_string)==0) {
	  if (p->wol_command_handler != NULL)
		  return ((p->wol_command_handler)(conn,numparams,params,text));
	}
  }
  return -1;
}

extern int handle_wol_log_command(t_connection * conn, char const * command, int numparams, char ** params, char * text)
{
  t_wol_command_table_row const *p;

  for (p = wol_log_command_table; p->wol_command_string != NULL; p++) {
    if (strcasecmp(command, p->wol_command_string)==0) {
	  if (p->wol_command_handler != NULL)
		  return ((p->wol_command_handler)(conn,numparams,params,text));
	}
  }
  return -1;
}

static int handle_wol_authenticate(t_connection * conn, char const * passhash)
{
    t_account * a;
    char const * tempapgar;
    char const * temphash;
    char const * username;

    if (!conn) {
        ERROR0("got NULL connection");
        return 0;
    }
    if (!passhash) {
        eventlog(eventlog_level_error,__FUNCTION__,"got NULL passhash");
        return 0;
    }
    username = conn_get_loggeduser(conn);
    if (!username) {
        /* redundant sanity check */
        eventlog(eventlog_level_error,__FUNCTION__,"got NULL conn->protocol.loggeduser");
        return 0;
    }
    a = accountlist_find_account(username);
    if (!a) {
        /* FIXME: Send real error code */
        message_send_text(conn,message_type_notice,NULL,"Authentication failed.");
        return 0;
    }
    tempapgar = conn_wol_get_apgar(conn);
    temphash = account_get_wol_apgar(a);

    if(!temphash) {
        /* Acount auto creating */
        account_set_wol_apgar(a,tempapgar);
        temphash = account_get_wol_apgar(a);
    }
    if(!tempapgar) {
        irc_send(conn,RPL_BAD_LOGIN,":You have specified an invalid password for that nickname."); /* bad APGAR */
        //std::sprintf(temp,":Closing Link %s[Some.host]:(Password needed for that nickname.)",conn_get_loggeduser(conn));
        //message_send_text(conn,message_type_error,conn,temp);
        conn_increment_passfail_count(conn);
        return 0;
    }
    if(std::strcmp(temphash,tempapgar) == 0) {
        /* LOGIN is OK. We sends motd */
        conn_login(conn,a,username);
    	conn_set_state(conn,conn_state_loggedin);
    	irc_send_motd(conn);
        return 1;
    }
    else {
        irc_send(conn,RPL_BAD_LOGIN,":You have specified an invalid password for that nickname."); /* bad APGAR */
        //std::sprintf(temp,":Closing Link %s[Some.host]:(Password needed for that nickname.)",conn_get_loggeduser(conn));
        //message_send_text(conn,message_type_error,conn,temp);
        conn_increment_passfail_count(conn);
        return 0;
    }
}

extern int handle_wol_welcome(t_connection * conn)
{
    /* This function need rewrite */

    conn_set_state(conn,conn_state_bot_password);
    if (connlist_find_connection_by_accountname(conn_get_loggeduser(conn))) {
        message_send_text(conn,message_type_notice,NULL,"This account is already logged in, use another account.");
        return -1;
    }

    if (conn_wol_get_apgar(conn)) {
        handle_wol_authenticate(conn,conn_wol_get_apgar(conn));
    }
    else {
        message_send_text(conn,message_type_notice,NULL,"No APGAR command received!");
    }

    return 0;
}

static int handle_wol_send_claninfo(t_connection * conn, t_clan * clan)
{
	char _temp[MAX_IRC_MESSAGE_LEN];
	unsigned int clanid;
	const char * clantag;
	const char * clanname;

	std::memset(_temp,0,sizeof(_temp));

    if (!conn) {
        ERROR0("got NULL connection");
        return -1;
    }
  
    if (clan) {
        clanid = clan_get_clanid(clan);
	    clantag = clantag_to_str(clan_get_clantag(clan));
	    clanname = clan_get_name(clan);
        snprintf(_temp, sizeof(_temp), "%u`%s`%s`0`0`1`0`0`0`0`0`0`0`x`x`x",clanid,clanname,clantag);
        irc_send(conn,RPL_BATTLECLAN,_temp);
    }
    else {
        snprintf(_temp, sizeof(_temp), ":ID does not exist");
	    irc_send(conn,ERR_IDNOEXIST,_temp);
    }
    return 0;
}

/* Commands: */

static int _handle_user_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /**
     *  In WOL isnt used USER command (only for backward compatibility)
   	 *  RFC 2812 says:
     *  USER <user> <mode> <unused> :<realname>
     *
     *  There is WOL imput expected:
     *  USER UserName HostName irc.westwood.com :RealName
     */

    char * user = NULL;
    t_account * a;

    user = (char *)conn_get_loggeduser(conn);

   	if (conn_get_user(conn)) {
        /* FIXME: Send real ERROR code/message */
	    irc_send(conn,ERR_ALREADYREGISTRED,":You are already registred");
    }
    else {
        eventlog(eventlog_level_debug,__FUNCTION__,"[%d][** WOL **] got USER: user=\"%s\"",conn_get_socket(conn),user);

        a = accountlist_find_account(user);
        if (!a) {
            /* Auto-create account */
            t_account * tempacct;
            t_hash pass_hash;
            char * pass = xstrdup(conn_wol_get_apgar(conn)); /* FIXME: Do not use bnet passhash when we have wol passhash */

            for (unsigned j=0; j<std::strlen(pass); j++)
                if (std::isupper((int)pass[j])) pass[j] = std::tolower((int)pass[j]);

            bnet_hash(&pass_hash,std::strlen(pass),pass);

            tempacct = accountlist_create_account(user,hash_get_str(pass_hash));
            if (!tempacct) {
                /* FIXME: Send real ERROR code/message */
                irc_send(conn,RPL_BAD_LOGIN,":Account creating failed");
                return 0;
            }
            if (pass)
                xfree((void *)pass);

			conn_set_user(conn,user);
			conn_set_owner(conn,user);
			if (conn_get_loggeduser(conn))
				handle_wol_welcome(conn); /* only send the welcome if we have USER and NICK */
    	}
		else {
            conn_set_user(conn,user);
            conn_set_owner(conn,user);
            if (conn_get_loggeduser(conn))
                handle_wol_welcome(conn); /* only send the welcome if we have USER and NICK */
        }
   	}
    return 0;
}

static int _handle_pass_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /**
     * PASS is not used in WOL
     * only for backward compatibility sent client PASS supersecret
     * real password sent client by apgar command
     */

	return 0;
}

static int _handle_privmsg_command(t_connection * conn, int numparams, char ** params, char * text)
{
     /**
      * Pelish: FIXME delete NICSERV and add support for matchbot.
      */

	if ((numparams>=1)&&(text))
	{
	    int i;
	    char ** e;

	    e = irc_get_listelems(params[0]);
	    /* FIXME: support wildcards! */

		/* start amadeo: code was sent by some unkown fellow of pvpgn (maybe u wanna give us your name
		   for any credits), it adds nick-registration, i changed some things here and there... */
	    for (i=0;((e)&&(e[i]));i++) {
    		if (strcasecmp(e[i],"NICKSERV")==0) {
 				char * pass;
				char * p;

 				pass = std::strchr(text,' ');
 				if (pass)
 		    		*pass++ = '\0';

				if (strcasecmp(text,"identify")==0) {
				    switch (conn_get_state(conn)) {
					case conn_state_bot_password:
					{
							if (pass) {
 		    				t_hash h;

						for (p = pass; *p; p++)
						    if (std::isupper((int)*p)) *p = std::tolower(*p);
 		    				bnet_hash(&h,std::strlen(pass),pass);
 		    				irc_authenticate(conn,hash_get_str(h));
 					    }
							else {
                                 message_send_text(conn,message_type_notice,NULL,"Syntax: IDENTIFY <password> (max 16 characters)");
					    }
					    break;
					}
					case conn_state_loggedin:
					{
					    message_send_text(conn,message_type_notice,NULL,"You don't need to IDENTIFY");
					    break;
					}
					default: ;
					    eventlog(eventlog_level_trace,__FUNCTION__,"got /msg in unexpected connection state (%s)",conn_state_get_str(conn_get_state(conn)));
				    }
				}
				else if (strcasecmp(text,"register")==0) {
					unsigned int j;
					t_hash       passhash;
					t_account  * temp;
					char         msgtemp[MAX_MESSAGE_LEN];
					char       * username=(char *)conn_get_loggeduser(conn);

					if (account_check_name(username)<0) {
						message_send_text(conn,message_type_error,conn,"Account name contains invalid symbol!");
						break;
					}

					if(!prefs_get_allow_new_accounts()){
						message_send_text(conn,message_type_error,conn,"Account creation is not allowed");
						break;
					}

					if (!pass || pass[0]=='\0' || (std::strlen(pass)>16) ) {
						message_send_text(conn,message_type_error,conn,":Syntax: REGISTER <password> (max 16 characters)");
						break;
					}

					for (j=0; j<std::strlen(pass); j++)
						if (std::isupper((int)pass[j])) pass[j] = std::tolower((int)pass[j]);

					bnet_hash(&passhash,std::strlen(pass),pass);

					snprintf(msgtemp, sizeof(msgtemp), "Trying to create account \"%s\" with password \"%s\"",username,pass);
					message_send_text(conn,message_type_info,conn,msgtemp);

					temp = accountlist_create_account(username,hash_get_str(passhash));
					if (!temp) {
						message_send_text(conn,message_type_error,conn,"Failed to create account!");
						eventlog(eventlog_level_debug,__FUNCTION__,"[%d] account \"%s\" not created (failed)",conn_get_socket(conn),username);
						conn_unget_chatname(conn,username);
						break;
					}

					snprintf(msgtemp, sizeof(msgtemp), "Account "UID_FORMAT" created.",account_get_uid(temp));
					message_send_text(conn,message_type_info,conn,msgtemp);
					eventlog(eventlog_level_debug,__FUNCTION__,"[%d] account \"%s\" created",conn_get_socket(conn),username);
					conn_unget_chatname(conn,username);
				}
				else {
					char tmp[MAX_IRC_MESSAGE_LEN+1];

 					message_send_text(conn,message_type_notice,NULL,"Invalid arguments for NICKSERV");
					snprintf(tmp, sizeof(tmp), ":Unrecognized command \"%s\"", text);
					message_send_text(conn,message_type_notice,NULL,tmp);
 				}
 	        }
			else if (conn_get_state(conn)==conn_state_loggedin) {
				if (e[i][0]=='#') {
					/* channel message */
					t_channel * channel;
					char msgtemp[MAX_MESSAGE_LEN];

					if ((channel = channellist_find_channel_by_name(irc_convert_ircname(e[i]),NULL,NULL))) {
						if ((std::strlen(text)>=9)&&(std::strncmp(text,"\001ACTION ",8)==0)&&(text[std::strlen(text)-1]=='\001')) {
							/* at least "\001ACTION \001" */
							/* it's a CTCP ACTION message */
							text = text + 8;
							text[std::strlen(text)-1] = '\0';
							channel_message_send(channel,message_type_emote,conn,text);
						}
						else {
                            if (text[0] == '/') {
                                /* "/" commands (like "/help..." */
                                handle_command(conn, text);
                            }
                            else {
                                channel_message_log(channel, conn, 1, text);
                                channel_message_send(channel,message_type_talk,conn,text);
                            }
						}
					}
					else {
                        snprintf(msgtemp,sizeof(msgtemp),"%s :No such channel", e[0]);
                        irc_send(conn,ERR_NOSUCHCHANNEL,msgtemp);
					}
	    	    }
				else {
					/* whisper */
					t_connection * user;

					if ((user = connlist_find_connection_by_accountname(e[i])))
					{
						message_send_text(user,message_type_whisper,conn,text);
					}
					else
					{
						irc_send(conn,ERR_NOSUCHNICK,":No such user");
					}
	    	    }
	        }
	    }
	    if (e)
	         irc_unget_listelems(e);
	}
	else
        irc_send(conn,ERR_NEEDMOREPARAMS,"PRIVMSG :Not enough parameters");
	return 0;
}

static int _handle_list_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_elem const * curr;

    irc_send(conn,RPL_LISTSTART,"Channel :Users Names"); /* backward compatibility */

    if ((numparams == 0) || ((params[0]) && (params[1]) && (std::strcmp(params[0], params[1]) != 0))) {
        /**
         * LIST all chat channels 
         * Emperor sends as params[0] == -1 if want QuickMatch channels too, 0 if not.
         * This sends also NOX but we dunno why.
         * DUNE 2000 use params[0] to determine channels by channeltype
         */

        LIST_TRAVERSE_CONST(channellist(),curr) {
   	         t_channel const * channel = (const t_channel*)elem_get_data(curr);
             char const * tempname;

             tempname = irc_convert_channel(channel,conn);

             if (std::strstr(tempname,"_game") == NULL) { /* FIXME: Delete this if games are not in channels */
                sprintf(temp,"%s %u ",tempname,channel_get_length(channel));

	        	if (channel_get_flags(channel) & channel_flags_permanent)
                    std::strcat(temp,"1");  /* Official channel */
                else
                    std::strcat(temp,"0");  /* User channel */

                if (conn_get_clienttag(conn)==CLIENTTAG_WCHAT_UINT)
                    std::strcat(temp,":");     /* WOLv1 ends by ":" */
                else
                    std::strcat(temp," 388");  /* WOLv2 ends by "388" */

                if (std::strlen(temp)>MAX_IRC_MESSAGE_LEN)
                    WARN0("LISTREPLY length exceeded");

                irc_send(conn,RPL_CHANNEL,temp);
             }
        }
    }
    /**
    *  Known channel game types:
    *  0 = Westwood Chat channels, 1 = Command & Conquer Win95 channels, 2 = Red Alert Win95 channels,
    *  3 = Red Alert Counterstrike channels, 4 = Red Alert Aftermath channels, 5 = CnC Sole Survivor channels,
    *  12 = C&C Renegade channels, 14 = Dune 2000 channels, 16 = Nox channels, 18 = Tiberian Sun channels,
    *  21 = Red Alert 1 v 3.03 channels, 31 = Emperor: Battle for Dune, 33 = Red Alert 2,
    *  37 = Nox Quest channels, 38,39,40 = Quickgame channels, 41 = Yuri's Revenge
	*/
    if ((numparams == 0) || ((params[0]) && (params[1]) && (std::strcmp(params[0], params[1]) == 0))) {
   		    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] LIST [Game]");
       	    LIST_TRAVERSE_CONST(channellist(),curr)
            {
    		    t_channel const * channel = (const t_channel*)elem_get_data(curr);
			    t_connection * m;
        	    char const * tempname;
				char * topic = channel_get_topic(channel_get_name(channel));

        	    tempname = irc_convert_channel(channel,conn);
        	    
				if((channel_wol_get_game_type(channel) != 0)) {
					m = channel_get_first(channel);
					if((tag_channeltype_to_uint(channel_wol_get_game_type(channel)) == conn_get_clienttag(conn)) || ((numparams == 0))) {
						eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] List [Channel: \"_game\"] %s %u 0 %u %u %s %u 128::",tempname,
									 channel_get_length(channel),channel_wol_get_game_type(channel),channel_wol_get_game_tournament(channel),
									 channel_wol_get_game_extension(channel),channel_wol_get_game_ownerip(channel));

						if (topic) {
	    					eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] List [Channel: \"_game\"] %s %u 0 %u %u %s %u 128::%s",tempname,
									 channel_get_length(channel),channel_wol_get_game_type(channel),channel_wol_get_game_tournament(channel),
									 channel_wol_get_game_extension(channel),channel_wol_get_game_ownerip(channel),topic);
							/**
							*  The layout of the game list entry is something like this:
                            *
							*   #game_channel_name users isofficial gameType gameIsTournment gameExtension longIP locked::topic
							*   by isofficial is used always 0 (its backward compatibility with chat channels)
							*   locked can be 128==unlocked or 384==locked
							*   gameExtension is used for game_sorting, i.e. in RedAlert1v3 or TSUN/TSXP for spliting
							*   extension pack, also is used for spliting Clan_game or quick_match from normal game
							*/
	        		        if (std::strlen(tempname)+1+20+1+1+std::strlen(topic)<MAX_IRC_MESSAGE_LEN)
                                   snprintf(temp, sizeof(temp), "%s %u 0 %u %u %s %u 128::%s",tempname,
                                         channel_get_length(channel),channel_wol_get_game_type(channel),channel_wol_get_game_tournament(channel),
										 channel_wol_get_game_extension(channel),channel_wol_get_game_ownerip(channel),topic);
           	        		else
         			               eventlog(eventlog_level_warn,__FUNCTION__,"LISTREPLY length exceeded");
                        }
						else {
        						if (std::strlen(tempname)+1+20+1+1<MAX_IRC_MESSAGE_LEN)
	    							snprintf(temp, sizeof(temp), "%s %u 0 %u %u %s %u 128::",tempname,channel_get_length(channel),channel_wol_get_game_type(channel),
											channel_wol_get_game_tournament(channel),channel_wol_get_game_extension(channel),channel_wol_get_game_ownerip(channel));
			                    else
            							eventlog(eventlog_level_warn,__FUNCTION__,"LISTREPLY length exceeded");
						}
					}
					irc_send(conn,RPL_GAME_CHANNEL,temp);
				}
			}
		}
    	irc_send(conn,RPL_LISTEND,":End of LIST command");
    	return 0;
}

static int _handle_quit_command(t_connection * conn, int numparams, char ** params, char * text)
{
    irc_send(conn,RPL_QUIT,":goodbye");
    if (conn_get_channel(conn))
        conn_quit_channel(conn, text);
    conn_set_state(conn, conn_state_destroy);

    return 0;
}

static int _handle_part_command(t_connection * conn, int numparams, char ** params, char * text)
{
    if ((conn_wol_get_ingame(conn) == 1)) {
        conn_wol_set_ingame(conn,0);
    }
    conn_part_channel(conn);
    return 0;
}

/**
*  Fallowing commands are only in Westwood Online protocol
*/
static int _handle_cvers_command(t_connection * conn, int numparams, char ** params, char * text)
{
    t_clienttag clienttag;

	/* Ignore command but set clienttag */

	/**
	*  Heres the imput expected:
	*  CVERS [oldvernum] [SKU]
	*
	*  SKU is specific number for any WOL game (Tiberian sun, RedAlert 2 etc.)
	*  This is the best way to set clienttag, because CVERS is the first command which
	*  client send to server.
	*/

	if (numparams == 2) {
        clienttag = tag_sku_to_uint(std::atoi(params[1]));
        if (clienttag != CLIENTTAG_WWOL_UINT)
             conn_set_clienttag(conn,clienttag);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"CVERS :Not enough parameters");
    return 0;
}

static int _handle_verchk_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_clienttag clienttag;

    /**
    *  Heres the imput expected:
    *  vercheck [SKU] [version]
    *
    *  Heres the output expected:
    *
    *  1) Update non-existant:
    *  :[servername] 379 [username] :none none none 1 [SKU] NONREQ
    *  2) Update existant:
    *  :[servername] 379 [username] :none none none [oldversnum] [SKU] REQ
    */

    if (numparams == 2) {
        clienttag = tag_sku_to_uint(std::atoi(params[0]));
        if (clienttag != CLIENTTAG_WWOL_UINT)
            conn_set_clienttag(conn,clienttag);

        snprintf(temp, sizeof(temp), ":none none none 1 %s NONREQ", params[0]);
        eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] VERCHK %s",temp);
        irc_send(conn,RPL_VERCHK_NONREQ,temp);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"VERCHK :Not enough parameters");
    return 0;
}

static int _handle_apgar_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char * apgar = NULL;

	if((numparams>=1)&&(params[0])) {
	    apgar = params[0];
	    conn_wol_set_apgar(conn,apgar);
	}
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"APGAR :Not enough parameters");
	return 0;
}

static int _handle_serial_command(t_connection * conn, int numparams, char ** params, char * text)
{
    // Ignore command
	return 0;
}

static int _handle_squadinfo_command(t_connection * conn, int numparams, char ** params, char * text)
{
	t_clan * clan;

    if ((numparams>=1)&&(params[0])) {
       if (std::strcmp(params[0], "0") == 0) {
           /* 0 == claninfo for itself */
           clan = account_get_clan(conn_get_account(conn));
           handle_wol_send_claninfo(conn,clan);
       }
       else {
           /* claninfo for clanid (params[0]) */
           clan = clanlist_find_clan_by_clanid(std::atoi(params[0]));
           handle_wol_send_claninfo(conn,clan);
       }
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"SQUADINFO :Not enough parameters");
	return 0;
}

static int _handle_clanbyname_command(t_connection * conn, int numparams, char ** params, char * text)
{
    t_clan * clan;

    if ((numparams>=1)&&(params[0])) {
        clan = account_get_clan(accountlist_find_account(params[0]));
        handle_wol_send_claninfo(conn,clan);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"CLANBYNAME :Not enough parameters");
    return 0;
}

static int _handle_setopt_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char ** elems;

    /**
    *   This is option for enabling/disabling Page and Find user.
    *
    *   Heres the input expected:
    *   SETOPT 17,32
    *
    *   First parameter: 16 == FindDisabled 17 == FindEnabled
    *   Second parameter: 32 == PageDisabled 33 == PageEnabled
    */

    if ((numparams>=1)&&(params[0])) {
        elems = irc_get_listelems(params[0]);

        if ((elems)&&(elems[0])&&(elems[1])) {
            conn_wol_set_findme(conn,std::atoi(elems[0]));
            conn_wol_set_pageme(conn,std::atoi(elems[1]));
        }
        if (elems)
            irc_unget_listelems(elems);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"SETOPT :Not enough parameters");
    return 0;
}

static int _handle_setcodepage_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char * codepage = NULL;

    if((numparams>=1)&&(params[0])) {
        codepage = params[0];
        conn_wol_set_codepage(conn,std::atoi(codepage));
        irc_send(conn,RPL_SET_CODEPAGE,codepage);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"SETCODEPAGE :Not enough parameters");
    return 0;
}

static int _handle_getcodepage_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	char _temp[MAX_IRC_MESSAGE_LEN];

	std::memset(temp,0,sizeof(temp));
	std::memset(_temp,0,sizeof(_temp));

	if((numparams>=1)&&(params[0])) {
	    int i;
	    for (i=0; i<numparams; i++) {
    		t_connection * user;
            int codepage;

    		if (user = connlist_find_connection_by_accountname(params[i]))
    		    codepage = conn_wol_get_codepage(user);
    		if (!codepage)
    		    codepage = 0;

    		snprintf(_temp, sizeof(_temp), "%s`%u", params[i], codepage);
    		std::strcat(temp,_temp);

    		if(i < numparams-1)
    		    std::strcat(temp,"`");
	    }
   	    irc_send(conn,RPL_GET_CODEPAGE,temp);
	}
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"GETCODEPAGE :Not enough parameters");
	return 0;
}

static int _handle_setlocale_command(t_connection * conn, int numparams, char ** params, char * text)
{
    t_account * account = conn_get_account(conn);
    int locale;

    if ((numparams>=1)&&(params[0])) {
       locale = std::atoi(params[0]);
       account_set_locale(account,locale);
       irc_send(conn,RPL_SET_LOCALE,params[0]);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"SETLOCALE :Not enough parameters");
    return 0;
}

static int _handle_getlocale_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	char _temp[MAX_IRC_MESSAGE_LEN];

	std::memset(temp,0,sizeof(temp));
	std::memset(_temp,0,sizeof(_temp));

	if ((numparams>=1)&&(params[0])) {
        int i;
        for (i=0; i<numparams; i++) {
            t_account * account;
            int locale;

    		if (account = accountlist_find_account(params[i]))
    		    locale = account_get_locale(account);
    		if (!locale)
    		    locale = 0;
    		snprintf(_temp, sizeof(_temp), "%s`%u", params[i], locale);
    		std::strcat(temp,_temp);
    		if (i < numparams-1)
    		    std::strcat(temp,"`");
	    }
	    irc_send(conn,RPL_GET_LOCALE,temp);
	}
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"GETLOCALE :Not enough parameters");
	return 0;
}

static int _handle_getinsider_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];

    /**
     * Here is imput expected:
     *   GETINSIDER [nickname]
     * Here is output expected:
     *   :[servername] 399 [nick] [nickname]`0
     */

	std::memset(_temp,0,sizeof(_temp));

    if ((numparams>=1)&&(params[0])) {
        snprintf(_temp, sizeof(_temp), "%s`%u", params[0], 0);
        irc_send(conn,RPL_GET_INSIDER,_temp);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"GETINSIDER :Not enough parameters");
    return 0;
}

static int _handle_joingame_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];

	std::memset(_temp,0,sizeof(_temp));

	/**
	*  Basically this has 2 modes, Join Game and Create Game output is pretty much
	*  the same...input and output of JOINGAME is listed below. By the way, there is a
	*  hack in here, for Red Alert 1, it use's JOINGAME for some reason to join a lobby channel.
	*
	*   Here is WOLv1 input expected:
    *   JOINGAME [#Game_channel_name] [MinPlayers] [MaxPlayers] [channelType] 1 1 [gameIsTournament]
    *   Knowed channelTypes (0-chat, 1-cnc, 2-ra1, 3-racs, 4-raam, 5-solsurv... listed in tag.cpp)
    *
	*   Here is WOLv2 input expected:
	*   JOINGAME [#Game_channel_name] [MinPlayers] [MaxPlayers] [channelType] unknown unknown [gameIsTournament] [gameExtension] [password_optional]
	*
	*   Heres the output expected:
	*   user!WWOL@hostname JOINGAME [MinPlayers] [MaxPlayers] [channelType] unknown clanID [longIP] [gameIsTournament] :[#Game_channel_name]
	*/
	if((numparams==2)) {
	    char ** e;

	    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] JOINGAME: * Join * (%s, %s)",
		     params[0],params[1]);

	    e = irc_get_listelems(params[0]);
	    if ((e)&&(e[0])) {
    		char const * wolname = irc_convert_ircname(e[0]);
    		char * old_channel_name = NULL;
			t_channel * channel;
	   	 	t_channel * old_channel = conn_get_channel(conn);

            channel = channellist_find_channel_by_name(wolname,NULL,NULL);

            if (channel == NULL) {
			     snprintf(_temp, sizeof(_temp), "%s :Game channel has closed",e[0]);
			     irc_send(conn,ERR_GAMEHASCLOSED,_temp);
			     if (e)
		             irc_unget_listelems(e);
     	         return 0;
            }

            if (channel_get_length(channel) == channel_get_max(channel)) {
	   	 	     snprintf(_temp, sizeof(_temp), "%s :Channel is full",e[0]);
                 irc_send(conn,ERR_CHANNELISFULL,_temp);
			     if (e)
		             irc_unget_listelems(e);
			     return 0;
            }

            if (channel_check_banning(channel,conn)) {
	   	 	     snprintf(_temp, sizeof(_temp), "%s :You are banned from that channel.",e[0]);
	   	 	     irc_send(conn,ERR_BANNEDFROMCHAN,_temp);
	   	 	     if (e)
	   	 	         irc_unget_listelems(e);
	   	 	     return 0;
            }

			conn_wol_set_ingame(conn,1);

			if (old_channel)
   	  		   old_channel_name = xstrdup(irc_convert_channel(old_channel,conn));

			if ((!(wolname)) || (conn_set_channel(conn,wolname)<0))	{
				irc_send(conn,ERR_NOSUCHCHANNEL,":JOINGAME failed");
				conn_wol_set_ingame(conn,0);
			}
			else {
    			channel = conn_get_channel(conn);

			    if (channel!=old_channel) {
                    if (conn_get_clienttag(conn) == CLIENTTAG_WCHAT_UINT) {
                        /* WOLv1 JOINGAME message */
                        std::sprintf(_temp,"2 %u %u 1 1 %u :%s", channel_get_length(channel), channel_wol_get_game_type(channel),
                                    channel_wol_get_game_tournament(channel), irc_convert_channel(channel,conn));
                    }
                    else {
                        /* WOLv2 JOINGAME message with BATTLECLAN support */
                        t_clan * clan = account_get_clan(conn_get_account(conn));
                        unsigned int clanid = 0;

                        if (clan)
                            clanid = clan_get_clanid(clan);

                        std::sprintf(_temp,"1 %u %u 1 %u %u %u :%s", channel_get_length(channel), channel_wol_get_game_type(channel),
                                     clanid, conn_get_addr(conn), channel_wol_get_game_tournament(channel), irc_convert_channel(channel,conn));
                    }

                    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] JOINGAME [Game Options] (%s) [Game Owner] (%s)",_temp,channel_wol_get_game_owner(channel));
   					channel_set_userflags(conn);
                    /* we have to send the JOINGAME acknowledgement */
   					channel_message_send(channel,message_wol_joingame,conn,_temp);

					irc_send_topic(conn, channel);

  					irc_send_rpl_namreply(conn,channel);
				}
				else {
				    irc_send(conn,ERR_NOSUCHCHANNEL,":JOINGAME failed");
				}
			}
			if (old_channel_name) xfree((void *)old_channel_name);
		}
    		if (e)
		    irc_unget_listelems(e);
    	    return 0;
	}
	/**
	* HACK: Check for 3 params, because in that case we must be running RA1
	* then just forward to _handle_join_command
	*/
	else if((numparams==3)) {
	    _handle_join_command(conn,numparams,params,text);
	}
	else if((numparams>=7)) {
	    char ** e;

	    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] JOINGAME: * Create * (%s, %s)",
		     params[0],params[1]);

        if((numparams==7)) {
            /* WOLv1 JOINGAME Create */
       	    snprintf(_temp, sizeof(_temp), "%s %s %s %s 0 %s :%s",params[1],params[2],params[3],params[4],params[6],params[0]);
        }
            /* WOLv2 JOINGAME Create */
        else if((numparams>=8)) {
            t_clan * clan = account_get_clan(conn_get_account(conn));
            unsigned int clanid = 0;

            if (clan)
                clanid = clan_get_clanid(clan);
            snprintf(_temp, sizeof(_temp), "%s %s %s %s %u %u %s :%s",params[1],params[2],params[3],params[4],clanid,conn_get_addr(conn),params[6],params[0]);
        }
	    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] JOINGAME [Game Options] (%s)",_temp);

		conn_wol_set_ingame(conn,1);

	    e = irc_get_listelems(params[0]);
	    if ((e)&&(e[0])) {
    		char const * wolname = irc_convert_ircname(e[0]);
    		char * old_channel_name = NULL;
	   	 	t_channel * old_channel = conn_get_channel(conn);

			if (old_channel)
			  old_channel_name = xstrdup(irc_convert_channel(old_channel,conn));

/* This will be in future: Start GAME (no start channel) */
//			if ((!(wolname)) || ((conn_set_game(conn, wolname, "", "", game_type_none, 0))<0)) {
//				irc_send(conn,ERR_NOSUCHCHANNEL,":JOINGAME failed"); /* FIXME: be more precise; what is the real error code for that? */
//			}
//

			if ((!(wolname)) || ((conn_set_channel(conn, wolname))<0)) {
				irc_send(conn,ERR_NOSUCHCHANNEL,":JOINGAME failed"); /* FIXME: be more precise; what is the real error code for that? */
			}
			else {
				t_channel * channel;

				channel = conn_get_channel(conn);
				if (channel!=old_channel) {
					channel_set_userflags(conn);
					channel_wol_set_game_owner(channel,conn_get_chatname(conn));
					channel_wol_set_game_ownerip(channel,conn_get_addr(conn));
					channel_set_max(channel,std::atoi(params[2]));
				    /* HACK: Currently, this is the best way to set the channel game type... */
					channel_wol_set_game_type(channel,std::atoi(params[3]));
					channel_wol_set_game_tournament(channel,std::atoi(params[6]));
                    if (params[7])
                        channel_wol_set_game_extension(channel,params[7]);
                    else
                        channel_wol_set_game_extension(channel,"0");

					message_send_text(conn,message_wol_joingame,conn,_temp); /* we have to send the JOINGAME acknowledgement */

	    			irc_send_topic(conn, channel);

	    			irc_send_rpl_namreply(conn,channel);

	    		}
			}
			if (old_channel_name) xfree((void *)old_channel_name);
		}
		if (e)
	       irc_unget_listelems(e);
	}
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"JOINGAME :Not enough parameters");
	return 0;
}

static int _handle_gameopt_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];

    /**
    *  Basically this has 2 modes, Game Owner Change and Game Joinee Change what the output
    *  on this does is pretty much unknown, we just dump this to the client to deal with...
    *
    *	Heres the output expected (from game owner):
    *	user!WWOL@hostname GAMEOPT #game_channel_name :gameOptions
    *
    *	Heres the output expected (from game joinee):
    *	user!WWOL@hostname GAMEOPT game_owner_name :gameOptions
    */

    if ((numparams>=1)&&(params[0])&&(text)) {
        int i;
        char ** e;
        t_connection * user;
        t_channel * channel;

        if (conn_get_clienttag(conn) == CLIENTTAG_WCHAT_UINT) {
            /* we only send text to another client */

            channel = conn_get_channel(conn);

            for (user = channel_get_first(channel);user;user = channel_get_next()) {
                char const * name = conn_get_chatname(user);
                if (std::strcmp(conn_get_chatname(conn),name) != 0) {
                    snprintf(temp, sizeof(temp), ":%s", text);
                    message_send_text(user,message_wol_gameopt_join,conn,temp);
                }
            }
        }
        else {
            e = irc_get_listelems(params[0]);
            /* FIXME: support wildcards! */

            for (i=0;((e)&&(e[i]));i++) {
                if (e[i][0]=='#') {
                    /* game owner change */
                    if ((channel = channellist_find_channel_by_name(irc_convert_ircname(params[0]),NULL,NULL))) {
                        snprintf(temp, sizeof(temp), ":%s", text);
                        channel_message_send(channel,message_wol_gameopt_owner,conn,temp);
                    }
                    else {
                        snprintf(temp,sizeof(temp),"%s :No such channel", params[0]);
                        irc_send(conn,ERR_NOSUCHCHANNEL,temp);
                    }
                }
                else {
                    /* user change */
                    if ((user = connlist_find_connection_by_accountname(e[i]))) {
                        snprintf(temp, sizeof(temp), ":%s", text);
                        message_send_text(user,message_wol_gameopt_join,conn,temp);
                    }
                    else {
                        irc_send(conn,ERR_NOSUCHNICK,":No such user");
                    }
                }
            }
            if (e)
                irc_unget_listelems(e);
        }
	}
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"GAMEOPT :Not enough parameters");
	return 0;
}

static int _handle_finduser_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];
	char const * wolname = NULL;

	std::memset(_temp,0,sizeof(_temp));

	if ((numparams>=1)&&(params[0])) {
	    t_connection * user;

	    if((user = connlist_find_connection_by_accountname(params[0]))&&(conn_wol_get_findme(user) == 17)) {
     		wolname = irc_convert_channel(conn_get_channel(user),conn);
	        snprintf(_temp, sizeof(_temp), "0 :%s", wolname); /* User found in channel wolname */
	    }
	    else
	        snprintf(_temp, sizeof(_temp), "1 :"); /* user not loged or have not allowed find */

	    irc_send(conn,RPL_FIND_USER,_temp);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"FINDUSER :Not enough parameters");
    return 0;
}

static int _handle_finduserex_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];
	char const * wolname = NULL;

	std::memset(_temp,0,sizeof(_temp));

	if ((numparams>=1)&&(params[0])) {
	    t_connection * user;

	    if((user = connlist_find_connection_by_accountname(params[0]))&&(conn_wol_get_findme(user) == 17)) {
     		wolname = irc_convert_channel(conn_get_channel(user),conn);
     		snprintf(_temp, sizeof(_temp), "0 :%s,0", wolname); /* User found in channel wolname */
	    }
	    else
	        snprintf(_temp, sizeof(_temp), "1 :"); /* user not loged or have not allowed find */

	    irc_send(conn,RPL_FIND_USER_EX,_temp);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"FINDUSEREX :Not enough parameters");
    return 0;;
}

static int _handle_page_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];
    bool paged = false;

	std::memset(_temp,0,sizeof(_temp));

	if ((numparams>=1)&&(params[0])&&(text)) {
	    t_connection * user;

	    if (std::strcmp(params[0], "0") == 0) {
            /* PAGE for MY BATTLECLAN */
            t_clan * clan = account_get_clan(conn_get_account(conn));

            if ((clan) && (clan_send_message_to_online_members(clan,message_type_page,conn,text) >= 1))
                paged = true;
        }
	    else if((user = connlist_find_connection_by_accountname(params[0]))&&(conn_wol_get_pageme(user) == 33)) {
     		message_send_text(user,message_type_page,conn,text);
            paged = true;
	    }

        if (paged)
            snprintf(_temp, sizeof(_temp), "0 :"); /* Page was succesfull */
        else
            snprintf(_temp, sizeof(_temp), "1 :"); /* User not loged in or have not allowed page */

	    irc_send(conn,RPL_PAGE,_temp);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"PAGE :Not enough parameters");
	return 0;
}

static int _handle_startg_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	char _temp_a[MAX_IRC_MESSAGE_LEN];
	t_channel * channel;

	std::time_t now;

 	/**
    *  Imput expected:
    *   STARTG [channel_name] [nick1](,nick2_optional)
    *
 	*  Heres the output expected (this can have up-to 8 entries (ie 8 players):
    *  (we are assuming for this example that user1 is the game owner)
    *
 	*   WOLv1:
    *   :user1!WWOL@hostname STARTG u :owner_ip gameNumber time_t
    *
    *   WOLv2:
 	*   :user1!WWOL@hostname STARTG u :user1 xxx.xxx.xxx.xxx user2 xxx.xxx.xxx.xxx :gameNumber time_t
 	*/

	if ((numparams>=2)&&(params[1])) {
	    int i;
	    char ** e;

	    std::memset(temp,0,sizeof(temp));
	    std::memset(_temp_a,0,sizeof(_temp_a));

	    e = irc_get_listelems(params[1]);
	    /* FIXME: support wildcards! */

        std::strcat(temp,":");

 	    if (conn_get_clienttag(conn) == CLIENTTAG_WCHAT_UINT) {
  		    t_connection * user;
   		    channel = conn_get_channel(conn);

   		    for (user = channel_get_first(channel);user;user = channel_get_next()) {
	           char const * name = conn_get_chatname(user);
	           if (std::strcmp(conn_get_chatname(conn),name) != 0) {
	              snprintf(temp, sizeof(temp), "%s ", addr_num_to_ip_str(channel_wol_get_game_ownerip(channel)));
               }
            }
        }
        else {
            for (i=0;((e)&&(e[i]));i++) {
   		        t_connection * user;
       		    const char * addr = NULL;

   	     	    if((user = connlist_find_connection_by_accountname(e[i]))) {
                    addr = addr_num_to_ip_str(conn_get_addr(user));
	            }
   		        snprintf(_temp_a, sizeof(_temp_a), "%s %s ", e[i], addr);
   		        std::strcat(temp,_temp_a);
            }
        }

        if (conn_get_clienttag(conn) != CLIENTTAG_WCHAT_UINT)
            std::strcat(temp,":");

        std::strcat(temp,"1337"); /* yes, ha ha funny, i just don't generate game numbers yet */
        std::strcat(temp," ");

        now = std::time(NULL);
        snprintf(_temp_a, sizeof(_temp_a), "%lu", now);
        std::strcat(temp,_temp_a);

        for (i=0;((e)&&(e[i]));i++) {
   		        t_connection * user;
   		        if((user = connlist_find_connection_by_accountname(e[i]))) {
                    message_send_text(user,message_wol_start_game,conn,temp);
                }
        }

	    if (e)
            irc_unget_listelems(e);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"STARTG :Not enough parameters");
   	return 0;
}

static int _handle_advertr_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];

    std::memset(temp,0,sizeof(temp));

    /**
    *  Heres the imput expected
    *  ADVERTR [channel]
    *
    *  Heres the output expected
    *  :[servername] ADVERTR 5 [channel]
    */

    if ((numparams>=1)&&(params[0])) {
        snprintf(temp,sizeof(temp),"5 %s",params[0]);
        message_send_text(conn,message_wol_advertr,conn,temp);
    }
    else
   	    irc_send(conn,ERR_NEEDMOREPARAMS,"ADVERTR :Not enough parameters");	
    return 0;
}

static int _handle_advertc_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /* FIXME: Not implemented yet */
    return 0;
}

static int _handle_chanchk_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_channel * channel;

   	std::memset(temp,0,sizeof(temp));

    /**
    *  Heres the imput expected
    *  chanchk [channel]
    *
    *  Heres the output expected
    *  :[servername] CHANCHK [channel]
    */

    if ((numparams>=1)&&(params[0])) {
        if ((channel = channellist_find_channel_by_name(irc_convert_ircname(params[0]),NULL,NULL))) {
            std::strcat(temp,params[0]);
            message_send_text(conn,message_wol_chanchk,conn,temp);
        }
        else {
            snprintf(temp,sizeof(temp),"%s :No such channel", params[0]);
            irc_send(conn,ERR_NOSUCHCHANNEL,temp);
        }
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"CHANCHK :Not enough parameters");
	return 0;
}

static int _handle_getbuddy_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
	char _temp[MAX_IRC_MESSAGE_LEN];
	char const * friend_name;
	t_account * my_acc;
	t_account * friend_acc;
	t_list * flist;
	t_friend * fr;
	int num;
	unsigned int uid;
    int i;

	std::memset(temp,0,sizeof(temp));
	std::memset(_temp,0,sizeof(_temp));

    /**
 	*  Heres the output expected
 	*  :[servername] 333 [user] [buddy_name1]`[buddy_name2]`
 	*
 	*  Without names:
    *  :[servername] 333 [user]
 	*/

    my_acc = conn_get_account(conn);
	num = account_get_friendcount(my_acc);

	flist=account_get_friends(my_acc);

	if(flist!=NULL) {
        for (i=0; i<num; i++) {
    		if ((!(uid = account_get_friend(my_acc,i))) || (!(fr = friendlist_find_uid(flist,uid)))) {
        	    eventlog(eventlog_level_error,__FUNCTION__,"friend uid in list");
        	    continue;
    		}
    		friend_acc = friend_get_account(fr);
    		friend_name = account_get_name(friend_acc);
		    snprintf(_temp, sizeof(_temp), "%s`", friend_name);
   		    std::strcat(temp,_temp);
        }
    }
	irc_send(conn,RPL_GET_BUDDY,temp);
	return 0;
}

static int _handle_addbuddy_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_account * my_acc;
    t_account * friend_acc;

    std::memset(temp,0,sizeof(temp));

    /**
    *  Heres the imput expected
    *  ADDBUDDY [buddy_name]
    *
    *  Heres the output expected
    *  :[servername] 334 [user] [buddy_name]
    */

    if ((numparams>=1)&&(params[0])) {
        my_acc = conn_get_account(conn);
        if (friend_acc = accountlist_find_account(params[0])) {
            account_add_friend(my_acc, friend_acc);
            /* FIXME: Check if add friend is done if not then send right message */

            snprintf(temp,sizeof(temp),"%s", params[0]);
            irc_send(conn,RPL_ADD_BUDDY,temp);
        }
        else {
            snprintf(temp,sizeof(temp),"%s :No such nick", params[0]);
            irc_send(conn,ERR_NOSUCHNICK,temp);
            /* NOTE: this is not dumped from WOL, this not shows message
              but in Emperor doesnt gives name to list, in RA2 have no efect */
        }
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"ADDBUDDY :Not enough parameters");
    return 0;
}

static int _handle_delbuddy_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    char const * friend_name;
    t_account * my_acc;
    int num;

    std::memset(temp,0,sizeof(temp));

    /**
    *  Heres the imput expected
    *  DELBUDDY [buddy_name]
    *
    *  Heres the output expected
    *  :[servername] 335 [user] [buddy_name]
    */

    if ((numparams>=1)&&(params[0])) {
        friend_name = params[0];
        my_acc = conn_get_account(conn);

        num = account_remove_friend2(my_acc, friend_name);

        /**
        *  FIXME: Check if remove friend is done if not then send right message
        *  Btw I dont know another then RPL_DEL_BUDDY message yet.
        */

        snprintf(temp,sizeof(temp),"%s", friend_name);
        irc_send(conn,RPL_DEL_BUDDY,temp);
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"DELBUDDY :Not enough parameters");
	return 0;
}

static int _handle_host_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_connection * user;

    std::memset(temp,0,sizeof(temp));

    if ((numparams>=1)&&(params[0])) {
        if ((user = connlist_find_connection_by_accountname(params[0]))) {
            snprintf(temp,sizeof(temp),": %s", text);
            message_send_text(user,message_type_host,conn,temp);
        }
        else {
            snprintf(temp,sizeof(temp),"%s :No such nick", params[0]);
            irc_send(conn,ERR_NOSUCHNICK,temp);
        }
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"HOST :Not enough parameters");
    return 0;
}

static int _handle_invmsg_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    char ** e;
    t_connection * user;
    int i;

    /**
     *  Here is the imput expected:
     *  INVMSG [channel] [unknown] [invited],[invited2_optional]
     *  [unknown] can be 1 or 2
     *
     *  Here is the output expected:
     *  :user!WWOL@hostname INVMSG [invited] [channel] [unknown]
     */

    if ((numparams>=3)&&(params[0])&&(params[1])&&(params[2])) {
    	std::memset(temp,0,sizeof(temp));
        e = irc_get_listelems(params[2]);

        for (i=0;((e)&&(e[i]));i++) {
            if ((user = connlist_find_connection_by_accountname(e[i]))) {
                snprintf(temp,sizeof(temp),"%s %s", params[0], params[1]);
                /* FIXME: set user to linvitelist! */
                message_send_text(user,message_type_invmsg,conn,temp);
            }
        }
    }
    else {
        irc_send(conn,ERR_NEEDMOREPARAMS,"INVMSG :Not enough parameters");
    }
    return 0;
}

static int _handle_invdel_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /* FIXME: Not implemented yet */
    return 0;
}

static int _handle_userip_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_connection * user;
    const char * addr = NULL;

   	std::memset(temp,0,sizeof(temp));

    if ((numparams>=1)&&(params[0])) {
        if((user = connlist_find_connection_by_accountname(params[0]))) {
            addr = addr_num_to_ip_str(conn_get_addr(user));
            snprintf(temp,sizeof(temp),"%s",addr);
            message_send_text(conn,message_wol_userip,conn,temp);
        }
        else {
            snprintf(temp,sizeof(temp),"%s :No such nick", params[0]);
            irc_send(conn,ERR_NOSUCHNICK,temp);
        }
    }
    else
        irc_send(conn,ERR_NEEDMOREPARAMS,"USERIP :Not enough parameters");
    return 0;
}

/**
 * LADDER Server commands:
 */
static int _handle_listsearch_command(t_connection * conn, int numparams, char ** params, char * text)
{
	// FIXME: Not implemetned yet
	conn_set_state(conn, conn_state_destroy);
	return 0;
}

static int _handle_rungsearch_command(t_connection * conn, int numparams, char ** params, char * text)
{
	// FIXME: Not implemetned yet
	conn_set_state(conn, conn_state_destroy);
	return 0;
}

static int _handle_highscore_command(t_connection * conn, int numparams, char ** params, char * text)
{
	// FIXME: Not implemetned yet
	conn_set_state(conn, conn_state_destroy);
	return 0;
}

}

}
