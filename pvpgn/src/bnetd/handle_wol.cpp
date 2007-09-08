/*
 * Copyright (C) 2001  Marco Ziech (mmz@gmx.net)
 * Copyright (C) 2005  Bryan Biedenkapp (gatekeep@gmail.com)
 * Copyright (C) 2006,2007  Pelish (pelish@gmail.com)
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
static int _handle_ping_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_pong_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_pass_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_privmsg_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_quit_command(t_connection * conn, int numparams, char ** params, char * text);

static int _handle_list_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_join_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_part_command(t_connection * conn, int numparams, char ** params, char * text);

static int _handle_cvers_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_verchk_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_apgar_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_serial_command(t_connection * conn, int numparams, char ** params, char * text);

static int _handle_squadinfo_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_setopt_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_setcodepage_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_setlocale_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_getcodepage_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_getlocale_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_getinsider_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_joingame_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_gameopt_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_finduser_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_finduserex_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_page_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_startg_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_advertr_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_chanchk_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_getbuddy_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_addbuddy_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_delbuddy_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_time_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_kick_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_mode_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_host_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_advertc_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_clanbyname_command(t_connection * conn, int numparams, char ** params, char * text);
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
	{ "SETOPT"		, _handle_setopt_command },
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
    { "CHANCHK"	    , _handle_chanchk_command },
    { "GETBUDDY"    , _handle_getbuddy_command },
    { "ADDBUDDY"    , _handle_addbuddy_command },
    { "DELBUDDY"    , _handle_delbuddy_command },
    { "TIME"        , _handle_time_command },
	{ "KICK"		, _handle_kick_command },
	{ "MODE"		, _handle_mode_command },
	{ "HOST"		, _handle_host_command },
	{ "ADVERTC"		, _handle_advertc_command },
	{ "CLANBYNAME"	, _handle_clanbyname_command },
	{ "USERIP"	    , _handle_userip_command },

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

extern int handle_wol_welcome(t_connection * conn)
{
    /* This function need rewrite */

    irc_send_motd(conn);

    conn_set_state(conn,conn_state_bot_password);
    if (connlist_find_connection_by_accountname(conn_get_loggeduser(conn))) {
        message_send_text(conn,message_type_notice,NULL,"This account is already logged in, use another account.");
	return -1;
    }

    if (conn_get_ircpass(conn)) {
    /* FIXME: In wol is not authentification by PASS but by APGAR!!! */
	// irc_send_cmd(conn,"NOTICE",":Trying to authenticate with PASS ...");
	irc_authenticate(conn,conn_get_ircpass(conn));
    } else {
    	    message_send_text(conn,message_type_notice,NULL,"No PASS command received. Please identify yourself by /msg NICKSERV identify <password>.");
    }

    return 0;
}

static int _handle_user_command(t_connection * conn, int numparams, char ** params, char * text)
{   /* In WOL isnt used user command (only by byckwar compatibility) */
	/* RFC 2812 says: */
	/* <user> <mode> <unused> :<realname>*/
	/* wolII and X-Chat say: */
	/* mz SHODAN localhost :Marco Ziech */
	/* BitchX says: */
	/* mz +iws mz :Marco Ziech */
	/* Don't bother with, params 1 and 2 anymore they don't contain what they should. */
	char * user = NULL;
	char * realname = NULL;
	t_account * a;

	if ((numparams>=3)&&(params[0])&&(text)) {
	    user = params[0];
	    realname = text;

		if (conn_get_wol(conn) == 1) {
			user = (char *)conn_get_loggeduser(conn);
			realname = (char *)conn_get_loggeduser(conn);

        	if (conn_get_user(conn)) {
		irc_send(conn,ERR_ALREADYREGISTRED,":You are already registred");
        }
			else {
				eventlog(eventlog_level_debug,__FUNCTION__,"[%d][** WOL **] got USER: user=\"%s\"",conn_get_socket(conn),user);

                a = accountlist_find_account(user);
                if (!a) {
                   if((conn_get_wol(conn) == 1)) {
                        t_account * tempacct;
                        t_hash pass_hash;
                        const char * pass = "supersecret";

				/* no need to std::tolower a known contant lowercase string
            			for (unsigned j=0; j<std::strlen(pass); j++)
            				if (std::isupper((int)pass[j])) pass[j] = std::tolower((int)pass[j]);
				*/

            			bnet_hash(&pass_hash,std::strlen(pass),pass);

            			tempacct = accountlist_create_account(user,hash_get_str(pass_hash));
            			if (!tempacct) {
                            return 0;
            			}
                   }
                }

			conn_set_user(conn,user);
			conn_set_owner(conn,realname);
			if (conn_get_loggeduser(conn))
				handle_wol_welcome(conn); /* only send the welcome if we have USER and NICK */
	    	}
    	}
		else {
			if (conn_get_user(conn)) {
				irc_send(conn,ERR_ALREADYREGISTRED,":You are already registred");
			}
			else {
				eventlog(eventlog_level_debug,__FUNCTION__,"[%d] got USER: user=\"%s\" realname=\"%s\"",conn_get_socket(conn),user,realname);
				conn_set_user(conn,user);
				conn_set_owner(conn,realname);
				if (conn_get_loggeduser(conn))
					handle_wol_welcome(conn); /* only send the welcome if we have USER and NICK */
    		}
		}
   	}
	else {
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to USER");
    	}
	return 0;
}

