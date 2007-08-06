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
#include "irc.h"

#include <cstring>
#include <cstdio>
#include <ctime>
#include <cstdlib>

#include "common/irc_protocol.h"
#include "common/packet.h"
#include "common/eventlog.h"
#include "common/field_sizes.h"
#include "common/bnethash.h"
#include "common/xalloc.h"
#include "common/addr.h"
#include "common/tag.h"
#include "common/list.h"
#include "common/util.h"

#include "message.h"
#include "channel.h"
#include "connection.h"
#include "server.h"
#include "account.h"
#include "account_wrap.h"
#include "prefs.h"
#include "tick.h"
#include "handle_irc.h"
#include "handle_wol.h"
#include "command_groups.h"
#include "topic.h"
#include "clan.h"
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

typedef struct {
    char const * nick;
    char const * user;
    char const * host;
} t_irc_message_from;


static char ** irc_split_elems(char * list, int separator, int ignoreblank);
static int irc_unget_elems(char ** elems);
static char * irc_message_preformat(t_irc_message_from const * from, char const * command, char const * dest, char const * text);

extern int irc_send_cmd(t_connection * conn, char const * command, char const * params)
{
    t_packet * p;
    char data[MAX_IRC_MESSAGE_LEN+1];
    unsigned len;
    char const * ircname = server_get_hostname();
    char const * nick;

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if (!command) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL command");
	return -1;
    }
    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create packet");
	return -1;
    }

    nick = conn_get_loggeduser(conn);
    if (!nick)
    	nick = "";

    /* snprintf isn't portable -> check message length first */
    if (params) {
        len = 1+std::strlen(ircname)+1+std::strlen(command)+1+std::strlen(nick)+1+std::strlen(params)+2;
	if (len > MAX_IRC_MESSAGE_LEN) {
	    eventlog(eventlog_level_error,__FUNCTION__,"message to send is too large (%u bytes)",len);
	    return -1;
	}
	else
	    std::sprintf(data,":%s %s %s %s\r\n",ircname,command,nick,params);
    } else {
        len = 1+std::strlen(ircname)+1+std::strlen(command)+1+std::strlen(nick)+1+2;
    	if (len > MAX_IRC_MESSAGE_LEN) {
	    eventlog(eventlog_level_error,__FUNCTION__,"message to send is too large (%u bytes)",len);
	    return -1;
	}
	else
	std::sprintf(data,":%s %s %s\r\n",ircname,command,nick);
    }
    packet_set_size(p,0);
    packet_append_data(p,data,len);
    // eventlog(eventlog_level_debug,__FUNCTION__,"[%d] sent \"%s\"",conn_get_socket(conn),data);
    conn_push_outqueue(conn,p);
    packet_del_ref(p);
    return 0;
}

extern int irc_send(t_connection * conn, int code, char const * params)
{
    char temp[4]; /* '000\0' */

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if ((code>999)||(code<0)) { /* more than 3 digits or negative */
	eventlog(eventlog_level_error,__FUNCTION__,"invalid message code (%d)",code);
	return -1;
    }
    std::sprintf(temp,"%03u",code);
    return irc_send_cmd(conn,temp,params);
}

extern int irc_send_ping(t_connection * conn)
{
    t_packet * p;
    char data[MAX_IRC_MESSAGE_LEN];

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }

    if ((conn_get_clienttag(conn) != CLIENTTAG_WCHAT_UINT) &&
        (conn_get_clienttag(conn) != CLIENTTAG_IIRC_UINT))
        return 0;

    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create packet");
	return -1;
    }

    conn_set_ircping(conn,get_ticks());
    if (conn_get_state(conn)==conn_state_bot_username)
    	std::sprintf(data,"PING :%u\r\n",conn_get_ircping(conn)); /* Undernet doesn't reveal the servername yet ... neither do we */
    else if ((6+std::strlen(server_get_hostname())+2+1)<=MAX_IRC_MESSAGE_LEN)
    	std::sprintf(data,"PING :%s\r\n",server_get_hostname());
    else
    	eventlog(eventlog_level_error,__FUNCTION__,"maximum message length exceeded");
    eventlog(eventlog_level_debug,__FUNCTION__,"[%d] sent \"%s\"",conn_get_socket(conn),data);
    packet_set_size(p,0);
    packet_append_data(p,data,std::strlen(data));
    conn_push_outqueue(conn,p);
    packet_del_ref(p);
    return 0;
}

