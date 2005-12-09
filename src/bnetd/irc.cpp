/*
 * Copyright (C) 2001  Marco Ziech (mmz@gmx.net)
 * Copyright (C) 2005  Bryan Biedenkapp (gatekeep@gmail.com)
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
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#endif
#include "compat/strdup.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "common/irc_protocol.h"
#include "common/packet.h"
#include "common/eventlog.h"
#include "connection.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/addr.h"
#include "common/version.h"
#include "common/queue.h"
#include "common/list.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "common/tag.h"
#include "message.h"
#include "account.h"
#include "account_wrap.h"
#include "channel.h"
#include "irc.h"
#include "prefs.h"
#include "server.h"
#include "tick.h"
#include "message.h"
#include "command_groups.h"
#include "common/util.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

namespace pvpgn
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
    int len;
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
        len = 1+strlen(ircname)+1+strlen(command)+1+strlen(nick)+1+strlen(params)+2;
	if (len > MAX_IRC_MESSAGE_LEN) {
	    eventlog(eventlog_level_error,__FUNCTION__,"message to send is too large (%d bytes)",len);
	    return -1;
	}
	else
	    sprintf(data,":%s %s %s %s\r\n",ircname,command,nick,params);
    } else {
        len = 1+strlen(ircname)+1+strlen(command)+1+strlen(nick)+1+2;
    	if (len > MAX_IRC_MESSAGE_LEN) {
	    eventlog(eventlog_level_error,__FUNCTION__,"message to send is too large (%d bytes)",len);
	    return -1;
	}
	else
	sprintf(data,":%s %s %s\r\n",ircname,command,nick);
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
    sprintf(temp,"%03u",code);
    return irc_send_cmd(conn,temp,params);
}

extern int irc_send_cmd2(t_connection * conn, char const * prefix, char const * command, char const * postfix, char const * comment)
{
    t_packet * p;
    char data[MAX_IRC_MESSAGE_LEN+1];
    int len;

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if (!prefix)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL prefix");
	return -1;
    }
    if (!command) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL command");
	return -1;
    }
    if (!postfix)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL postfix");
	return -1;
    }

    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create packet");
	return -1;
    }

    if (comment) {
        len = 1+strlen(prefix)+1+strlen(command)+1+strlen(postfix)+2+strlen(comment)+1+2;
    	if (len > MAX_IRC_MESSAGE_LEN) {
	    eventlog(eventlog_level_error,__FUNCTION__,"message to send is too large (%d bytes)",len);
	    return -1;
	}
	else
	    sprintf(data,":%s %s %s :%s\r\n",prefix,command,postfix,comment);
    } else {
        len = 1+strlen(prefix)+1+strlen(command)+1+strlen(postfix)+1+2;
    	if (len > MAX_IRC_MESSAGE_LEN) {
	    eventlog(eventlog_level_error,__FUNCTION__,"message to send is too large (%d bytes)",len);
	    return -1;
	}
	else
	sprintf(data,":%s %s %s\r\n",prefix,command,postfix);
    }
    packet_set_size(p,0);
    packet_append_data(p,data,len);
    // eventlog(eventlog_level_debug,__FUNCTION__,"[%d] sent \"%s\"",conn_get_socket(conn),data);
    conn_push_outqueue(conn,p);
    packet_del_ref(p);
    return 0;
}

extern int irc_send_ping(t_connection * conn)
{
    t_packet * p;
    char data[MAX_IRC_MESSAGE_LEN];

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create packet");
	return -1;
    }

    if((conn_get_wol(conn) == 1))
        return 0;

    conn_set_ircping(conn,get_ticks());
    if (conn_get_state(conn)==conn_state_bot_username)
    	sprintf(data,"PING :%u\r\n",conn_get_ircping(conn)); /* Undernet doesn't reveal the servername yet ... neither do we */
    else if ((6+strlen(server_get_hostname())+2+1)<=MAX_IRC_MESSAGE_LEN)
    	sprintf(data,"PING :%s\r\n",server_get_hostname());
    else
    	eventlog(eventlog_level_error,__FUNCTION__,"maximum message length exceeded");
    eventlog(eventlog_level_debug,__FUNCTION__,"[%d] sent \"%s\"",conn_get_socket(conn),data);
    packet_set_size(p,0);
    packet_append_data(p,data,strlen(data));
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
    if ((1+strlen(server_get_hostname())+1+4+1+strlen(server_get_hostname())+((params)?(2+strlen(params)):(0))+2+1) > MAX_IRC_MESSAGE_LEN) {
	eventlog(eventlog_level_error,__FUNCTION__,"max message length exceeded");
	return -1;
    }
    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create packet");
	return -1;
    }

    if (params)
    	sprintf(data,":%s PONG %s :%s\r\n",server_get_hostname(),server_get_hostname(),params);
    else
    	sprintf(data,":%s PONG %s\r\n",server_get_hostname(),server_get_hostname());
    eventlog(eventlog_level_debug,__FUNCTION__,"[%d] sent \"%s\"",conn_get_socket(conn),data);
    packet_set_size(p,0);
    packet_append_data(p,data,strlen(data));
    conn_push_outqueue(conn,p);
    packet_del_ref(p);
    return 0;
}