static int _handle_ping_command(t_connection * conn, int numparams, char ** params, char * text)
{
	if (conn_get_clienttag(conn) == CLIENTTAG_WCHAT_UINT) {
       /* WCHAT need ping */
	   if (numparams)
	       irc_send_pong(conn,params[0]);
       else
	       irc_send_pong(conn,text);
       return 0;
    }
    return 0;
}

static int _handle_pong_command(t_connection * conn, int numparams, char ** params, char * text)
{
	/* NOTE: RFC2812 doesn't seem to be very expressive about this ... */
	if (conn_get_clienttag(conn) == CLIENTTAG_WCHAT_UINT) {
	    if (conn_get_ircping(conn)==0) {
            eventlog(eventlog_level_warn,__FUNCTION__,"[%d] PONG without PING",conn_get_socket(conn));
        }
	    else {
	         unsigned int val = 0;
	         char * sname;

	         if (numparams>=1) {
	             val =  std::strtoul(params[0],NULL,10);
		         sname = params[0];
             }
	         else if (text) {
	    	     val = std::strtoul(text,NULL,10);
	             sname = text;
             }
	         else {
		        val = 0;
		        sname = 0;
             }

	         if (conn_get_ircping(conn) != val) {
                 if ((!(sname)) || (std::strcmp(sname,server_get_hostname())!=0)) {
			        /* Actually the servername should not be always accepted but we aren't that pedantic :) */
			        eventlog(eventlog_level_warn,__FUNCTION__,"[%d] got bad PONG (%u!=%u && %s!=%s)",conn_get_socket(conn),val,conn_get_ircping(conn),sname,server_get_hostname());
			        return -1;
		         }
             }
	         conn_set_latency(conn,get_ticks()-conn_get_ircping(conn));
	         eventlog(eventlog_level_debug,__FUNCTION__,"[%d] latency is now %d (%u-%u)",conn_get_socket(conn),get_ticks()-conn_get_ircping(conn),get_ticks(),conn_get_ircping(conn));
	         conn_set_ircping(conn,0);
        }
	    return 0;
    }
    return 0;
}

static int _handle_pass_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /**
     * PASS isnt used in WOL (only for backwrd compatibility client sent PASS supersecret
     * real password sent client by apgar command
     */

	if ((!conn_get_ircpass(conn))&&(conn_get_state(conn)==conn_state_bot_username)) {
		t_hash h;

	    if (numparams>=1) {
			bnet_hash(&h,std::strlen(params[0]),params[0]);
			conn_set_ircpass(conn,hash_get_str(h));
	    }
		else
			irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to PASS");
    }
	else {
	    eventlog(eventlog_level_warn,__FUNCTION__,"[%d] client tried to set password twice with PASS",conn_get_socket(conn));
    }
	return 0;
}