extern int irc_send_pong(t_connection * conn, char const * params)
{
    t_packet * p;
    char data[MAX_IRC_MESSAGE_LEN];

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if ((1+std::strlen(server_get_hostname())+1+4+1+std::strlen(server_get_hostname())+((params)?(2+std::strlen(params)):(0))+2+1) > MAX_IRC_MESSAGE_LEN) {
	eventlog(eventlog_level_error,__FUNCTION__,"max message length exceeded");
	return -1;
    }
    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create packet");
	return -1;
    }

    if (params)
    	std::sprintf(data,":%s PONG %s :%s\r\n",server_get_hostname(),server_get_hostname(),params);
    else
    	std::sprintf(data,":%s PONG %s\r\n",server_get_hostname(),server_get_hostname());
    eventlog(eventlog_level_debug,__FUNCTION__,"[%d] sent \"%s\"",conn_get_socket(conn),data);
    packet_set_size(p,0);
    packet_append_data(p,data,std::strlen(data));
    conn_push_outqueue(conn,p);
    packet_del_ref(p);
    return 0;
}

extern int irc_authenticate(t_connection * conn, char const * passhash)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    t_hash h1;
    t_hash h2;
    t_account * a;
    char const * temphash;
    char const * username;

    char const * tempapgar;

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
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
        message_send_text(conn,message_type_notice,NULL,"Authentication failed.");
	return 0;
    }

    if (connlist_find_connection_by_account(a) && prefs_get_kick_old_login()==0) {
            message_send_text(conn,message_type_notice,NULL,"Authentication rejected (already logged in) ");
    }
    else if (account_get_auth_lock(a)==1) {
            message_send_text(conn,message_type_notice,NULL,"Authentication rejected (account is locked) ");
    }
    else
    {
     	if((conn_get_wol(conn) == 1)) {
    	    temphash = account_get_wol_apgar(a);
    	    tempapgar = conn_wol_get_apgar(conn);

    	    if(temphash == NULL) {
        		account_set_wol_apgar(a,tempapgar);
        		temphash = account_get_wol_apgar(a);
    	    }

    	    if(tempapgar == NULL) {
                irc_send(conn,RPL_BAD_LOGIN,":You have specified an invalid password for that nickname."); /* bad APGAR */
                //std::sprintf(temp,":Closing Link %s[Some.host]:(Password needed for that nickname.)",conn_get_loggeduser(conn));
                //message_send_text(conn,message_type_error,conn,temp);
                conn_increment_passfail_count(conn);
                return 0;
            }

    	    if(std::strcmp(temphash,tempapgar) == 0) {
                conn_login(conn,a,username);
    	        conn_set_state(conn,conn_state_loggedin);
        		return 1;
    	    }
    	    else {
                irc_send(conn,RPL_BAD_LOGIN,":You have specified an invalid password for that nickname."); /* bad APGAR */
                //std::sprintf(temp,":Closing Link %s[Some.host]:(Password needed for that nickname.)",conn_get_loggeduser(conn));
                ///message_send_text(conn,message_type_error,conn,temp);
        		conn_increment_passfail_count(conn);
        		return 0;
    	    }
    	}

        hash_set_str(&h1,passhash);
        temphash = account_get_pass(a);
        hash_set_str(&h2,temphash);
        if (hash_eq(h1,h2)) {
            conn_login(conn,a,username);
            conn_set_state(conn,conn_state_loggedin);
            conn_set_clienttag(conn,CLIENTTAG_IIRC_UINT); /* IIRC hope here is ok */
            message_send_text(conn,message_type_notice,NULL,"Authentication successful. You are now logged in.");
	    return 1;
        } else {
            message_send_text(conn,message_type_notice,NULL,"Authentication failed."); /* wrong password */
	    conn_increment_passfail_count(conn);
        }
    }
    return 0;
}

/* Channel name conversion rules: */
/* Not allowed in IRC (RFC2812): NUL, BELL, CR, LF, ' ', ':' and ','*/
/*   ' '  -> '_'      */
/*   '_'  -> '%_'     */
/*   '%'  -> '%%'     */
/*   '\b' -> '%b'     */
/*   '\n' -> '%n'     */
/*   '\r' -> '%r'     */
/*   ':'  -> '%='     */
/*   ','  -> '%-'     */
/* In IRC a channel can be specified by '#'+channelname or '!'+channelid */
extern char const * irc_convert_channel(t_channel const * channel)
{
    char const * bname;
    static char out[MAX_CHANNELNAME_LEN];
    unsigned int outpos;
    int i;

    if (!channel)
	return "*";

    std::memset(out,0,sizeof(out));
    out[0] = '#';
    outpos = 1;
    bname = channel_get_name(channel);
    for (i=0; bname[i]!='\0'; i++) {
	if (bname[i]==' ') {
	    out[outpos++] = '_';
	} else if (bname[i]=='_') {
	    out[outpos++] = '%';
	    out[outpos++] = '_';
	} else if (bname[i]=='%') {
	    out[outpos++] = '%';
	    out[outpos++] = '%';
	} else if (bname[i]=='\b') {
	    out[outpos++] = '%';
	    out[outpos++] = 'b';
	} else if (bname[i]=='\n') {
	    out[outpos++] = '%';
	    out[outpos++] = 'n';
	} else if (bname[i]=='\r') {
	    out[outpos++] = '%';
	    out[outpos++] = 'r';
	} else if (bname[i]==':') {
	    out[outpos++] = '%';
	    out[outpos++] = '=';
	} else if (bname[i]==',') {
	    out[outpos++] = '%';
	    out[outpos++] = '-';
	} else {
	    out[outpos++] = bname[i];
	}
	if ((outpos+2)>=(sizeof(out))) {
	    std::sprintf(out,"!%u",channel_get_channelid(channel));
	    return out;
	}
    }
    return out;
}

