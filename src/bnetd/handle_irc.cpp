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
#include "handle_irc.h"

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
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

typedef int (* t_irc_command)(t_connection * conn, int numparams, char ** params, char * text);

typedef struct {
	const char     * irc_command_string;
	t_irc_command    irc_command_handler;
} t_irc_command_table_row;

static int _handle_user_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_ping_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_pong_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_pass_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_privmsg_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_notice_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_quit_command(t_connection * conn, int numparams, char ** params, char * text);


static int _handle_who_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_list_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_join_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_mode_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_userhost_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_ison_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_whois_command(t_connection * conn, int numparams, char ** params, char * text);
static int _handle_part_command(t_connection * conn, int numparams, char ** params, char * text);


/* state "connected" handlers */
static const t_irc_command_table_row irc_con_command_table[] =
{
	{ "NICK"		, _handle_nick_command },
	{ "USER"		, _handle_user_command },
	{ "PING"		, _handle_ping_command },
	{ "PONG"		, _handle_pong_command },
	{ "PASS"		, _handle_pass_command },
	{ "PRIVMSG"		, _handle_privmsg_command },
	{ "NOTICE"		, _handle_notice_command },
	{ "QUIT"		, _handle_quit_command },

	{ NULL			, NULL }
};

/* state "logged in" handlers */
static const t_irc_command_table_row irc_log_command_table[] =
{
	{ "WHO"			, _handle_who_command },
	{ "LIST"		, _handle_list_command },
	{ "TOPIC"		, _handle_topic_command },
	{ "JOIN"		, _handle_join_command },
	{ "NAMES"		, _handle_names_command },
	{ "MODE"		, _handle_mode_command },
	{ "USERHOST"		, _handle_userhost_command },
	{ "ISON"		, _handle_ison_command },
	{ "WHOIS"		, _handle_whois_command },
	{ "PART"		, _handle_part_command },

	{ NULL			, NULL }
};


static int handle_irc_con_command(t_connection * conn, char const * command, int numparams, char ** params, char * text)
{
  t_irc_command_table_row const *p;

  for (p = irc_con_command_table; p->irc_command_string != NULL; p++) {
    if (strcasecmp(command, p->irc_command_string)==0) {
	  if (p->irc_command_handler != NULL)
		  return ((p->irc_command_handler)(conn,numparams,params,text));
	}
  }
  return -1;
}

static int handle_irc_log_command(t_connection * conn, char const * command, int numparams, char ** params, char * text)
{
  t_irc_command_table_row const *p;

  for (p = irc_log_command_table; p->irc_command_string != NULL; p++) {
    if (strcasecmp(command, p->irc_command_string)==0) {
	  if (p->irc_command_handler != NULL)
		  return ((p->irc_command_handler)(conn,numparams,params,text));
	}
  }
  return -1;
}

