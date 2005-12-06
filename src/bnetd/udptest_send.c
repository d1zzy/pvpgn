/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
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
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h> /* FIXME: probably not needed... do some systems put types in here or something? */
#endif
#include "compat/psock.h"
#include "common/packet.h"
#include "common/bn_type.h"
#include "common/udp_protocol.h"
#include "connection.h"
#include "common/addr.h"
#include "common/tag.h"
#include "common/hexdump.h"
#include "common/eventlog.h"
#include "udptest_send.h"
#include "common/setup_after.h"


extern FILE * hexstrm; /* from main.c */


extern int udptest_send(t_connection const * c)
{
    t_packet *         upacket;
    struct sockaddr_in caddr;
    unsigned int       tries,successes;
    
    memset(&caddr,0,sizeof(caddr));
    caddr.sin_family = PSOCK_AF_INET;
    caddr.sin_port = htons(conn_get_game_port(c));
    caddr.sin_addr.s_addr = htonl(conn_get_game_addr(c));
    
    for (tries=successes=0; successes!=2 && tries<5; tries++)
    {
	if (!(upacket = packet_create(packet_class_udp)))
	{
            eventlog(eventlog_level_error,__FUNCTION__,"[%d] could not allocate memory for packet",conn_get_socket(c));
	    continue;
	}
	packet_set_size(upacket,sizeof(t_server_udptest));
	packet_set_type(upacket,SERVER_UDPTEST);
	bn_int_tag_set(&upacket->u.server_udptest.bnettag,BNETTAG);
	
	if (hexstrm)
	{
	    fprintf(hexstrm,"%d: send class=%s[0x%02x] type=%s[0x%04x] ",
		    conn_get_game_socket(c),
		    packet_get_class_str(upacket),(unsigned int)packet_get_class(upacket),
		    packet_get_type_str(upacket,packet_dir_from_server),packet_get_type(upacket));
	    fprintf(hexstrm,"from=%s ",
		    addr_num_to_addr_str(conn_get_game_addr(c),conn_get_game_port(c)));
	    fprintf(hexstrm,"to=%s ",
		    addr_num_to_addr_str(ntohl(caddr.sin_addr.s_addr),ntohs(caddr.sin_port)));
	    fprintf(hexstrm,"length=%u\n",
		    packet_get_size(upacket));
	    hexdump(hexstrm,packet_get_raw_data(upacket,0),packet_get_size(upacket));
	}
	
        if (psock_sendto(conn_get_game_socket(c),
			 packet_get_raw_data_const(upacket,0),packet_get_size(upacket),
			 0,(struct sockaddr *)&caddr,(psock_t_socklen)sizeof(caddr))!=(int)packet_get_size(upacket))
            eventlog(eventlog_level_error,__FUNCTION__,"[%d] failed to send UDPTEST to %s (attempt %u) (psock_sendto: %s)",conn_get_socket(c),addr_num_to_addr_str(ntohl(caddr.sin_addr.s_addr),conn_get_game_port(c)),tries+1,pstrerror(psock_errno()));
	else
	    successes++;
	
	packet_del_ref(upacket);
    }
    
    if (successes!=2)
	return -1;
    
    return 0;
}