extern char const * irc_convert_ircname(char const * pircname)
{
    static char out[MAX_CHANNELNAME_LEN];
    unsigned int outpos;
    int special;
    int i;
    char const * ircname = pircname + 1;

    if (!ircname) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ircname");
	return NULL;
    }

    outpos = 0;
    std::memset(out,0,sizeof(out));
    special = 0;
    if (pircname[0]=='!') {
	t_channel * channel;

	channel = channellist_find_channel_bychannelid(std::atoi(ircname));
	if (channel)
	    return channel_get_name(channel);
	else
	    return NULL;
    } else if (pircname[0]!='#') {
	return NULL;
    }
    for (i=0; ircname[i]!='\0'; i++) {
    	if (ircname[i]=='_') {
	    out[outpos++] = ' ';
    	} else if (ircname[i]=='%') {
	    if (special) {
		out[outpos++] = '%';
		special = 0;
	    } else {
		special = 1;
	    }
    	} else if (special) {
	    if (ircname[i]=='_') {
		out[outpos++] = '_';
	    } else if (ircname[i]=='b') {
		out[outpos++] = '\b';
	    } else if (ircname[i]=='n') {
		out[outpos++] = '\n';
	    } else if (ircname[i]=='r') {
		out[outpos++] = '\r';
	    } else if (ircname[i]=='=') {
		out[outpos++] = ':';
	    } else if (ircname[i]=='-') {
		out[outpos++] = ',';
	    } else {
		/* maybe it's just a typo :) */
		out[outpos++] = '%';
		out[outpos++] = ircname[i];
	    }
    	} else {
    	    out[outpos++] = ircname[i];
    	}
	if ((outpos+2)>=(sizeof(out))) {
	    return NULL;
	}
    }
    return out;
}

/* splits an string list into its elements */
/* (list will be modified) */
static char ** irc_split_elems(char * list, int separator, int ignoreblank)
{
    int i;
    int count;
    char ** out;

    if (!list) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL list");
	return NULL;
    }

    for (count=0,i=0;list[i]!='\0';i++) {
	if (list[i]==separator) {
	    count++;
	    if (ignoreblank) {
	        /* ignore more than one separators "in a row" */
		while ((list[i+1]!='\0')&&(list[i]==separator)) i++;
	    }
	}
    }
    count++; /* count separators -> we have one more element ... */
    /* we also need a terminating element */
    out = (char**)xmalloc((count+1)*sizeof(char *));

    out[0] = list;
    if (count>1) {
	for (i=1;i<count;i++) {
	    out[i] = std::strchr(out[i-1],separator);
	    if (!out[i]) {
		eventlog(eventlog_level_error,__FUNCTION__,"BUG: wrong number of separators");
		xfree(out);
		return NULL;
	    }
	    if (ignoreblank)
	    	while ((*out[i]+1)==separator) out[i]++;
	    *out[i]++ = '\0';
	}
	if ((ignoreblank)&&(out[count-1])&&(*out[count-1]=='\0')) {
	    out[count-1] = NULL; /* last element is blank */
	}
    } else if ((ignoreblank)&&(*out[0]=='\0')) {
	out[0] = NULL; /* now we have 2 terminators ... never mind */
    }
    out[count] = NULL; /* terminating element */
    return out;
}

static int irc_unget_elems(char ** elems)
{
    if (!elems) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL elems");
	return -1;
    }
    xfree(elems);
    return 0;
}

extern char ** irc_get_listelems(char * list)
{
    return irc_split_elems(list,',',0);
}

extern int irc_unget_listelems(char ** elems)
{
    return irc_unget_elems(elems);
}

extern char ** irc_get_paramelems(char * list)
{
    return irc_split_elems(list,' ',1);
}

extern int irc_unget_paramelems(char ** elems)
{
    return irc_unget_elems(elems);
}