static int handle_irc_line(t_connection * conn, char const * ircline)
{
	/* [:prefix] <command> [[param1] [param2] ... [paramN]] [:<text>] */
    char * line; /* copy of ircline */
    char * prefix = NULL; /* optional; mostly NULL */
    char * command; /* mandatory */
    char ** params = NULL; /* optional (array of params) */
    char * text = NULL; /* optional */
	char * bnet_command = NULL;  /* amadeo: used for battle.net.commands */
    int unrecognized_before = 0;
	int linelen; /* amadeo: counter for stringlenghts */

    int numparams = 0;
    char * tempparams;
	int i;
 	char paramtemp[MAX_IRC_MESSAGE_LEN*2];
	int first = 1;

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if (!ircline) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ircline");
	return -1;
    }
    if (ircline[0] == '\0') {
	eventlog(eventlog_level_error,__FUNCTION__,"got empty ircline");
	return -1;
    }
	//amadeo: code was sent by some unknown fellow of pvpgn, prevents buffer-overflow for
	// too long irc-lines

    if (std::strlen(ircline)>254) {
        char * tmp = (char *)ircline;
	eventlog(eventlog_level_warn,__FUNCTION__,"line to long, truncation...");
	tmp[254]='\0';
    }

    line = xstrdup(ircline);

    /* split the message */
    if (line[0] == ':') {
	/* The prefix is optional and is rarely provided */
	prefix = line;
	if (!(command = std::strchr(line,' '))) {
	    eventlog(eventlog_level_warn,__FUNCTION__,"got malformed line (missing command)");
	    xfree(line);
	    return -1;
	}
	*command++ = '\0';
    }
	else {
	/* In most cases command is the first thing on the line */
	command = line;
    }

    tempparams = std::strchr(command,' ');
    if (tempparams) {
	*tempparams++ = '\0';
	 if (tempparams[0]==':') {
	    text = tempparams+1; /* theres just text, no params. skip the colon */
	} else {
	    for (i=0;tempparams[i]!='\0';i++) {
	    	if ((tempparams[i]==' ')&&(tempparams[i+1]==':')) {
		    text = tempparams+i;
		    *text++ = '\0';
		    text++; /* skip the colon */
		    break; /* text found, stop search */
	    	}
	    }
	    params = irc_get_paramelems(tempparams);
	}
    }

    if (params) {
	/* count parameters */
	for (numparams=0;params[numparams];numparams++);
    }

	std::memset(paramtemp,0,sizeof(paramtemp));
    	for (i=0;((numparams>0)&&(params[i]));i++) {
		if (!first)
			std::strcat(paramtemp," ");
	    std::strcat(paramtemp,"\"");
	    std::strcat(paramtemp,params[i]);
	    std::strcat(paramtemp,"\"");
	    first = 0;
    	}

    	eventlog(eventlog_level_debug,__FUNCTION__,"[%d] got \"%s\" \"%s\" [%s] \"%s\"",conn_get_socket(conn),((prefix)?(prefix):("")),command,paramtemp,((text)?(text):("")));

    if (conn_get_state(conn)==conn_state_connected) {
	t_timer_data temp;

	conn_set_state(conn,conn_state_bot_username);
	temp.n = prefs_get_irc_latency();
	conn_test_latency(conn,std::time(NULL),temp);
    }

	if (handle_irc_con_command(conn, command, numparams, params, text)!=-1) {}
    else if (conn_get_state(conn)!=conn_state_loggedin) {
	char temp[MAX_IRC_MESSAGE_LEN+1];

	if ((38+std::strlen(command)+16+1)<sizeof(temp)) {
	    snprintf(temp, sizeof(temp), ":Unrecognized command \"%s\" (before login)", command);
	    irc_send(conn,ERR_UNKNOWNCOMMAND,temp);
	} else {
	    irc_send(conn,ERR_UNKNOWNCOMMAND,":Unrecognized command (before login)");
	}
    } else {
        /* command is handled later */
	unrecognized_before = 1;
    }
    /* --- The following should only be executable after login --- */
    if ((conn_get_state(conn)==conn_state_loggedin)&&(unrecognized_before)) {

		if (handle_irc_log_command(conn, command, numparams, params, text)!=-1) {}
		else if ((strstart(command,"LAG")!=0)&&(strstart(command,"JOIN")!=0)){
			linelen = std::strlen (ircline);
			bnet_command = (char*)xmalloc(linelen + 2);
			bnet_command[0]='/';
			std::strcpy(bnet_command + 1, ircline);
			handle_command(conn,bnet_command);
			xfree((void*)bnet_command);
		}
    } /* loggedin */
    if (params)
	irc_unget_paramelems(params);
    xfree(line);
    return 0;
}

