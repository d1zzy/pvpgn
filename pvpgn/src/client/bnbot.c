/*
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
#include "common/setup_before.h"
#include <stdio.h>
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#include "compat/exitstatus.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif
#include "compat/memset.h"
#include "compat/memcpy.h"
#include <ctype.h>
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
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#include "compat/termios.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
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
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include "compat/inet_ntoa.h"
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#include "compat/psock.h"
#include "common/packet.h"
#include "common/init_protocol.h"
#include "common/bot_protocol.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/network.h"
#include "common/version.h"
#include "common/util.h"
#ifdef CLIENTDEBUG
# include "common/eventlog.h"
#endif
#include "client.h"
#include "common/setup_after.h"


static void usage(char const * progname);


static void usage(char const * progname)
{
    fprintf(stderr,"usage: %s [<options>] [<servername> [<TCP portnumber>]]\n"
            "    -h, --help, --usage         show this information and exit\n"
            "    -v, --version               print version number and exit\n",
	    progname);
    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    int                a;
    int                sd;
    struct sockaddr_in saddr;
    t_packet *         packet;
    t_packet *         rpacket;
    char const *       servname=NULL;
    unsigned short     servport=0;
    char               text[MAX_MESSAGE_LEN];
    struct hostent *   host;
    unsigned int       commpos;
    struct termios     in_attr_old;
    struct termios     in_attr_new;
    int                changed_in;
    unsigned int       currsize;
    int                fd_stdin;
    unsigned int       screen_width,screen_height;
    
    if (argc<1 || !argv || !argv[0])
    {
	fprintf(stderr,"bad arguments\n");
	return STATUS_FAILURE;
    }
    
    for (a=1; a<argc; a++)
	if (servname && isdigit((int)argv[a][0]) && a+1>=argc)
	{
	    if (str_to_ushort(argv[a],&servport)<0)
	    {
		fprintf(stderr,"%s: \"%s\" should be a positive integer\n",argv[0],argv[a]);
		usage(argv[0]);
	    }
	}
	else if (!servname && argv[a][0]!='-' && a+2>=argc)
	    servname = argv[a];
	else if (strcmp(argv[a],"-v")==0 || strcmp(argv[a],"--version")==0)
	{
            printf("version "PVPGN_VERSION"\n");
            return STATUS_SUCCESS;
	}
	else if (strcmp(argv[a],"-h")==0 || strcmp(argv[a],"--help")==0 || strcmp(argv[a],"--usage")==0)
            usage(argv[0]);
	else
	{
	    fprintf(stderr,"%s: unknown option \"%s\"\n",argv[0],argv[a]);
	    usage(argv[0]);
	}
    
    if (servport==0)
        servport = BNETD_SERV_PORT;
    if (!servname)
       servname = BNETD_DEFAULT_HOST;
    
    if (psock_init()<0)
    {
        fprintf(stderr,"%s: could not inialialize socket functions\n",argv[0]);
        return STATUS_FAILURE;
    }
    
    if (!(host = gethostbyname(servname)))
    {
	fprintf(stderr,"%s: unknown host \"%s\"\n",argv[0],servname);
	return STATUS_FAILURE;
    }
    
    fd_stdin = fileno(stdin);
    if (tcgetattr(fd_stdin,&in_attr_old)>=0)
    {
        in_attr_new = in_attr_old;
        in_attr_new.c_lflag &= ~(ECHO | ICANON); /* turn off ECHO and ICANON */
	in_attr_new.c_cc[VMIN]  = 0; /* don't require reads to return data */
        in_attr_new.c_cc[VTIME] = 1; /* timeout = .1 second */
        tcsetattr(fd_stdin,TCSANOW,&in_attr_new);
        changed_in = 1;
    }
    else
    {
	fprintf(stderr,"%s: could not set terminal attributes for stdin\n",argv[0]);
	changed_in = 0;
    }
    
    if (client_get_termsize(fd_stdin,&screen_width,&screen_height)<0)
    {
	fprintf(stderr,"%s: could not determine screen size\n",argv[0]);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }

    if ((sd = psock_socket(PSOCK_PF_INET,PSOCK_SOCK_STREAM,PSOCK_IPPROTO_TCP))<0)
    {
	fprintf(stderr,"%s: could not create socket (psock_socket: %s)\n",argv[0],pstrerror(psock_errno()));
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = PSOCK_AF_INET;
    saddr.sin_port   = htons(servport);
    memcpy(&saddr.sin_addr.s_addr,host->h_addr_list[0],host->h_length);
    if (psock_connect(sd,(struct sockaddr *)&saddr,sizeof(saddr))<0)
    {
	fprintf(stderr,"%s: could not connect to server \"%s\" port %hu (psock_connect: %s)\n",argv[0],servname,servport,pstrerror(psock_errno()));
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    if (psock_ctl(sd,PSOCK_NONBLOCK)<0)
    {
	fprintf(stderr,"%s: could not set TCP socket to non-blocking mode (psock_ctl: %s)\n",argv[0],pstrerror(psock_errno()));
	psock_close(sd);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    printf("Connected to %s:%hu.\n",inet_ntoa(saddr.sin_addr),servport);
#ifdef CLIENTDEBUG
    eventlog_set(stderr);
#endif
    
    if (!(packet = packet_create(packet_class_init)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    bn_byte_set(&packet->u.client_initconn.class,CLIENT_INITCONN_CLASS_BOT);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
    if (!(rpacket = packet_create(packet_class_raw)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    if (!(packet = packet_create(packet_class_raw)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    packet_append_ntstring(packet,"\004");
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
    {
	int            highest_fd;
	t_psock_fd_set rfds;
	
	PSOCK_FD_ZERO(&rfds);
	highest_fd = fd_stdin;
	if (sd>highest_fd)
	    highest_fd = sd;
	
	commpos = 0;
	text[0] = '\0';
	
	for (;;)
	{
	    PSOCK_FD_SET(fd_stdin,&rfds);
	    PSOCK_FD_SET(sd,&rfds);
	    
	    if (psock_select(highest_fd+1,&rfds,NULL,NULL,NULL)<0)
	    {
		if (errno!=PSOCK_EINTR)
		    fprintf(stderr,"%s: select failed (select: %s)\n",argv[0],pstrerror(errno));
		continue;
	    }
	    
	    if (PSOCK_FD_ISSET(sd,&rfds)) /* got network data */
	    {
		packet_set_size(rpacket,MAX_PACKET_SIZE-1);
		currsize = 0;
		if (net_recv_packet(sd,rpacket,&currsize)<0)
		{
		    psock_close(sd);
		    sd = -1;
		}
		
		if (currsize>0)
		{
		    char * str=packet_get_raw_data(rpacket,0);
		    
		    str[currsize] = '\0';
		    printf("%s",str);
		    fflush(stdout);
		}
		
		if (sd==-1) /* if connection was closed */
		{
		    printf("Connection closed by server.\n");
		    if (changed_in)
			tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
 		    packet_del_ref(rpacket);
		    return STATUS_SUCCESS;
		}
	    }
	    
	    if (PSOCK_FD_ISSET(fd_stdin,&rfds)) /* got keyboard data */
	    {
		switch (client_get_comm("",text,sizeof(text),&commpos,-1,0,screen_width))
		{
		case -1: /* cancel */
		    if (changed_in)
			tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		    return STATUS_FAILURE;
		    
		case 0: /* timeout */
		    break;
		    
		case 1:
		    if (!(packet = packet_create(packet_class_raw)))
		    {
			fprintf(stderr,"%s: could not create packet\n",argv[0]);
			if (changed_in)
			    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
 			packet_del_ref(rpacket);
			return STATUS_FAILURE;
		    }
		    packet_append_ntstring(packet,text);
		    packet_append_ntstring(packet,"\r\n");
		    client_blocksend_packet(sd,packet);
		    packet_del_ref(packet);
		    commpos = 0;
		    text[0] = '\0';
		}
	    }
	}
    }
    
    /* not reached */
}