static int _handle_privmsg_command(t_connection * conn, int numparams, char ** params, char * text)
{
     /**
      * Pelish: FIXME delete NICSERV and add support for matchbot.
      * ACTION messages is not in WOLv1 and Dune 2000 sended by server (client add this messages automaticaly)
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
						irc_send(conn,ERR_NOSUCHCHANNEL,":No such channel");
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
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to PRIVMSG");
	return 0;
}

static int _handle_list_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];

	irc_send(conn,RPL_LISTSTART,"Channel :Users Names"); /* backward compatibility */
    t_elem const * curr;

    if (numparams == 0) {
        /* This is Westwood Chat LIST command */
        eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] LIST WCHAT");

        LIST_TRAVERSE_CONST(channellist(),curr) {
   	         t_channel const * channel = (const t_channel*)elem_get_data(curr);
             char const * tempname;

             tempname = irc_convert_channel(channel);

             if (std::strstr(tempname,"_game") == NULL) {
                sprintf(temp,"%s %u ",tempname,channel_get_length(channel));

                if ((channel_get_clienttag(channel) != NULL) &&
                   (std::strcmp(channel_get_clienttag(channel), clienttag_uint_to_str(CLIENTTAG_WCHAT_UINT)) == 0 ))
                    std::strcat(temp,"1");  /* WestwoodChat Icon before chanelname */
                else
                    std::strcat(temp,"0");  /* No WestwoodChat Icon before chanelname */

                std::strcat(temp,":");
                irc_send(conn,RPL_CHANNEL,temp);
             }
        }
    }
    else if ((std::strcmp(params[0], "0") == 0) || (std::strcmp(params[0], "-1") == 0)) {
        /* Nox and Emperor send by some reason -1 */
		/* HACK: Currently, this is the best way to set the game type... */
		conn_wol_set_game_type(conn,std::atoi(params[1]));

		eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] LIST [Channel]");
   	    LIST_TRAVERSE_CONST(channellist(),curr)
		{
    		t_channel const * channel = (t_channel const *)elem_get_data(curr);
       	    char const * tempname;

	        tempname = irc_convert_channel(channel);

            if ((channel_get_clienttag(channel) != NULL) &&
                (std::strcmp(channel_get_clienttag(channel), clienttag_uint_to_str(conn_get_clienttag(conn))) == 0)) {
			    if((std::strstr(tempname,"Lob") != NULL) || (std::strstr(tempname,"Emperor") != NULL)) {
                     eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] LIST [Channel: \"Lob\"] (%s)",tempname);
				     if (std::strlen(tempname)+1+20+1+1<MAX_IRC_MESSAGE_LEN)
					     snprintf(temp, sizeof(temp), "%s %u 0 388:",tempname,channel_get_length(channel));
 				     else
   					     eventlog(eventlog_level_warn,__FUNCTION__,"LISTREPLY length exceeded");
                     irc_send(conn,RPL_CHANNEL,temp);
			    }
            }
   		}
    }
    /**
    *  Known channel game types:
    *  0 = Westwood Chat channels, 1 = Command & Conquer Win95 channels, 2 = Red Alert Win95 channels,
    *  3 = Red Alert Counterstrike channels, 4 = Red Alert Aftermath channels, 5 = CnC Sole Survivor channels,
    *  12 = C&C Renegade channels, 14 = Dune 2000 channels, 16 = Nox channels, 18 = Tiberian Sun channels,
    *  21 = Red Alert 1 v 3.03 channels, 31 = Emperor: Battle for Dune, 33 = Red Alert 2,
    *  37 = Nox Quest channels, 39 = C&C Renegade Quickgame channels, 41 = Yuri's Revenge
	*/
    if ((numparams == 0) ||
       (std::strcmp(params[0], "12") == 0) ||
       (std::strcmp(params[0], "14") == 0) ||
       (std::strcmp(params[0], "16") == 0) ||
       (std::strcmp(params[0], "18") == 0) ||
       (std::strcmp(params[0], "21") == 0) ||
       (std::strcmp(params[0], "31") == 0) ||
       (std::strcmp(params[0], "33") == 0) ||
       (std::strcmp(params[0], "37") == 0) ||
       (std::strcmp(params[0], "41") == 0)) {
   		    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] LIST [Game]");
       	    LIST_TRAVERSE_CONST(channellist(),curr)
            {
    		    t_channel const * channel = (const t_channel*)elem_get_data(curr);
			    t_connection * m;
        	    char const * tempname;
				char * topic = channel_get_topic(channel_get_name(channel));

        	    tempname = irc_convert_channel(channel);
        	    
				if((channel_wol_get_game_type(channel) != 0)) {
					m = channel_get_first(channel);
					if((channel_wol_get_game_type(channel) == conn_wol_get_game_type(conn)) || ((numparams == 0))) {
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
							*   #game_channel_name users unknown gameType gameIsTournment gameExtension longIP 128::topic
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

    	irc_send(conn,RPL_LISTEND," :End of LIST command");
    	return 0;
}

static int _handle_join_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /* NOTICE: Move this function to IRC Common file!!! */

	if (numparams>=1) {
	    char ** e;

	    e = irc_get_listelems(params[0]);
	    if ((e)&&(e[0])) {
	    		char const * wolname = irc_convert_ircname(e[0]);
	   	 	t_channel * old_channel = conn_get_channel(conn);

			if ((!(wolname)) || (conn_set_channel(conn,wolname)<0)) {
				irc_send(conn,ERR_NOSUCHCHANNEL,":JOIN failed"); /* FIXME: be more precise; what is the real error code for that? */
			}
			else {
				channel_set_userflags(conn);
			}
		}
    		if (e)
			irc_unget_listelems(e);
	}
	else
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to JOIN");
	return 0;
}

static int _handle_quit_command(t_connection * conn, int numparams, char ** params, char * text)
{
    irc_send(conn,RPL_QUIT,":goodbye");
    conn_set_channel(conn, NULL);
    conn_set_state(conn, conn_state_destroy);

    return 0;
}

static int _handle_part_command(t_connection * conn, int numparams, char ** params, char * text)
{
    if ((conn_wol_get_ingame(conn) == 1)) {
        conn_wol_set_ingame(conn,0);
    }
    conn_set_channel(conn, NULL);
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
 	    return 0;
    }

    return -1;
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
    *  :[servername] 379 [username] :none none none 1 [SKU] NONREQ
    */

    clienttag = tag_sku_to_uint(std::atoi(params[0]));
    if (clienttag != CLIENTTAG_WWOL_UINT)
        conn_set_clienttag(conn,clienttag);

    snprintf(temp, sizeof(temp), ":none none none 1 %s NONREQ", params[0]);
    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] VERCHK %s",temp);
    irc_send(conn,RPL_VERCHK_NONREQ,temp);

   	return 0;
}

