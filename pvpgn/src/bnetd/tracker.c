/*
 * Copyright (C) 1999  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#define TRACKER_INTERNAL_ACCESS
#define SERVER_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
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
#include "compat/memset.h"
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
#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif
#include "compat/uname.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#include "compat/send.h"
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#include "compat/psock.h"
#include "common/eventlog.h"
#include "connection.h"
#include "channel.h"
#include "prefs.h"
#include "game.h"
#include "common/version.h"
#include "server.h"
#include "common/addr.h"
#include "common/list.h"
#include "common/tracker.h"
#include "common/setup_after.h"


static t_addrlist * track_servers=NULL;


extern int tracker_set_servers(char const * servers)
{
    t_addr const * addr;
    t_elem const * curr;
    char           temp[32];
    
    if (track_servers && addrlist_destroy(track_servers)<0)
        eventlog(eventlog_level_error,__FUNCTION__,"unable to destroy tracker list");
    
    if (!servers)
     {
	track_servers = NULL;
	return 0;
     }
   
    if (!(track_servers = addrlist_create(servers,INADDR_LOOPBACK,BNETD_TRACK_PORT)))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create tracking server list");
	return -1;
    }
    
    LIST_TRAVERSE_CONST(track_servers,curr)
    {
	addr = (t_addr*)elem_get_data(curr);
	if (!addr_get_addr_str(addr,temp,sizeof(temp)))
	    strcpy(temp,"x.x.x.x:x");
	eventlog(eventlog_level_info,__FUNCTION__,"tracking packets will be sent to %s",temp);
    }
    
    return 0;
}


extern int tracker_send_report(t_addrlist const * laddrs)
{
    t_addr const *     addrl;
    t_elem const *     currl;
    t_addr const *     addrt;
    t_elem const *     currt;
    t_trackpacket      packet;
    struct utsname     utsbuf;
    struct sockaddr_in tempaddr;
    t_laddr_info *     laddr_info;
    char               tempa[64];
    char               tempb[64];
    
    if (addrlist_get_length(track_servers)>0)
    {
	packet.packet_version = htons((unsigned short)TRACK_VERSION);
	/* packet.port is set below */
	packet.flags = 0;
	strncpy(packet.server_location,
		prefs_get_location(),
		sizeof(packet.server_location));
	packet.server_location[sizeof(packet.server_location)-1] = '\0';
	strncpy(packet.software,
		PVPGN_SOFTWARE,
		sizeof(packet.software));
	strncpy(packet.version,
		PVPGN_VERSION,
		sizeof(packet.version));
	strncpy(packet.server_desc,
		prefs_get_description(),
		sizeof(packet.server_desc));
	packet.server_desc[sizeof(packet.server_desc)-1] = '\0';
	strncpy(packet.server_url,
		prefs_get_url(),
		sizeof(packet.server_url));
	packet.server_url[sizeof(packet.server_url)-1] = '\0';
	strncpy(packet.contact_name,
		prefs_get_contact_name(),
		sizeof(packet.contact_name));
	packet.contact_name[sizeof(packet.contact_name)-1] = '\0';
	strncpy(packet.contact_email,
		prefs_get_contact_email(),
		sizeof(packet.contact_email));
	packet.contact_email[sizeof(packet.contact_email)-1] = '\0';
	packet.users = htonl(connlist_login_get_length());
	packet.channels = htonl(channellist_get_length());
	packet.games = htonl(gamelist_get_length());
	packet.uptime = htonl(server_get_uptime());
	packet.total_logins = htonl(connlist_total_logins());
	packet.total_games = htonl(gamelist_total_games());
	
	if (uname(&utsbuf)<0)
	{
	    eventlog(eventlog_level_warn,__FUNCTION__,"could not get platform info (uname: %s)",pstrerror(errno));
	    strncpy(packet.platform,"",sizeof(packet.platform));
	}
	else
	{
	    strncpy(packet.platform,
		    utsbuf.sysname,
		    sizeof(packet.platform));
	    packet.platform[sizeof(packet.platform)-1] = '\0';
	}
	
	LIST_TRAVERSE_CONST(laddrs,currl)
	{
	    addrl = (t_addr*)elem_get_data(currl);
	    
	    if (!(laddr_info = (t_laddr_info*)addr_get_data(addrl).p))
	    {
		eventlog(eventlog_level_error,__FUNCTION__,"address data is NULL");
		continue;
	    }
	    if (laddr_info->type!=laddr_type_bnet)
		continue; /* don't report IRC, telnet, and other non-game ports */
	    
	    packet.port = htons(addr_get_port(addrl));
	    
	    LIST_TRAVERSE_CONST(track_servers,currt)
	    {
		addrt = (t_addr*)elem_get_data(currt);
		
		memset(&tempaddr,0,sizeof(tempaddr));
		tempaddr.sin_family = PSOCK_AF_INET; 
		tempaddr.sin_port = htons(addr_get_port(addrt));
		tempaddr.sin_addr.s_addr = htonl(addr_get_ip(addrt));
		
		if (!addr_get_addr_str(addrl,tempa,sizeof(tempa)))
		    strcpy(tempa,"x.x.x.x:x");
		if (!addr_get_addr_str(addrt,tempb,sizeof(tempb)))
		    strcpy(tempa,"x.x.x.x:x");
		/* eventlog(eventlog_level_debug,__FUNCTION__,"sending tracking info from %s to %s",tempa,tempb); */
		
		if (psock_sendto(laddr_info->usocket,&packet,sizeof(packet),0,(struct sockaddr *)&tempaddr,(psock_t_socklen)sizeof(tempaddr))<0)
		    eventlog(eventlog_level_warn,__FUNCTION__,"could not send tracking information from %s to %s (psock_sendto: %s)",tempa,tempb,pstrerror(errno));
	    }
	}
    }
    
    return 0;
}