static char * irc_message_preformat(t_irc_message_from const * from, char const * command, char const * dest, char const * text)
{
    char * myfrom;
    char const * mydest = "";
    char const * mytext = "";
    int len;
    char * msg;

    if (!command) {
	    eventlog(eventlog_level_error,__FUNCTION__,"got NULL command");
	    return NULL;
    }
    if (from) {
	    if ((!from->nick)||(!from->user)||(!from->host)) {
	        eventlog(eventlog_level_error,__FUNCTION__,"got malformed from");
	        return NULL;
	    }
	    myfrom = (char*)xmalloc(std::strlen(from->nick)+1+std::strlen(from->user)+1+std::strlen(from->host)+1); /* nick + "!" + user + "@" + host + "\0" */
	    std::sprintf(myfrom,"%s!%s@%s",from->nick,from->user,from->host);
    } else
    	myfrom = xstrdup(server_get_hostname());
    if (dest)
    	mydest = dest;

    if (text)
    	mytext = text;

    len = 1+std::strlen(myfrom)+1+
    	  std::strlen(command)+1+
    	  std::strlen(mydest)+1+
    	  1+std::strlen(mytext)+1;


    msg = (char*)xmalloc(len);

    std::sprintf(msg,":%s\n%s\n%s\n%s",myfrom,command,mydest,mytext);
    xfree(myfrom);
    return msg;
}

extern int irc_message_postformat(t_packet * packet, t_connection const * dest)
{
    /* the four elements */
    char * e1;
    char * e1_2;
    char * e2;
    char * e3;
    char * e4;
    char const * tname = NULL;
    char const * toname = "AUTH"; /* fallback name */
    const char * temp;

    if (!packet) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL packet");
	return -1;
    }
    if (!dest) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL dest");
	return -1;
    }

    e1 = (char*)packet_get_raw_data(packet,0);
    e2 = std::strchr(e1,'\n');
    if (!e2) {
	eventlog(eventlog_level_warn,__FUNCTION__,"malformed message (e2 missing)");
	return -1;
    }
    *e2++ = '\0';
    e3 = std::strchr(e2,'\n');
    if (!e3) {
	eventlog(eventlog_level_warn,__FUNCTION__,"malformed message (e3 missing)");
	return -1;
    }
    *e3++ = '\0';
    e4 = std::strchr(e3,'\n');
    if (!e4) {
	eventlog(eventlog_level_warn,__FUNCTION__,"malformed message (e4 missing)");
	return -1;
    }
    *e4++ = '\0';

    if (prefs_get_hide_addr() && !(account_get_command_groups(conn_get_account(dest)) & command_get_group("/admin-addr")))
    {
      e1_2 = std::strchr(e1,'@');
      if (e1_2)
      {
	  *e1_2++ = '\0';
      }
    }
    else
    e1_2 = NULL;

    if (e3[0]=='\0') { /* fill in recipient */
    	if ((tname = conn_get_loggeduser(dest)))
    	    toname = tname;
    } else
    	toname = e3;

    if (std::strcmp(toname,"\r")==0) {
    	toname = ""; /* HACK: the target field is really empty */
    	temp = "";
    } else
    	temp = " ";

    if (std::strlen(e1)+1+std::strlen(e2)+1+std::strlen(toname)+std::strlen(temp)+std::strlen(e4)+2+1 <= MAX_IRC_MESSAGE_LEN) {
	char msg[MAX_IRC_MESSAGE_LEN+1];

	if (e1_2)
	    std::sprintf(msg,"%s@hidden %s %s%s%s\r\n",e1,e2,toname,temp,e4);
	else
	    std::sprintf(msg,"%s %s %s%s%s\r\n",e1,e2,toname,temp,e4);
	eventlog(eventlog_level_debug,__FUNCTION__,"sent \"%s\"",msg);
	packet_set_size(packet,0);
	packet_append_data(packet,msg,std::strlen(msg));
	if (tname)
	    conn_unget_chatname(dest,tname);
	return 0;
    } else {
	/* FIXME: split up message? */
    	eventlog(eventlog_level_warn,__FUNCTION__,"maximum IRC message length exceeded");
	if (tname)
	    conn_unget_chatname(dest,tname);
	return -1;
    }
}