extern int irc_authenticate(t_connection * conn, char const * passhash)
{
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
    	irc_send_cmd(conn,"NOTICE",":Authentication failed."); /* user does not exist */
	return 0;
    }

    if (connlist_find_connection_by_account(a) && prefs_get_kick_old_login()==0) {
            irc_send_cmd(conn,"NOTICE",":Authentication rejected (already logged in) ");
    }
    else if (account_get_auth_lock(a)==1) {
            irc_send_cmd(conn,"NOTICE",":Authentication rejected (account is locked) ");
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
                irc_send_cmd(conn,"NOTICE",":Authentication failed."); /* bad APGAR */
                conn_increment_passfail_count(conn);
                return 0;
            }

    	    if(strcmp(temphash,tempapgar) == 0) {
                conn_login(conn,a,username);
    	        conn_set_state(conn,conn_state_loggedin);
        	    conn_set_clienttag(conn,CLIENTTAG_WWOL_UINT); /* WWOL hope here is ok */
        		return 1;
    	    }
    	    else {
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
            irc_send_cmd(conn,"NOTICE",":Authentication successful. You are now logged in.");
	    return 1;
        } else {
            irc_send_cmd(conn,"NOTICE",":Authentication failed."); /* wrong password */
	    conn_increment_passfail_count(conn);
        }
    }
    return 0;
}