static int _handle_apgar_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char * apgar = NULL;

	if((numparams>=1)&&params[0]) {
	    apgar = params[0];
	    conn_wol_set_apgar(conn,apgar);
	}

	return 0;
}

static int _handle_serial_command(t_connection * conn, int numparams, char ** params, char * text)
{
    // Ignore command
	return 0;
}

static int _handle_squadinfo_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];
	t_clan * clan;
	unsigned int clanid;
	const char * clantag;
	const char * clanname;

	std::memset(_temp,0,sizeof(_temp));

    if (params[0]) {
       if (std::strcmp(params[0], "0") == 0) {
           /* 0 == question for me! */
           clan = account_get_clan(conn_get_account(conn));
       }
       else {
           /* question for another one */
           clan = clanlist_find_clan_by_clanid(std::atoi(params[0]));
       }
    }

    if (clan) {
        clanid = clan_get_clanid(clan);
	    clantag = clantag_to_str(clan_get_clantag(clan));
	    clanname = clan_get_name(clan);
        snprintf(_temp, sizeof(_temp), "%u`%s`%s`0`0`1`0`0`0`0`0`0`0`x`x`x",clanid,clanname,clantag);
        irc_send(conn,RPL_BATTLECLAN,_temp);
    }
    else {
        snprintf(_temp, sizeof(_temp), "ID does not exist");
	    irc_send(conn,ERR_IDNOEXIST,_temp);
    }
    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] SQUADINFO: %s",_temp);

	return 0;
}

static int _handle_setopt_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /**
    *   This is option for enabling/disabling Page and Find user.
    *
    *   Heres the input expected:
    *   SETOPT 17,32
    *
    *   First parameter: 16 == FindDisabled 17 == FindEnabled
    *   Second parameter: 32 == PageDisabled 33 == PageEnabled
    */

    char ** elems;

    elems = irc_get_listelems(params[0]);

    if ((elems)&&(elems[0])&&(elems[1])) {
        conn_wol_set_findme(conn,std::atoi(elems[0]));
        conn_wol_set_pageme(conn,std::atoi(elems[1]));
    }

	if (elems)
	     irc_unget_listelems(elems);

	return 0;
}

static int _handle_setcodepage_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char * codepage = NULL;

	if((numparams>=1)&&params[0]) {
	    codepage = params[0];
	    conn_wol_set_codepage(conn,std::atoi(codepage));
	}
	irc_send(conn,RPL_SET_CODEPAGE,codepage);
	return 0;
}