extern int irc_message_format(t_packet * packet, t_message_type type, t_connection * me, t_connection * dst, char const * text, unsigned int dstflags)
{
    char * msg;
    char const * ctag;
    t_irc_message_from from;

    if (!packet)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL packet");
	return -1;
    }

    msg = NULL;
    if (me)
        ctag = clienttag_uint_to_str(conn_get_clienttag(me));
    else
	    ctag = clienttag_uint_to_str(CLIENTTAG_IIRC_UINT);

    switch (type)
    {
    /* case message_type_adduser: this is sent manually in handle_irc */
	case message_type_adduser:
		/* when we do it somewhere else, then we can also make sure to not get our logs spammed */
		break;
    case message_type_join:
    {
        t_channel * channel;
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));

        channel = conn_get_channel(me);

	if((conn_get_wol(dst) == 1) && (conn_get_clienttag(dst) != CLIENTTAG_WCHAT_UINT))
	{
            char temp[MAX_IRC_MESSAGE_LEN];
    	    t_clan * clan;
    	    unsigned int clanid = 0;
    	    std::memset(temp,0,sizeof(temp));
	    
	    /**
            *  For WOLv2 the channel JOIN output must be like the following:
    	    *  user!WWOL@hostname JOIN :clanID,longIP channelName
            */

    	    clan = account_get_clan(conn_get_account(me));

    	    if (clan)
    	        clanid = clan_get_clanid(clan);

            std::sprintf(temp,":%u,%u",clanid,conn_get_addr(me));
    	    msg = irc_message_preformat(&from,"JOIN",temp,irc_convert_channel(channel));
        }
        else if (conn_get_clienttag(dst) == CLIENTTAG_WCHAT_UINT) {
            char temp[MAX_IRC_MESSAGE_LEN];
    	    std::memset(temp,0,sizeof(temp));

            if (conn_wol_get_ingame(me) == 1) {
                std::sprintf(temp,"2 %u %u 1 1 %u :%s", channel_get_length(channel), channel_wol_get_game_type(channel), channel_wol_get_game_tournament(channel), irc_convert_channel(channel));
                msg = irc_message_preformat(&from,"JOINGAME",temp,irc_convert_channel(channel));
            }
            else
	    {
                msg = irc_message_preformat(&from,"JOIN","\r",irc_convert_channel(channel));
	    }
        }
        else {
            msg = irc_message_preformat(&from,"JOIN","\r",irc_convert_channel(channel));
        }
    	conn_unget_chatname(me,from.nick);
    }
    	break;
    case message_type_part:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"PART","\r",irc_convert_channel(conn_get_channel(me)));
    	conn_unget_chatname(me,from.nick);
    	break;
    case message_type_talk:
    case message_type_whisper:
    	{
    	    char const * dest;
	    char temp[MAX_IRC_MESSAGE_LEN];

	    if (me) {
    	        from.nick = conn_get_chatname(me);
    	        from.host = addr_num_to_ip_str(conn_get_addr(me));
	    }
	    else {
                from.nick = server_get_hostname();
                from.host = server_get_hostname();
	    }
        from.user = ctag;

        if (type==message_type_talk)
            dest = irc_convert_channel(conn_get_channel(me)); /* FIXME: support more channels and choose right one! */
        else
            dest = ""; /* will be replaced with username in postformat */

        std::sprintf(temp,":%s",text);

        if ((conn_get_wol(me) != 1) && (conn_get_wol(dst) == 1) && (conn_wol_get_pageme(dst))
             && (type==message_type_whisper) && (conn_get_channel(me)!=(conn_get_channel(dst))))
            msg = irc_message_preformat(&from,"PAGE",NULL,temp);
        else
            msg = irc_message_preformat(&from,"PRIVMSG",dest,temp);

        if (me)
    	    conn_unget_chatname(me,from.nick);
    }
    break;
    case message_type_emote:
    	{
	    char const * dest;
	    char temp[MAX_IRC_MESSAGE_LEN];

	    /* "\001ACTION " + text + "\001" + \0 */
	    if ((8+std::strlen(text)+1+1)<=MAX_IRC_MESSAGE_LEN) {
		std::sprintf(temp,":\001ACTION %s\001",text);
	    }
        else {
		std::sprintf(temp,":\001ACTION (maximum message length exceeded)\001");
	    }
    	    from.nick = conn_get_chatname(me);
            from.user = ctag;
    	    from.host = addr_num_to_ip_str(conn_get_addr(me));
    	    /* FIXME: also supports whisper emotes? */
    	    dest = irc_convert_channel(conn_get_channel(me)); /* FIXME: support more channels and choose right one! */
	    msg = irc_message_preformat(&from,"PRIVMSG",dest,temp);
    	    conn_unget_chatname(me,from.nick);
    	}
        break;
    case message_type_broadcast:
    case message_type_info:
    case message_type_error:
	{
	    char temp[MAX_IRC_MESSAGE_LEN];
	    std::sprintf(temp,":%s",text);
        if ((type==message_type_info || type==message_type_error) && (conn_get_wol(dst) == 1))
            msg = irc_message_preformat(NULL,"PAGE",NULL,temp);
        else
	        msg = irc_message_preformat(NULL,"NOTICE",NULL,temp);
	}
	break;
    case message_type_channel:
    	/* ignore it */
	break;
    case message_type_mode:
	    from.nick = conn_get_chatname(me);
	    from.user = ctag;
	    from.host = addr_num_to_ip_str(conn_get_addr(me));
	    msg = irc_message_preformat(&from,"MODE","\r",text);
	    conn_unget_chatname(me,from.nick);
    break;
	case message_type_nick:
	{
        from.nick = conn_get_loggeduser(me);
        from.host = addr_num_to_ip_str(conn_get_addr(me));
        from.user = ctag;
        msg = irc_message_preformat(&from,"NICK","\r",text);
	}
    break;
    case message_type_notice:
	{
   	    char temp[MAX_IRC_MESSAGE_LEN];
	    std::sprintf(temp,":%s",text);

        if (me && conn_get_chatname(me))
	    {
    	        from.nick = conn_get_chatname(me);
    	        from.host = addr_num_to_ip_str(conn_get_addr(me));
    	        from.user = ctag;
        	    msg = irc_message_preformat(&from,"NOTICE","",temp);
	    }
	    else
	    {
	            msg = irc_message_preformat(NULL,"NOTICE","",temp);
	    }
	}
	break;
    case message_type_namreply:
        t_channel * channel;

        channel = conn_get_channel(me);

        irc_send_rpl_namreply(dst,channel);
	break;

   	/**
   	*  Westwood Online Extensions
   	*/
    case message_type_host:
	    from.nick = conn_get_chatname(me);
	    from.user = ctag;
	    from.host = addr_num_to_ip_str(conn_get_addr(me));
	    msg = irc_message_preformat(&from,"HOST","\r",text);
	    conn_unget_chatname(me,from.nick);
    	break;
   	case message_type_page:
    {
   	    char temp[MAX_IRC_MESSAGE_LEN];
	    from.nick = conn_get_chatname(me);
	    from.user = ctag;
	    from.host = addr_num_to_ip_str(conn_get_addr(me));
	    std::sprintf(temp,":%s",text);
        msg = irc_message_preformat(&from,"PAGE",NULL,temp);
	    conn_unget_chatname(me,from.nick);
    	break;
    }
    case message_wol_joingame:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
     	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"JOINGAME","\r",text);
    	conn_unget_chatname(me,from.nick);
    	break;
    case message_wol_gameopt_owner:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"GAMEOPT",irc_convert_channel(conn_get_channel(me)),text);
    	conn_unget_chatname(me,from.nick);
    	break;
    case message_wol_gameopt_join:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"GAMEOPT",channel_wol_get_game_owner(conn_get_channel(me)),text);
    	conn_unget_chatname(me,from.nick);
    	break;
    case message_wol_start_game:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"STARTG","u",text);
    	conn_unget_chatname(me,from.nick);
    	break;
    case message_wol_advertr:
    	msg = irc_message_preformat(NULL,"ADVERTR","\r",text);
    	break;
   	case message_wol_chanchk:
        msg = irc_message_preformat(NULL,"CHANCHK","\r",text);
        break;
   	case message_wol_kick:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"KICK","\r",text);
    	conn_unget_chatname(me,from.nick);
        break;
   	case message_wol_userip:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"USERIP","\r",text);
    	conn_unget_chatname(me,from.nick);
        break;
    default:
    	eventlog(eventlog_level_warn,__FUNCTION__,"%d not yet implemented",type);
	return -1;
    }

    if (msg) {
	packet_append_string(packet,msg);
	xfree(msg);
        return 0;
    }
    return -1;
}