extern int irc_welcome(t_connection * conn)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    time_t temptime;
    char const * tempname;
    char const * temptimestr;
    char const * filename;
    FILE *fp;
    char * line, * formatted_line;
    char send_line[MAX_IRC_MESSAGE_LEN];
    char motd_failed = 0;

    if (!conn) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }

    tempname = conn_get_loggeduser(conn);

    if ((34+strlen(tempname)+1)<=MAX_IRC_MESSAGE_LEN)
        sprintf(temp,":Welcome to the %s IRC Network %s",prefs_get_irc_network_name(), tempname);
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_WELCOME,temp);

    if ((14+strlen(server_get_hostname())+10+strlen(PVPGN_SOFTWARE" "PVPGN_VERSION)+1)<=MAX_IRC_MESSAGE_LEN)
        sprintf(temp,":Your host is %s, running "PVPGN_SOFTWARE" "PVPGN_VERSION,server_get_hostname());
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_YOURHOST,temp);

    temptime = server_get_starttime(); /* FIXME: This should be build time */
    temptimestr = ctime(&temptime);
    if ((25+strlen(temptimestr)+1)<=MAX_IRC_MESSAGE_LEN)
        sprintf(temp,":This server was created %s",temptimestr); /* FIXME: is ctime() portable? */
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_CREATED,temp);

    /* we don't give mode information on MYINFO we give it on ISUPPORT */
    if ((strlen(server_get_hostname())+7+strlen(PVPGN_SOFTWARE" "PVPGN_VERSION)+9+1)<=MAX_IRC_MESSAGE_LEN)
        sprintf(temp,"%s "PVPGN_SOFTWARE" "PVPGN_VERSION" - -",server_get_hostname());
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_MYINFO,temp);

    if((conn_get_wol(conn) == 1))
        sprintf(temp,"NICKLEN=%d TOPICLEN=%d CHANNELLEN=%d PREFIX="CHANNEL_PREFIX" CHANTYPES="CHANNEL_TYPE" NETWORK=%s IRCD="PVPGN_SOFTWARE,
        WOL_NICKNAME_LEN, MAX_TOPIC_LEN, CHANNEL_NAME_LEN, prefs_get_irc_network_name());
    else
        sprintf(temp,"NICKLEN=%d TOPICLEN=%d CHANNELLEN=%d PREFIX="CHANNEL_PREFIX" CHANTYPES="CHANNEL_TYPE" NETWORK=%s IRCD="PVPGN_SOFTWARE,
        CHAR_NAME_LEN, MAX_TOPIC_LEN, CHANNEL_NAME_LEN, prefs_get_irc_network_name());

    if((strlen(temp))<=MAX_IRC_MESSAGE_LEN)
        irc_send(conn,RPL_ISUPPORT,temp);
    else {
        sprintf(temp,":Maximum length exceeded");
        irc_send(conn,RPL_ISUPPORT,temp);
    }

    if ((3+strlen(server_get_hostname())+22+1)<=MAX_IRC_MESSAGE_LEN)
    	sprintf(temp,":- %s, "PVPGN_SOFTWARE" "PVPGN_VERSION", built on %s",server_get_hostname(),temptimestr);
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_MOTDSTART,temp);

    if ((filename = prefs_get_motdfile())) {
	 if ((fp = fopen(filename,"r"))) {
	  while ((line=file_get_line(fp))) {
		if ((formatted_line = message_format_line(conn,line))) {
		  formatted_line[0]=' ';
		  sprintf(send_line,":-%s",formatted_line);
		  irc_send(conn,RPL_MOTD,send_line);
		  xfree(formatted_line);
		}
	  }

	  file_get_line(NULL); // clear file_get_line buffer
	  fclose(fp);
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
    irc_send_cmd(conn,"NOTICE",":This is an experimental service.");

    conn_set_state(conn,conn_state_bot_password);
    if (connlist_find_connection_by_accountname(conn_get_loggeduser(conn))) {
    	irc_send_cmd(conn,"NOTICE","This account is already logged in, use another account.");
	return -1;
    }

    if (conn_get_ircpass(conn)) {
	irc_send_cmd(conn,"NOTICE",":Trying to authenticate with PASS ...");
	irc_authenticate(conn,conn_get_ircpass(conn));
    } else {
    	irc_send_cmd(conn,"NOTICE",":No PASS command received. Please identify yourself by /msg NICKSERV identify <password>.");
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
    static char out[CHANNEL_NAME_LEN];
    unsigned int outpos;
    int i;

    if (!channel)
	return "*";

    memset(out,0,sizeof(out));
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
	    sprintf(out,"!%u",channel_get_channelid(channel));
	    return out;
	}
    }
    return out;
}

extern char const * irc_convert_ircname(char const * pircname)
{
    static char out[CHANNEL_NAME_LEN];
    unsigned int outpos;
    int special;
    int i;
    char const * ircname = pircname + 1;

    if (!ircname) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ircname");
	return NULL;
    }

    outpos = 0;
    memset(out,0,sizeof(out));
    special = 0;
    if (pircname[0]=='!') {
	t_channel * channel;

	channel = channellist_find_channel_bychannelid(atoi(ircname));
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
	    out[i] = strchr(out[i-1],separator);
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
	myfrom = (char*)xmalloc(strlen(from->nick)+1+strlen(from->user)+1+strlen(from->host)+1); /* nick + "!" + user + "@" + host + "\0" */
	sprintf(myfrom,"%s!%s@%s",from->nick,from->user,from->host);
    } else
    	myfrom = xstrdup(server_get_hostname());
    if (dest)
    	mydest = dest;
    if (text)
    	mytext = text;

    len = 1+strlen(myfrom)+1+
    	  strlen(command)+1+
    	  strlen(mydest)+1+
    	  1+strlen(mytext)+1;


    msg = (char*)xmalloc(len);
    sprintf(msg,":%s\n%s\n%s\n%s",myfrom,command,mydest,mytext);
    xfree(myfrom);
    return msg;
}