static int _handle_setlocale_command(t_connection * conn, int numparams, char ** params, char * text)
{
	t_account * account = conn_get_account(conn);
	int locale;

	if((numparams>=1)&&params[0]) {
       locale = std::atoi(params[0]);
       account_set_locale(account,locale);
	}
	irc_send(conn,RPL_SET_LOCALE,params[0]);
	return 0;
}

static int _handle_getcodepage_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	char _temp[MAX_IRC_MESSAGE_LEN];

	std::memset(temp,0,sizeof(temp));
	std::memset(_temp,0,sizeof(_temp));

	if((numparams>=1)) {
	    int i;
	    for (i=0; i<numparams; i++) {
    		t_connection * user;
            int codepage;
    		char const * name;

    		if((user = connlist_find_connection_by_accountname(params[i]))) {
    		    codepage = conn_wol_get_codepage(user);
    		    name = conn_get_chatname(user);

    		    snprintf(_temp, sizeof(_temp), "%s`%u", name, codepage);
    		    std::strcat(temp,_temp);
    		    if(i < numparams-1)
    			     std::strcat(temp,"`");
    		}
	    }
   	    irc_send(conn,RPL_GET_CODEPAGE,temp);
	}
	return 0;
}

static int _handle_getlocale_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	char _temp[MAX_IRC_MESSAGE_LEN];

	std::memset(temp,0,sizeof(temp));
	std::memset(_temp,0,sizeof(_temp));

	if((numparams>=1)) {
	    int i;
	    for (i=0; i<numparams; i++) {
    		t_account * account;
    		int locale;
    		char const * name;

    		if(account = accountlist_find_account(params[i])) {
    		    locale = account_get_locale(account);
    		    if (!locale)
    		       locale = 0;
    		    snprintf(_temp, sizeof(_temp), "%s`%u", params[i], locale);
    		    std::strcat(temp,_temp);
    		    if(i < numparams-1)
           			std::strcat(temp,"`");
    		}
	    }
	    DEBUG1("[** WOL **] GETLOCALE %s",temp);
	    irc_send(conn,RPL_GET_LOCALE,temp);
	}
	return 0;
}