int irc_send_rpl_namreply_internal(t_connection * c, t_channel const * channel){
    char temp[MAX_IRC_MESSAGE_LEN];
    char const * ircname;
    int first = 1;
    t_connection * m;

    if (!channel) {
        eventlog(eventlog_level_error,__FUNCTION__,"got NULL channel");
	return -1;
    }

    std::memset(temp,0,sizeof(temp));
    ircname = irc_convert_channel(channel);
    if (!ircname) {
	eventlog(eventlog_level_error,__FUNCTION__,"channel has NULL ircname");
	return -1;
    }
    /* '@' = secret; '*' = private; '=' = public */
    if ((1+1+std::strlen(ircname)+2+1)<=MAX_IRC_MESSAGE_LEN) {
	std::sprintf(temp,"%c %s :",((channel_get_permanent(channel))?('='):('*')),ircname);
    } else {
	eventlog(eventlog_level_warn,__FUNCTION__,"maximum message length exceeded");
	return -1;
    }

    /* FIXME: Add per user flags (@(op) and +(voice))*/
    if (!(channel_get_flags(channel) & channel_flags_thevoid))
    for (m = channel_get_first(channel);m;m = channel_get_next()) {
	char const * name = conn_get_chatname(m);
	char flg[5] = "";
	unsigned int flags;

	if (!name)
	    continue;
	flags = conn_get_flags(m);
	if (flags & MF_BLIZZARD)
		std::strcat(flg,"@");
	else if ((flags & MF_BNET) || (flags & MF_GAVEL))
		std::strcat(flg,"%");
	else if (flags & MF_VOICE)
		std::strcat(flg,"+");
	if ((std::strlen(temp)+((!first)?(1):(0))+std::strlen(flg)+std::strlen(name)+1)<=sizeof(temp)) {
	    if (!first) std::strcat(temp," ");

            if((conn_get_wol(c) == 1)) {
                if ((channel_wol_get_game_owner(channel) != NULL) && (std::strcmp(channel_wol_get_game_owner(channel),name) == 0)) {
                       std::strcat(temp,"@");
                    }
                else if ((conn_wol_get_ingame(c) == 0)) {
                   	if (flags & MF_BLIZZARD)
		               std::strcat(temp,"@");
                }
                if (conn_get_clienttag(c) != CLIENTTAG_WCHAT_UINT) {
                    /* BATTLECLAN Support */
                    t_clan * clan = account_get_clan(conn_get_account(m));
                    unsigned int clanid = 0;

                    if (clan)
                        clanid = clan_get_clanid(clan);

                    std::sprintf(temp,"%s%s,%u,%u",temp,name,clanid,conn_get_addr(m));
                }
                else {
                    std::sprintf(temp,"%s%s",temp,name);
                }
    	    }
    	    else
    	    {
	            std::strcat(temp,flg);
	            std::strcat(temp,name);
    	    }

	    first = 0;
	}
	conn_unget_chatname(m,name);
    }
    irc_send(c,RPL_NAMREPLY,temp);
}