extern int irc_message_postformat(t_packet * packet, t_connection const * dest)
{
    int len;
    /* the four elements */
    char * e1;
    char * e1_2;
    char * e2;
    char * e3;
    char * e4;
    char const * tname = NULL;
    char const * toname = "AUTH"; /* fallback name */

    if (!packet) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL packet");
	return -1;
    }
    if (!dest) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL dest");
	return -1;
    }

    e1 = (char*)packet_get_raw_data(packet,0);
    e2 = strchr(e1,'\n');
    if (!e2) {
	eventlog(eventlog_level_warn,__FUNCTION__,"malformed message (e2 missing)");
	return -1;
    }
    *e2++ = '\0';
    e3 = strchr(e2,'\n');
    if (!e3) {
	eventlog(eventlog_level_warn,__FUNCTION__,"malformed message (e3 missing)");
	return -1;
    }
    *e3++ = '\0';
    e4 = strchr(e3,'\n');
    if (!e4) {
	eventlog(eventlog_level_warn,__FUNCTION__,"malformed message (e4 missing)");
	return -1;
    }
    *e4++ = '\0';

    if (prefs_get_hide_addr() && !(account_get_command_groups(conn_get_account(dest)) & command_get_group("/admin-addr")))
    {
      e1_2 = strchr(e1,'@');
      if (e1_2)
      {
	  *e1_2++ = '\0';
      }
    }
    else
    e1_2 = NULL;

    if (e3[0]=='\0') { /* fill in recipient */
    	if ((tname = conn_get_chatname(dest)))
    	    toname = tname;
    } else
    	toname = e3;

    if (strcmp(toname,"\r")==0) {
	toname = ""; /* HACK: the target field is really empty */
    }

    len = (strlen(e1)+1+strlen(e2)+1+strlen(toname)+1+strlen(e4)+2+1);
    if (len<=MAX_IRC_MESSAGE_LEN) {
	char msg[MAX_IRC_MESSAGE_LEN+1];

	if (e1_2)
	    sprintf(msg,"%s@hidden %s %s %s\r\n",e1,e2,toname,e4);
	else
	    sprintf(msg,"%s %s %s %s\r\n",e1,e2,toname,e4);
	eventlog(eventlog_level_debug,__FUNCTION__,"sent \"%s\"",msg);
	packet_set_size(packet,0);
	packet_append_data(packet,msg,strlen(msg));
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
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));

	    if((conn_get_wol(me) == 1))
	    {
        	char temp[MAX_IRC_MESSAGE_LEN];
    		memset(temp,0,sizeof(temp));

    		/**
            *  For WOL the channel JOIN output must be like the following:
    		*   user!WWOL@hostname JOIN :clanID,longIP channelName
    		*/
    		sprintf(temp,":0,%u",conn_get_addr(me));
    		msg = irc_message_preformat(&from,"JOIN",temp,irc_convert_channel(conn_get_channel(me)));
	    }
	    else
    	msg = irc_message_preformat(&from,"JOIN","\r",irc_convert_channel(conn_get_channel(me)));
    	conn_unget_chatname(me,from.nick);
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

	    if (me)
	    {
    	        from.nick = conn_get_chatname(me);
    	        from.host = addr_num_to_ip_str(conn_get_addr(me));
	    }
	    else
	    {
		from.nick = server_get_hostname();
		from.host = server_get_hostname();
	    }

            from.user = ctag;

    	    if (type==message_type_talk)
    	    	dest = irc_convert_channel(conn_get_channel(me)); /* FIXME: support more channels and choose right one! */
	    else
	        dest = ""; /* will be replaced with username in postformat */
	    sprintf(temp,":%s",text);
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
	    if ((8+strlen(text)+1+1)<=MAX_IRC_MESSAGE_LEN) {
		sprintf(temp,":\001ACTION %s\001",text);
	    } else {
		sprintf(temp,":\001ACTION (maximum message length exceeded)\001");
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
	    sprintf(temp,":%s",text);
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
   	/**
   	*  Westwood Online Extensions
   	*/
    case message_wol_joingame:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"JOINGAME",text,"\r");
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
    case message_wol_page:
    	from.nick = conn_get_chatname(me);
    	from.user = ctag;
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"PAGE","u",text);
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