static int _handle_getinsider_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /**
     * FIXME:
     * XWIS working:
     *   GETINSIDER Pelish
     *   : 399 u Pelish`0
     * FSGS working: (used in PvPGN now)
     *   GETINSIDER Pelish
     *    :irc.westwood.com 399 Pelish
     */
    
    if (params[0]) {
       eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] GETINSIDER %s",params[0]);
       irc_send(conn,RPL_GET_INSIDER,params[0]);
       return 0;
    }
    return -1;
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
	*   Here is the input expected for WCHAT:
    *   JOINGAME [#Game_channel] 2 [NumberOfPlayers] [gameType] 1 1 [gameIsTournament]
    *   Knowed GameTypes (0-chat, 1-cnc, 2-ra1, 3-racs, 4-raam, 5-solsurv)
    *
	*   Here is the input expected:
	*   JOINGAME #user's_game MinOfPlayers MaxOfPlayers gameType unknown unknown gameIsTournament gameExtension password
	*
	*   Heres the output expected:
	*   user!WWOL@hostname JOINGAME MinOfPlayers MaxOfPlayers gameType unknown clanID longIP gameIsTournament :#game_channel_name
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
	             irc_send(conn,ERR_NOSUCHCHANNEL,":No such channel");
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

			conn_wol_set_ingame(conn,1);

			if (old_channel)
   	  		   old_channel_name = xstrdup(irc_convert_channel(old_channel));

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
                                    channel_wol_get_game_tournament(channel), irc_convert_channel(channel));
                    }
                    else {
                        /* WOLv2 JOINGAME message with BATTLECLAN support */
                        t_clan * clan = account_get_clan(conn_get_account(conn));
                        unsigned int clanid = 0;

                        if (clan)
                            clanid = clan_get_clanid(clan);

                        std::sprintf(_temp,"1 %u %u 1 %u %u %u :%s", channel_get_length(channel), channel_wol_get_game_type(channel),
                                     clanid, conn_get_addr(conn), channel_wol_get_game_tournament(channel), irc_convert_channel(channel));
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
			  old_channel_name = xstrdup(irc_convert_channel(old_channel));

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
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to JOINGAME");
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
 	if (conn_get_clienttag(conn) == CLIENTTAG_WCHAT_UINT) {
  		    /* we only send text to another client */
   		    t_connection * user;
   		    t_channel * channel;

   		    channel = conn_get_channel(conn);

   		    for (user = channel_get_first(channel);user;user = channel_get_next()) {
	           char const * name = conn_get_chatname(user);
	           if (std::strcmp(conn_get_chatname(conn),name) != 0) {
	              snprintf(temp, sizeof(temp), ":%s", text);
	              message_send_text(user,message_wol_gameopt_join,conn,temp);
               }
   		    }

       return 0;
    }
	if ((numparams>=1)&&(text)) {
	    int i;
	    char ** e;

	    e = irc_get_listelems(params[0]);
	    /* FIXME: support wildcards! */

	    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] GAMEOPT: (%s :%s)",params[0],text);
	    conn_wol_set_game_options(conn,text);

	    for (i=0;((e)&&(e[i]));i++) {
    		if (e[i][0]=='#') {
    		    /* game owner change */
    		    t_channel * channel;

    		    if ((channel = channellist_find_channel_by_name(irc_convert_ircname(params[0]),NULL,NULL))) {
        			snprintf(temp, sizeof(temp), ":%s", text);
        			channel_message_send(channel,message_wol_gameopt_owner,conn,temp);
    		    }
    		    else {
        			irc_send(conn,ERR_NOSUCHCHANNEL,":No such channel");
    		    }
    		}
    		else
    		{
    		    /* user change */
    		    t_connection * user;

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
	else
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to GAMEOPT");
	return 0;
}

static int _handle_finduser_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];
	char const * wolname = NULL;

	std::memset(_temp,0,sizeof(_temp));

	if ((numparams>=1)) {
	    t_connection * user;

	    if((user = connlist_find_connection_by_accountname(params[0]))&&(conn_wol_get_findme(user) == 17)) {
     		wolname = irc_convert_channel(conn_get_channel(user));
	        snprintf(_temp, sizeof(_temp), "0 :%s", wolname); /* User found in channel wolname */
	    }
	    else
	        snprintf(_temp, sizeof(_temp), "1 :"); /* user not loged or have not allowed find */

	    irc_send(conn,RPL_FIND_USER,_temp);
    }
	return 0;
}

static int _handle_finduserex_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];
	char const * wolname = NULL;

	std::memset(_temp,0,sizeof(_temp));

	if ((numparams>=1)) {
	    t_connection * user;

	    if((user = connlist_find_connection_by_accountname(params[0]))&&(conn_wol_get_findme(user) == 17)) {
     		wolname = irc_convert_channel(conn_get_channel(user));
     		snprintf(_temp, sizeof(_temp), "0 :%s,0", wolname); /* User found in channel wolname */
	    }
	    else
	        snprintf(_temp, sizeof(_temp), "1 :"); /* user not loged or have not allowed find */

	    irc_send(conn,RPL_FIND_USER_EX,_temp);
    }
	return 0;
}

static int _handle_page_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char _temp[MAX_IRC_MESSAGE_LEN];

	std::memset(_temp,0,sizeof(_temp));

	if ((numparams>=1)&&(text)) {
	    t_connection * user;

	    if (std::strcmp(params[0], "0") == 0) {
            /* PAGE for MY BATTLECLAN */
            t_clan * clan = account_get_clan(conn_get_account(conn));
            t_list * cl_member_list;
            t_elem * curr;
            if (clan) {
               cl_member_list = clan_get_members(clan);

               LIST_TRAVERSE(cl_member_list,curr) {
                   t_clanmember * member = (t_clanmember*)elem_get_data(curr);

                   if ((user = clanmember_get_conn(member))&&(conn_wol_get_pageme(user) == 33));
                       message_send_text(user,message_type_page,conn,text);
               }
            }
            else
                ERROR1("User %s want to ClanPAGE but is not clanmember!",conn_get_chatname(conn));
            return 0;
        }
	    else if((user = connlist_find_connection_by_accountname(params[0]))&&(conn_wol_get_pageme(user) == 33)) {
     		message_send_text(user,message_type_page,conn,text);
     		snprintf(_temp, sizeof(_temp), "0 :"); /* Page was succesfull */
	    }
	    else
	        snprintf(_temp, sizeof(_temp), "1 :"); /* User not loged in or have not allowed page */
	    irc_send(conn,RPL_PAGE,_temp);
	}
	return 0;
}