extern int handle_irc_welcome(t_connection * conn)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    std::time_t temptime;
    char const * tempname;
    char const * temptimestr;

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }

    tempname = conn_get_loggeduser(conn);

    if ((34+std::strlen(tempname)+1)<=MAX_IRC_MESSAGE_LEN)
        std::sprintf(temp,":Welcome to the %s IRC Network %s",prefs_get_irc_network_name(), tempname);
    else
        std::sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_WELCOME,temp);

    if ((14+std::strlen(server_get_hostname())+10+std::strlen(PVPGN_SOFTWARE" "PVPGN_VERSION)+1)<=MAX_IRC_MESSAGE_LEN)
        std::sprintf(temp,":Your host is %s, running "PVPGN_SOFTWARE" "PVPGN_VERSION,server_get_hostname());
    else
        std::sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_YOURHOST,temp);

    temptime = server_get_starttime(); /* FIXME: This should be build time */
    temptimestr = std::ctime(&temptime);
    if ((25+std::strlen(temptimestr)+1)<=MAX_IRC_MESSAGE_LEN)
        std::sprintf(temp,":This server was created %s",temptimestr); /* FIXME: is ctime() portable? */
    else
        std::sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_CREATED,temp);

    /* we don't give mode information on MYINFO we give it on ISUPPORT */
    if ((std::strlen(server_get_hostname())+7+std::strlen(PVPGN_SOFTWARE" "PVPGN_VERSION)+9+1)<=MAX_IRC_MESSAGE_LEN)
        std::sprintf(temp,"%s "PVPGN_SOFTWARE" "PVPGN_VERSION" - -",server_get_hostname());
    else
        std::sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_MYINFO,temp);

    std::sprintf(temp,"NICKLEN=%d TOPICLEN=%d CHANNELLEN=%d PREFIX="CHANNEL_PREFIX" CHANTYPES="CHANNEL_TYPE" NETWORK=%s IRCD="PVPGN_SOFTWARE,
    MAX_CHARNAME_LEN, MAX_TOPIC_LEN, MAX_CHANNELNAME_LEN, prefs_get_irc_network_name());

    if((std::strlen(temp))<=MAX_IRC_MESSAGE_LEN)
        irc_send(conn,RPL_ISUPPORT,temp);
    else {
        std::sprintf(temp,":Maximum length exceeded");
        irc_send(conn,RPL_ISUPPORT,temp);
    }

    irc_send_motd(conn);

    message_send_text(conn,message_type_notice,NULL,"This is an experimental service");

    conn_set_state(conn,conn_state_bot_password);
    if (connlist_find_connection_by_accountname(conn_get_loggeduser(conn))) {
       message_send_text(conn,message_type_notice,NULL,"This account is already logged in, use another account.");
	return -1;
    }

    if (conn_get_ircpass(conn)) {
        message_send_text(conn,message_type_notice,NULL,"Trying to authenticate with PASS ...");
	irc_authenticate(conn,conn_get_ircpass(conn));
    } else {
        message_send_text(conn,message_type_notice,NULL,"No PASS command received. Please identify yourself by /msg NICKSERV identify <password>.");
    }
    return 0;
}