extern int irc_send_rpl_namreply(t_connection * c, t_channel const * channel)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    char const * ircname;
    int first = 1;
    t_connection * m;

    if (!c) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if (!channel) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL channel");
	return -1;
    }
    memset(temp,0,sizeof(temp));
    ircname = irc_convert_channel(channel);
    if (!ircname) {
	eventlog(eventlog_level_error,__FUNCTION__,"channel has NULL ircname");
	return -1;
    }
    /* '@' = secret; '*' = private; '=' = public */
    if ((1+1+strlen(ircname)+2+1)<=MAX_IRC_MESSAGE_LEN) {
	sprintf(temp,"%c %s :",((channel_get_permanent(channel))?('='):('*')),ircname);
    } else {
	eventlog(eventlog_level_warn,__FUNCTION__,"maximum message length exceeded");
	return -1;
    }
    /* FIXME: Add per user flags (@(op) and +(voice))*/
    for (m = channel_get_first(channel);m;m = channel_get_next()) {
	char const * name = conn_get_chatname(m);
	char flg[5] = "";
	unsigned int flags;

	if (!name)
	    continue;
	flags = conn_get_flags(m);
	if (flags & MF_BLIZZARD)
		strcat(flg,"@");
	else if ((flags & MF_BNET) || (flags & MF_GAVEL))
		strcat(flg,"%");
	else if (flags & MF_VOICE)
		strcat(flg,"+");
	if ((strlen(temp)+((!first)?(1):(0))+strlen(flg)+strlen(name)+1)<=sizeof(temp)) {
	    if (!first) strcat(temp," ");

    	    if((conn_get_wol(c) == 1))
    	    {
        		if((conn_wol_get_ingame(c) == 0))
        		{
                    if ((flags & MF_BLIZZARD))
        			   strcat(temp,"@");
        		    if ((flags & MF_BNET) || (flags & MF_GAVEL))
        			   strcat(temp,"@");
        		}
                sprintf(temp,"%s%s,0,%u",temp,name,conn_get_addr(m));
    	    }
    	    else
    	    {
	    strcat(temp,flg);
	    strcat(temp,name);
    	    }

	    first = 0;
	}
	conn_unget_chatname(m,name);
    }
    irc_send(c,RPL_NAMREPLY,temp);
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
    if ((strlen(tempchannel)+1+strlen(tempuser)+1+strlen(tempip)+1+strlen(server_get_hostname())+1+strlen(tempname)+1+1+strlen(tempflags)+4+strlen(tempowner)+1)>MAX_IRC_MESSAGE_LEN) {
	eventlog(eventlog_level_info,__FUNCTION__,"WHO reply too long - skip");
	return -1;
    } else
        sprintf(temp,"%s %s %s %s %s %c%s :0 %s",tempchannel,tempuser,tempip,server_get_hostname(),tempname,'H',tempflags,tempowner);
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

	    if ((strlen(":No such channel")+1+strlen(name)+1)<=MAX_IRC_MESSAGE_LEN) {
		sprintf(temp,":No such channel %s",name);
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

}