static int _handle_startg_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	char _temp_a[MAX_IRC_MESSAGE_LEN];
	t_channel * channel;
	int numelems = 0;

	std::time_t now;

 	/**
 	*  Heres the output expected (this can have up-to 8 entries (ie 8 players):
    *  (we are assuming for this example that user1 is the game owner)
    *
 	*   WOLv1:
    *   :user1!WWOL@hostname STARTG u :owner_ip :gameNumber time_t
    *
    *   WOLv2:
 	*   user1!WWOL@hostname STARTG u :user1 xxx.xxx.xxx.xxx user2 xxx.xxx.xxx.xxx :gameNumber time_t
 	*/

	if((numparams>=1)) {
	    int i;
	    char ** e;

	    std::memset(temp,0,sizeof(temp));
	    std::memset(_temp_a,0,sizeof(_temp_a));

	    e = irc_get_listelems(params[1]);
	    /* FIXME: support wildcards! */

        if (e) {
           for (numelems=0;e[numelems];numelems++);
        }

        std::strcat(temp,":");

 	    if (numelems == 1) {
  		    t_connection * user;
   		    channel = conn_get_channel(conn);

   		    for (user = channel_get_first(channel);user;user = channel_get_next()) {
	           char const * name = conn_get_chatname(user);
	           if (std::strcmp(conn_get_chatname(conn),name) != 0) {
	              snprintf(temp, sizeof(temp), "%s ", addr_num_to_ip_str(channel_wol_get_game_ownerip(channel)));
               }
            }
        }
        else if (numelems>=2) {
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

        std::strcat(temp,":");
        std::strcat(temp,"1337"); /* yes, ha ha funny, i just don't generate game numbers yet */
        std::strcat(temp," ");

        now = std::time(NULL);
        snprintf(_temp_a, sizeof(_temp_a), "%lu", now);
        std::strcat(temp,_temp_a);

	    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] STARTG: (%s)",temp);

        for (i=0;((e)&&(e[i]));i++) {
   		        t_connection * user;
   		        if((user = connlist_find_connection_by_accountname(e[i]))) {
                    message_send_text(user,message_wol_start_game,conn,temp);
                }
        }

	    if (e)
            irc_unget_listelems(e);
	}
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

    // Ignore command but, return 5

    snprintf(temp,sizeof(temp),"5 ");
 	std::strcat(temp,params[0]);

	message_send_text(conn,message_wol_advertr,conn,temp);

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

	if ((channel = channellist_find_channel_by_name(irc_convert_ircname(params[0]),NULL,NULL))) {
     	std::strcat(temp,params[0]);
     	message_send_text(conn,message_wol_chanchk,conn,temp);
	}
    else {
     	irc_send(conn,ERR_NOSUCHCHANNEL,":No such channel");
	}
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
    		if ((!(uid = account_get_friend(my_acc,i))) || (!(fr = friendlist_find_uid(flist,uid))))
    		{
        	    eventlog(eventlog_level_error,__FUNCTION__,"friend uid in list");
        	    continue;
    		}
    		friend_acc = friend_get_account(fr);
    		friend_name = account_get_name(friend_acc);
		    snprintf(_temp, sizeof(_temp), "%s`", friend_name);
   		    std::strcat(temp,_temp);
        }
    }
    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] GETBUDDY (%s)",temp);
	irc_send(conn,RPL_GET_BUDDY,temp);
	return 0;
}

static int _handle_addbuddy_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    char const * friend_name;
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

 	friend_name = params[0];
    my_acc = conn_get_account(conn);

   	friend_acc = accountlist_find_account(friend_name);
    account_add_friend(my_acc, friend_acc);

   	/* FIXME: Check if add friend is done if not then send right message */

    snprintf(temp,sizeof(temp),"%s", friend_name);
	irc_send(conn,RPL_ADD_BUDDY,temp);

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

 	friend_name = params[0];
    my_acc = conn_get_account(conn);

 	num = account_remove_friend2(my_acc, friend_name);

  	/**
    *  FIXME: Check if remove friend is done if not then send right message
    *  Btw I dont know another then RPL_DEL_BUDDY message yet lol.
    */

    snprintf(temp,sizeof(temp),"%s", friend_name);
	irc_send(conn,RPL_DEL_BUDDY,temp);
	return 0;
}