extern int handle_irc_packet(t_connection * conn, t_packet const * const packet)
{
    unsigned int i;
    char ircline[MAX_IRC_MESSAGE_LEN];
    char const * data;

    if (!packet) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL packet");
	return -1;
    }
    if (conn_get_class(conn) != conn_class_irc) {
	eventlog(eventlog_level_error,__FUNCTION__,"FIXME: handle_irc_packet without any reason (conn->class != conn_class_irc)");
	return -1;
    }

    /* eventlog(eventlog_level_debug,__FUNCTION__,"got \"%s\"",packet_get_raw_data_const(packet,0)); */

    std::memset(ircline,0,sizeof(ircline));
    data = conn_get_ircline(conn); /* fetch current status */
    if (data)
	std::strcpy(ircline,data);
    unsigned ircpos = std::strlen(ircline);
    data = (const char *)packet_get_raw_data_const(packet,0);
    for (i=0; i < packet_get_size(packet); i++) {
	if ((data[i] == '\r')||(data[i] == '\0')) {
	    /* kindly ignore \r and NUL ... */
	} else if (data[i] == '\n') {
	    /* end of line */
	    handle_irc_line(conn,ircline);
	    std::memset(ircline,0,sizeof(ircline));
	    ircpos = 0;
	} else {
	    if (ircpos < MAX_IRC_MESSAGE_LEN-1)
		ircline[ircpos++] = data[i];
	    else {
		ircpos++; /* for the statistic :) */
	    	eventlog(eventlog_level_warn,__FUNCTION__,"[%d] client exceeded maximum allowed message length by %d characters",conn_get_socket(conn),ircpos-MAX_IRC_MESSAGE_LEN);
		if (ircpos > 100 + MAX_IRC_MESSAGE_LEN) {
		    /* automatic flood protection */
		    eventlog(eventlog_level_error,__FUNCTION__,"[%d] excess flood",conn_get_socket(conn));
		    return -1;
		}
	    }
	}
    }
    conn_set_ircline(conn,ircline); /* write back current status */
    return 0;
}

static int _handle_user_command(t_connection * conn, int numparams, char ** params, char * text)
{
	/* RFC 2812 says: */
	/* <user> <mode> <unused> :<realname>*/
	/* ircII and X-Chat say: */
	/* mz SHODAN localhost :Marco Ziech */
	/* BitchX says: */
	/* mz +iws mz :Marco Ziech */
	/* Don't bother with, params 1 and 2 anymore they don't contain what they should. */
	char * user = NULL;
	char * realname = NULL;

	if ((numparams>=3)&&(params[0])&&(text)) {
	    user = params[0];
	    realname = text;

		if (conn_get_user(conn)) {
			irc_send(conn,ERR_ALREADYREGISTRED,":You are already registred");
		}
		else {
			eventlog(eventlog_level_debug,__FUNCTION__,"[%d] got USER: user=\"%s\" realname=\"%s\"",conn_get_socket(conn),user,realname);
			conn_set_user(conn,user);
			conn_set_owner(conn,realname);
			if (conn_get_loggeduser(conn))
			   handle_irc_welcome(conn); /* only send the welcome if we have USER and NICK */
   		}
   	}
	else {
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to USER");
    	}
	return 0;
}

static int _handle_ping_command(t_connection * conn, int numparams, char ** params, char * text)
{
	/* Dizzy: just ignore this because RFC says we should not reply client PINGs
	 * NOTE: RFC2812 doesn't seem to be very expressive about this ... */
	if (numparams)
	    irc_send_pong(conn,params[0]);
	else
	    irc_send_pong(conn,text);
	return 0;
}

