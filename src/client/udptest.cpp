/*
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_MEMORY_H
# include <memory.h>
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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#include "compat/recv.h"
#include "compat/send.h"
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#include "compat/psock.h"
#include "common/packet.h"
#include "common/init_protocol.h"
#include "common/udp_protocol.h"
#include "common/tag.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/network.h"
#include "client.h"
#include "udptest.h"
#include "common/setup_after.h"


#ifdef CLIENTDEBUG
#define dprintf printf
#else
#define dprintf if (0) printf
#endif


extern int client_udptest_setup(char const * progname, unsigned short * lsock_port_ret)
{
    int                lsock;
    struct sockaddr_in laddr;
    unsigned short     lsock_port;
    
    if (!progname)
    {
	fprintf(stderr,"got NULL progname\n");
	return -1;
    }
    
    if ((lsock = psock_socket(PF_INET,SOCK_DGRAM,PSOCK_IPPROTO_UDP))<0)
    {
	fprintf(stderr,"%s: could not create UDP socket (psock_socket: %s)\n",progname,pstrerror(psock_errno()));
	return -1;
    }
    
    if (psock_ctl(lsock,PSOCK_NONBLOCK)<0)
	fprintf(stderr,"%s: could not set UDP socket to non-blocking mode (psock_ctl: %s)\n",progname,pstrerror(psock_errno()));
    
    for (lsock_port=BNETD_MIN_TEST_PORT; lsock_port<=BNETD_MAX_TEST_PORT; lsock_port++)
    {
	memset(&laddr,0,sizeof(laddr));
	laddr.sin_family = PSOCK_AF_INET;
	laddr.sin_port = htons(lsock_port);
	laddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (psock_bind(lsock,(struct sockaddr *)&laddr,(psock_t_socklen)sizeof(laddr))==0)
	    break;
	
	if (lsock_port==BNETD_MIN_TEST_PORT)
	    dprintf("Could not bind to standard UDP port %hu, trying others. (psock_bind: %s)\n",BNETD_MIN_TEST_PORT,pstrerror(psock_errno()));
    }
    if (lsock_port>BNETD_MAX_TEST_PORT)
    {
	fprintf(stderr,"%s: could not bind to any UDP port %hu through %hu (psock_bind: %s)\n",progname,BNETD_MIN_TEST_PORT,BNETD_MAX_TEST_PORT,pstrerror(psock_errno()));
	psock_close(lsock);
	return -1;
    }
    
    if (lsock_port_ret)
	*lsock_port_ret = lsock_port;
    
    return lsock;
}


extern int client_udptest_recv(char const * progname, int lsock, unsigned short lsock_port, unsigned int timeout)
{
    int          len;
    unsigned int count;
    t_packet *   rpacket;
    time_t       start;
    
    if (!progname)
    {
	fprintf(stderr,"%s: got NULL progname\n",progname);
	return -1;
    }
    
    if (!(rpacket = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	return -1;
    }
    
    start = time(NULL);
    count = 0;
    while (start+(time_t)timeout>=time(NULL))  /* timeout after a few seconds from last packet */
    {
	if ((len = psock_recv(lsock,packet_get_raw_data_build(rpacket,0),MAX_PACKET_SIZE,0))<0)
	{
	    if (psock_errno()!=PSOCK_EAGAIN && psock_errno()!=PSOCK_EWOULDBLOCK)
		fprintf(stderr,"%s: failed to receive UDPTEST on port %hu (psock_recv: %s)\n",progname,lsock_port,pstrerror(psock_errno()));
	    continue;
	}
	packet_set_size(rpacket,len);
	
	if (packet_get_type(rpacket)!=SERVER_UDPTEST)
	{
	    dprintf("Got unexpected UDP packet type %u on port %hu\n",packet_get_type(rpacket),lsock_port);
	    continue;
	}
	
	if (bn_int_tag_eq(rpacket->u.server_udptest.bnettag,BNETTAG)<0)
	{
	    fprintf(stderr,"%s: got bad UDPTEST packet on port %hu\n",progname,lsock_port);
	    continue;
	}
	
	count++;
	if (count>=2)
	    break;
	
	start = time(NULL);
    }
    
    packet_destroy(rpacket);
    
    if (count<2)
    {
	printf("Only received %d UDP packets on port %hu. Connection may be slow or firewalled.\n",count,lsock_port);
	return -1;
    }
    
    return 0;
}