static int _handle_time_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
	std::time_t now;

   	std::memset(temp,0,sizeof(temp));
    now = std::time(NULL);

    snprintf(temp,sizeof(temp),"irc.westwood.com :%lu", now);
	irc_send(conn,RPL_TIME,temp);
    return 0;
}

static int _handle_kick_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_channel * channel;
    t_connection * c;
    unsigned int flags;

 	/**
 	*  Heres the imput expected
    *  KICK [channel] [kicked_user]
    *
    *  Heres the output expected
    *  :user!WWOL@hostname KICK [channel] [kicked_user] :[user_op]
    */

	flags = conn_get_flags(conn);

	if (!flags & MF_BLIZZARD) {
	    snprintf(temp,sizeof(temp),"That command is for Admins/Operators only.");
        message_send_text(conn,message_type_whisper,conn,temp);
        return 0;
    }

    if (params[1])
      c = connlist_find_connection_by_name(params[1],NULL);

	if ((channel = channellist_find_channel_by_name(irc_convert_ircname(params[0]),NULL,NULL))) {
	    snprintf(temp, sizeof(temp), "%s %s :%s", params[0], params[1], conn_get_loggeduser(conn));
   	    channel_message_send(channel,message_wol_kick,conn,temp);
	    if ((conn_wol_get_ingame(c) == 1)) {
		   conn_wol_set_ingame(c,0);
        }
        conn_set_channel(c, NULL);
	}
    else {
     	irc_send(conn,ERR_NOSUCHCHANNEL,":No such channel");
	}
    return 0;
}

static int _handle_mode_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];

   	std::memset(temp,0,sizeof(temp));
    t_channel * channel;

    /**
    * FIXME: CHECK IF USER IS OPERATOR
    * because in WOLv1 is used mode command to add OP or Voice to another user !!!!
    * in WOLv2 is used for change game mode (cooperative, free for all...)
    */

	if ((channel = channellist_find_channel_by_name(irc_convert_ircname(params[0]),NULL,NULL))) {
        snprintf(temp,sizeof(temp),"%s %s %s", params[0], params[1], params[2]);
   	    channel_message_send(channel,message_type_mode,conn,temp);
	}
    else {
     	irc_send(conn,ERR_NOSUCHCHANNEL,":No such channel");
	}
    return 0;
}

static int _handle_host_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_connection * user;

   	std::memset(temp,0,sizeof(temp));

	if ((user = connlist_find_connection_by_accountname(params[0]))) {
        snprintf(temp,sizeof(temp),": %s", text);
        message_send_text(user,message_type_host,conn,temp);
	}
	else {
		irc_send(conn,ERR_NOSUCHNICK,":No such user");
	}
    return 0;
}

static int _handle_advertc_command(t_connection * conn, int numparams, char ** params, char * text)
{
    /* FIXME: Not implemented yet */
    return 0;
}

static int _handle_clanbyname_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	t_clan * clan;
	unsigned int clanid;
	const char * clantag;
	const char * clanname;

	std::memset(temp,0,sizeof(temp));

    if (params[0])
        clan = account_get_clan(accountlist_find_account(params[0]));

    if (clan) {
        clanid = clan_get_clanid(clan);
	    clantag = clantag_to_str(clan_get_clantag(clan));
	    clanname = clan_get_name(clan);
        snprintf(temp, sizeof(temp), "%u`%s`%s`0`0`1`0`0`0`0`0`0`0`x`x`x",clanid,clanname,clantag);
        irc_send(conn,RPL_BATTLECLAN,temp);
    }
    else {
        snprintf(temp, sizeof(temp), "ID does not exist");
	    irc_send(conn,ERR_IDNOEXIST,temp);
    }
    eventlog(eventlog_level_debug,__FUNCTION__,"[** WOL **] CLANBYNAME (%s)",temp);
    return 0;
}

static int _handle_userip_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_connection * user;
    const char * addr = NULL;

   	std::memset(temp,0,sizeof(temp));

    if((user = connlist_find_connection_by_accountname(params[0]))) {
        addr = addr_num_to_ip_str(conn_get_addr(user));
        snprintf(temp,sizeof(temp),"%s",addr);
	    message_send_text(conn,message_wol_userip,conn,temp);
	}
	else {
		irc_send(conn,ERR_NOSUCHNICK,":No such user");
	}
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