static int _handle_pong_command(t_connection * conn, int numparams, char ** params, char * text)
{
	/* NOTE: RFC2812 doesn't seem to be very expressive about this ... */
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

static int _handle_pass_command(t_connection * conn, int numparams, char ** params, char * text)
{
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
						message_send_text(conn,message_type_error,conn,"Syntax: REGISTER <password> (max 16 characters)");
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
							channel_message_log(channel, conn, 1, text);
							channel_message_send(channel,message_type_talk,conn,text);
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

static int _handle_notice_command(t_connection * conn, int numparams, char ** params, char * text)
{
	if ((numparams>=1)&&(text)) {
	    int i;
	    char ** e;

	    e = irc_get_listelems(params[0]);
	    /* FIXME: support wildcards! */

	    for (i=0;((e)&&(e[i]));i++) {
			if (conn_get_state(conn)==conn_state_loggedin) {
				t_connection * user;

				if ((user = connlist_find_connection_by_accountname(e[i]))) {
                          message_send_text(user,message_type_notice,conn,text);
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
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to PRIVMSG");
	return 0;
}

static int _handle_who_command(t_connection * conn, int numparams, char ** params, char * text)
{
	if (numparams>=1) {
	    int i;
	    char ** e;

	    e = irc_get_listelems(params[0]);
	    for (i=0; ((e)&&(e[i]));i++) {
	    	irc_who(conn,e[i]);
	    }
	    irc_send(conn,RPL_ENDOFWHO,":End of WHO list"); /* RFC2812 only requires this to be sent if a list of names was given. Undernet seems to always send it, so do we :) */
        if (e)
			irc_unget_listelems(e);
	}
	else
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to WHO");
	return 0;
}

static int _handle_list_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];

	irc_send(conn,RPL_LISTSTART,"Channel :Users Names"); /* backward compatibility */

	if (numparams==0) {
 	    t_elem const * curr;

   	    LIST_TRAVERSE_CONST(channellist(),curr)
			{
    	    t_channel const * channel = (const t_channel*)elem_get_data(curr);
	        char const * tempname;
			char * topic = channel_get_topic(channel_get_name(channel));

	        tempname = irc_convert_channel(channel);

			/* FIXME: AARON: only list channels like in /channels command */
			if (topic) {
	        	if (std::strlen(tempname)+1+20+1+1+std::strlen(topic)<MAX_IRC_MESSAGE_LEN)
		    		snprintf(temp, sizeof(temp), "%s %u :%s",tempname,channel_get_length(channel),topic);
	        	else
	            	eventlog(eventlog_level_warn,__FUNCTION__,"LISTREPLY length exceeded");
			}
			else {
	        	if (std::strlen(tempname)+1+20+1+1<MAX_IRC_MESSAGE_LEN)
		    		snprintf(temp, sizeof(temp), "%s %u :",tempname,channel_get_length(channel));
	        		else
	            			eventlog(eventlog_level_warn,__FUNCTION__,"LISTREPLY length exceeded");
			}
	        	irc_send(conn,RPL_LIST,temp);
    	}
    }
	else if (numparams>=1) {
        int i;
        char ** e;

	e = irc_get_listelems(params[0]);
		/* FIXME: support wildcards! */

		for (i=0;((e)&&(e[i]));i++) {
		t_channel const * channel;
		char const * verytemp; /* another good example for creative naming conventions :) */
	       	char const * tempname;
		char * topic;

		verytemp = irc_convert_ircname(e[i]);
		if (!verytemp)
			continue; /* something is wrong with the name ... */
		channel = channellist_find_channel_by_name(verytemp,NULL,NULL);
		if (!channel)
			continue; /* channel doesn't exist */

		topic = channel_get_topic(channel_get_name(channel));
	       	tempname = irc_convert_channel(channel);

			if (topic) {
	       		if (std::strlen(tempname)+1+20+1+1+std::strlen(topic)<MAX_IRC_MESSAGE_LEN)
	    			snprintf(temp, sizeof(temp), "%s %u :%s", tempname,channel_get_length(channel), topic);
	       		else
	       			eventlog(eventlog_level_warn,__FUNCTION__,"LISTREPLY length exceeded");
		}
			else {
	       		if (std::strlen(tempname)+1+20+1+1<MAX_IRC_MESSAGE_LEN)
	    			snprintf(temp, sizeof(temp), "%s %u :", tempname,channel_get_length(channel));
	       		else
	       			eventlog(eventlog_level_warn,__FUNCTION__,"LISTREPLY length exceeded");
		}
	       	irc_send(conn,RPL_LIST,temp);
	}
        if (e)
		irc_unget_listelems(e);
    }
    irc_send(conn,RPL_LISTEND,":End of LIST command");
	return 0;
}

static int _handle_join_command(t_connection * conn, int numparams, char ** params, char * text)
{
	if (numparams>=1) {
	    char ** e;

	    e = irc_get_listelems(params[0]);
	    if ((e)&&(e[0])) {
	    		char const * ircname = irc_convert_ircname(e[0]);
	   	 	t_channel * old_channel = conn_get_channel(conn);

			if ((!(ircname)) || (conn_set_channel(conn,ircname)<0)) {
				irc_send(conn,ERR_NOSUCHCHANNEL,":JOIN failed"); /* FIXME: be more precise; what is the real error code for that? */
			}
			else {
    			char temp[MAX_IRC_MESSAGE_LEN];
				t_channel * channel;
				channel = conn_get_channel(conn);

				if (channel!=old_channel) {
				    channel_set_userflags(conn);
				}

	    		}
		}
    		if (e)
			irc_unget_listelems(e);
	}
	else
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to JOIN");
	return 0;
}

static int _handle_mode_command(t_connection * conn, int numparams, char ** params, char * text)
{
	/* FIXME: Not yet implemented */
	return 0;
}

static int _handle_userhost_command(t_connection * conn, int numparams, char ** params, char * text)
{
	/* FIXME: Send RPL_USERHOST */
	return 0;
}

static int _handle_quit_command(t_connection * conn, int numparams, char ** params, char * text)
{
	conn_set_channel(conn, NULL);
	conn_set_state(conn, conn_state_destroy);
	return 0;
}

static int _handle_ison_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	char first = 1;

	if (numparams>=1) {
	    int i;

	    temp[0]='\0';
	    for (i=0; (i<numparams && (params) && (params[i]));i++) {
    	  if (connlist_find_connection_by_accountname(params[i])) {
		    if (first)
		        std::strcat(temp,":");
		    else
		        std::strcat(temp," ");
		    std::strcat(temp,params[i]);
		    first = 0;
		  }
	    }
	    irc_send(conn,RPL_ISON,temp);
	}
	else
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to ISON");
	return 0;
}

static int _handle_whois_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char temp[MAX_IRC_MESSAGE_LEN];
	char temp2[MAX_IRC_MESSAGE_LEN];
	if (numparams>=1) {
	    int i;
	    char ** e;
	    t_connection * c;
	    t_channel * chan;

	    temp[0]='\0';
	    temp2[0]='\0';
	    e = irc_get_listelems(params[0]);
	    for (i=0; ((e)&&(e[i]));i++) {
    	  if ((c = connlist_find_connection_by_accountname(e[i]))) {
		    if (prefs_get_hide_addr() && !(account_get_command_groups(conn_get_account(conn)) & command_get_group("/admin-addr")))
		      snprintf(temp, sizeof(temp), "%s %s hidden * :%s", e[i], clienttag_uint_to_str(conn_get_clienttag(c)), "PvPGN user");
		    else
		      snprintf(temp, sizeof(temp), "%s %s %s * :%s", e[i], clienttag_uint_to_str(conn_get_clienttag(c)), addr_num_to_ip_str(conn_get_addr(c)), "PvPGN user");
		    irc_send(conn,RPL_WHOISUSER,temp);

		    if ((chan=conn_get_channel(conn))) {
			char flg;
			unsigned int flags;

			flags = conn_get_flags(c);

	            	if (flags & MF_BLIZZARD)
		            flg='@';
	            	else if ((flags & MF_BNET) || (flags & MF_GAVEL))
		            flg='%';
	            	else if (flags & MF_VOICE)
		            flg='+';
		        else flg = ' ';
			snprintf(temp2, sizeof(temp2), "%s :%c%s", e[i], flg, irc_convert_channel(chan));
			irc_send(conn,RPL_WHOISCHANNELS,temp2);
		    }

		  }
		  else
		    irc_send(conn,ERR_NOSUCHNICK,":No such nick/channel");

	    }
	    irc_send(conn,RPL_ENDOFWHOIS,":End of /WHOIS list");
        if (e)
			irc_unget_listelems(e);
	}
	else
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to WHOIS");
	return 0;
}

static int _handle_part_command(t_connection * conn, int numparams, char ** params, char * text)
{
    conn_set_channel(conn, NULL);
    return 0;
}

}

}