extern int irc_send_rpl_namreply(t_connection * c, t_channel const * channel)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    char const * ircname;

    if (!c) {
        eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }

    if (channel) {
        ircname = irc_convert_channel(channel);
        if (!ircname) {
	    eventlog(eventlog_level_error,__FUNCTION__,"channel has NULL ircname");
	    return -1;
        }
        irc_send_rpl_namreply_internal(c, channel);
        std:sprintf(temp, "%.32s :End of NAMES list", ircname);
    } else {
        t_elem const * curr;
        LIST_TRAVERSE_CONST(channellist(),curr)
        {
            channel = (t_channel*)elem_get_data(curr);
            irc_send_rpl_namreply_internal(c, channel);
        }
	std::sprintf(temp, "* :End of NAMES list");
    }

    irc_send(c, RPL_ENDOFNAMES, temp);
    return 0;
}

static int irc_who_connection(t_connection * dest, t_connection * c)
{
    t_account * a;
    char const * tempuser;
    char const * tempowner;
    char const * tempname;
    char const * tempip;
    char const * tempflags = "@"; /* FIXME: that's dumb */
    char temp[MAX_IRC_MESSAGE_LEN];
    char const * tempchannel;

    if (!dest) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL destination");
	return -1;
    }
    if (!c) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    a = conn_get_account(c);
    if (!(tempuser = clienttag_uint_to_str(conn_get_clienttag(c))))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL clienttag (tempuser)");
	return -1;
    }
    if (!(tempowner = account_get_ll_owner(a)))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ll_owner (tempowner)");
	return -1;
    }
    if (!(tempname = conn_get_username(c)))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL username (tempname)");
	return -1;
    }
    if (!(tempip = addr_num_to_ip_str(conn_get_addr(c))))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL addr (tempip)");
	return -1;
    }
    if (!(tempchannel = irc_convert_channel(conn_get_channel(c))))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL channel (tempchannel)");
	return -1;
    }
    if ((std::strlen(tempchannel)+1+std::strlen(tempuser)+1+std::strlen(tempip)+1+std::strlen(server_get_hostname())+1+std::strlen(tempname)+1+1+std::strlen(tempflags)+4+std::strlen(tempowner)+1)>MAX_IRC_MESSAGE_LEN) {
	eventlog(eventlog_level_info,__FUNCTION__,"WHO reply too long - skip");
	return -1;
    } else
        std::sprintf(temp,"%s %s %s %s %s %c%s :0 %s",tempchannel,tempuser,tempip,server_get_hostname(),tempname,'H',tempflags,tempowner);
    irc_send(dest,RPL_WHOREPLY,temp);
    return 0;
}

extern int irc_who(t_connection * c, char const * name)
{
    /* FIXME: support wildcards! */

    if (!c) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL name");
	return -1;
    }
    if ((name[0]=='#')||(name[0]=='&')||(name[0]=='!')) {
	/* it's a channel */
	t_connection * info;
	t_channel * channel;
	char const * ircname;

	ircname = irc_convert_ircname(name);
	channel = channellist_find_channel_by_name(ircname,NULL,NULL);
	if (!channel) {
	    char temp[MAX_IRC_MESSAGE_LEN];

	    if ((std::strlen(":No such channel")+1+std::strlen(name)+1)<=MAX_IRC_MESSAGE_LEN) {
		std::sprintf(temp,":No such channel %s",name);
		irc_send(c,ERR_NOSUCHCHANNEL,temp);
	    } else {
		irc_send(c,ERR_NOSUCHCHANNEL,":No such channel");
	    }
	    return 0;
	}
	for (info = channel_get_first(channel);info;info = channel_get_next()) {
	    irc_who_connection(c,info);
	}
    } else {
	/* it's just one user */
	t_connection * info;

	if ((info = connlist_find_connection_by_accountname(name)))
	    return irc_who_connection(c,info);
    }
    return 0;
}

extern int irc_send_motd(t_connection * conn)
{
    char const * tempname;
    char const * filename;
    std::FILE *fp;
    char * line, * formatted_line;
    char send_line[MAX_IRC_MESSAGE_LEN];
    char motd_failed = 0;

    if (!conn) {
	   eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	   return -1;
    }

    tempname = conn_get_loggeduser(conn);

    irc_send(conn,RPL_MOTDSTART,":-");

    if ((filename = prefs_get_motdfile())) {
	 if ((fp = std::fopen(filename,"r"))) {
	  while ((line=file_get_line(fp))) {
		if ((formatted_line = message_format_line(conn,line))) {
		  formatted_line[0]=' ';
		  std::sprintf(send_line,":-%s",formatted_line);
		  irc_send(conn,RPL_MOTD,send_line);
		  xfree(formatted_line);
		}
	  }

	  file_get_line(NULL); // clear file_get_line buffer
	  std::fclose(fp);
	}
	 else
	 	motd_failed = 1;
   }
   else
      motd_failed = 1;

   if (motd_failed) {
      irc_send(conn,RPL_MOTD,":- Failed to load motd, sending default motd              ");
      irc_send(conn,RPL_MOTD,":- ====================================================== ");
      irc_send(conn,RPL_MOTD,":-                 http://www.pvpgn.org                   ");
      irc_send(conn,RPL_MOTD,":- ====================================================== ");
   }
   irc_send(conn,RPL_ENDOFMOTD,":End of /MOTD command");
   return 0;
}

int irc_welcome(t_connection * conn){
    if (conn_get_wol(conn))
       handle_wol_welcome(conn);
    else
       handle_irc_welcome(conn);

    return 0;
}

extern int _handle_nick_command(t_connection * conn, int numparams, char ** params, char * text)
{
    char temp[MAX_IRC_MESSAGE_LEN];
	/* FIXME: more strict param checking */

	if ((conn_get_loggeduser(conn))&&
	    (conn_get_state(conn)!=conn_state_bot_password &&
	     conn_get_state(conn)!=conn_state_bot_username)) {
	    irc_send(conn,ERR_RESTRICTED,":You can't change your nick after login");
	}
	else {
	    if ((params)&&(params[0])) {
			if (conn_get_loggeduser(conn)){
                snprintf(temp, sizeof(temp),":%s",params[0]);
			    message_send_text(conn,message_type_nick,conn,temp);
             }
			conn_set_loggeduser(conn,params[0]);
	    }
	    else if (text) {
			if (conn_get_loggeduser(conn)){
                snprintf(temp, sizeof(temp),":%s",text);
			    message_send_text(conn,message_type_nick,conn,temp);
             }
			conn_set_loggeduser(conn,text);
	    }
	    else
	        irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to NICK");

	    if ((conn_get_user(conn))&&(conn_get_loggeduser(conn)))
			irc_welcome(conn); /* only send the welcome if we have USER and NICK */
	}
	return 0;
}

extern int _handle_topic_command(t_connection * conn, int numparams, char ** params, char * text)
{
	char ** e = NULL;

	if((conn_get_wol(conn) == 1)) {
	    t_channel * channel = conn_get_channel(conn);
	    channel_set_topic(channel_get_name(channel),text,NO_SAVE_TOPIC);
	}

	if (params!=NULL) e = irc_get_listelems(params[0]);

	if ((e)&&(e[0])) {
		t_channel *channel = conn_get_channel(conn);

		if (channel) {
			char * topic;
			char temp[MAX_IRC_MESSAGE_LEN];
			char const * ircname = irc_convert_ircname(e[0]);

			if ((ircname) && (strcasecmp(channel_get_name(channel),ircname)==0)) {
				if ((topic = channel_get_topic(channel_get_name(channel)))) {
			  		snprintf(temp, sizeof(temp), "%s :%s", irc_convert_channel(channel), topic);
			    		irc_send(conn,RPL_TOPIC,temp);
				}
				else
			    		irc_send(conn,RPL_NOTOPIC,":No topic is set");
			}
			else
				irc_send(conn,ERR_NOTONCHANNEL,":You are not on that channel");
		}
		else {
			irc_send(conn,ERR_NOTONCHANNEL,":You're not on a channel");
		}
		irc_unget_listelems(e);
	}
	else
		irc_send(conn,ERR_NEEDMOREPARAMS,":too few arguments to TOPIC");
	return 0;
}

extern int _handle_names_command(t_connection * conn, int numparams, char ** params, char * text)
{
	t_channel * channel;

    if (numparams>=1) {
		char ** e;
		char const * ircname;
		char const * verytemp;
		char temp[MAX_IRC_MESSAGE_LEN];
		int i;

		e = irc_get_listelems(params[0]);
		for (i=0;((e)&&(e[i]));i++) {
			verytemp = irc_convert_ircname(e[i]);

			if (!verytemp)
				continue; /* something is wrong with the name ... */
			channel = channellist_find_channel_by_name(verytemp,NULL,NULL);
			if (!channel)
				continue; /* channel doesn't exist */
			irc_send_rpl_namreply(conn,channel);
		}
		if (e)
		irc_unget_listelems(e);
    }
	else if (numparams==0) {
		irc_send_rpl_namreply(conn, NULL);
    }
	return 0;
}

}

}
