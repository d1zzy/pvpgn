/*
 * Copyright (C) 1999  Mark Baysinger (mbaysing@ucsd.edu)
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
#define TRACKER_INTERNAL_ACCESS
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
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "compat/stdfileno.h"
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
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include "compat/inet_ntoa.h"
#include "compat/psock.h"
#include "common/list.h"
#include "common/version.h"
#include "common/eventlog.h"
#include "common/util.h"
#include "common/tracker.h"
#include "common/xalloc.h"
#include "common/setup_after.h"


/******************************************************************************
 * TYPES
 *****************************************************************************/
typedef struct
{
    struct in_addr address;
    time_t         updated;
    t_trackpacket  info;
} t_server;

typedef struct
{
    int            foreground;
    int            debug;
    int            XML_mode;
    unsigned int   expire;
    unsigned int   update;
    unsigned short port;
    char const *   outfile;
    char const *   pidfile;
    char const *   logfile;
    char const *   process;
} t_prefs;


/******************************************************************************
 * STATIC FUNCTION PROTOTYPES
 *****************************************************************************/
static int server_process(int sockfd);
static void usage(char const * progname);
static void getprefs(int argc, char * argv[]);
static void fixup_str(char * str);


/******************************************************************************
 * GLOBAL VARIABLES
 *****************************************************************************/
static t_prefs prefs;


extern int main(int argc, char * argv[])
{
    int sockfd;
    
    if (argc<1 || !argv || !argv[0])
    {
        fprintf(stderr,"bad arguments\n");
        return STATUS_FAILURE;
    }
    
    getprefs(argc,argv);
    
    if (!prefs.debug)
        eventlog_del_level("debug");
    if (prefs.logfile)
    {
	eventlog_set(stderr);
        if (eventlog_open(prefs.logfile)<0)
	{
            eventlog(eventlog_level_fatal,__FUNCTION__,"could not use file \"%s\" for the eventlog (exiting)",prefs.logfile);
	    return STATUS_FAILURE;
	}
    }
    
#ifdef DO_DAEMONIZE
    if (!prefs.foreground)
    {
	switch (fork())
	{
	case -1:
	    eventlog(eventlog_level_error,__FUNCTION__,"could not fork (fork: %s)\n",pstrerror(errno));
	    return STATUS_FAILURE;
	case 0: /* child */
	    break;
	default: /* parent */
	    return STATUS_SUCCESS;
	}
	
	close(STDINFD);
	close(STDOUTFD);
	close(STDERRFD);
	
# ifdef HAVE_SETPGID
	if (setpgid(0,0)<0)
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"could not create new process group (setpgid: %s)\n",pstrerror(errno));
	    return STATUS_FAILURE;
	}
# else
#  ifdef HAVE_SETPGRP
#   ifdef SETPGRP_VOID
	if (setpgrp()<0)
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"could not create new process group (setpgrp: %s)\n",pstrerror(errno));
	    return STATUS_FAILURE;
	}
#   else
	if (setpgrp(0,0)<0)
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"could not create new process group (setpgrp: %s)\n",pstrerror(errno));
	    return STATUS_FAILURE;
	}
#   endif
#  else
#   ifdef HAVE_SETSID
	if (setsid()<0)
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"could not create new process group (setsid: %s)\n",pstrerror(errno));
	    return STATUS_FAILURE;
	}
#   else
#    error "One of setpgid(), setpgrp(), or setsid() is required"
#   endif
#  endif
# endif
    }
#endif
    
    if (prefs.pidfile)
    {
#ifdef HAVE_GETPID
        FILE * fp;

        if (!(fp = fopen(prefs.pidfile,"w")))
        {
            eventlog(eventlog_level_error,__FUNCTION__,"unable to open pid file \"%s\" for writing (fopen: %s)",prefs.pidfile,pstrerror(errno));
            prefs.pidfile = NULL;
        }
        else
        {
            fprintf(fp,"%u",(unsigned int)getpid());
            if (fclose(fp)<0)
                eventlog(eventlog_level_error,__FUNCTION__,"could not close pid file \"%s\" after writing (fclose: %s)",prefs.pidfile,pstrerror(errno));
        }
#else
        eventlog(eventlog_level_warn,__FUNCTION__,"no getpid() system call, do not use the -P or the --pidfile option");
        prefs.pidfile = NULL;
#endif
    }
    
#ifdef HAVE_GETPID
    eventlog(eventlog_level_info,__FUNCTION__,"bntrackd version "PVPGN_VERSION" process %u",(unsigned int)getpid());
#else
    eventlog(eventlog_level_info,__FUNCTION__,"bntrackd version "PVPGN_VERSION);
#endif
    
    if (psock_init()<0)
    {
        eventlog(eventlog_level_error,__FUNCTION__,"could not initialize socket functions");
        return STATUS_FAILURE;
    }
    
    /* create the socket */
    if ((sockfd = psock_socket(PSOCK_PF_INET,PSOCK_SOCK_DGRAM,PSOCK_IPPROTO_UDP))<0)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create UDP listen socket (psock_socket: %s)\n",pstrerror(psock_errno()));
	return STATUS_FAILURE;
    }
    
    {
	struct sockaddr_in servaddr;
	
	/* bind the socket to correct port and interface */
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = PSOCK_AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(prefs.port);
	if (psock_bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"could not bind to UDP port %hu (psock_bind: %s)\n",prefs.port,pstrerror(psock_errno()));
	    return STATUS_FAILURE;
	}
    }
    
    if (server_process(sockfd)<0)
	return STATUS_FAILURE;
    return STATUS_SUCCESS;
}


static int server_process(int sockfd)
{
    t_list *           serverlist_head;
    t_elem *           curr;
    t_server *         server;
    struct sockaddr_in cliaddr;
    t_psock_fd_set     rfds;
    struct timeval     tv;
    time_t             last;
    FILE *             outfile;
    psock_t_socklen    len;
    t_trackpacket      packet;
    
    if (!(serverlist_head = list_create()))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create server list");
	return -1;
    }
    
    /* the main loop */
    last = time(NULL) - prefs.update;
    for (;;)
    {
	/* time to dump our list to disk and call the process command */
	/* (I'm making the assumption that this won't take very long.) */
	if (last+(signed)prefs.update<time(NULL))
	{
	    last = time(NULL);
	    
	    if (!(outfile = fopen(prefs.outfile,"w")))
	    {
		eventlog(eventlog_level_error,__FUNCTION__,"unable to open file \"%s\" for writing (fopen: %s)",prefs.outfile,pstrerror(errno));
		continue;
	    }
	    
	    LIST_TRAVERSE(serverlist_head,curr)
	    {
		server = (t_server*)elem_get_data(curr);
		
		if (server->updated+(signed)prefs.expire<last)
		{
		    list_remove_elem(serverlist_head,&curr);
		    xfree(server);
		}
		else
		{
		   if (prefs.XML_mode == 1)
		  {
		    fprintf(outfile,"<server>\n\t<address>%s</address>\n",inet_ntoa(server->address));
		    fprintf(outfile,"\t<port>%hu</port>\n",(unsigned short)ntohs(server->info.port));
		    fprintf(outfile,"\t<location>%s</location>\n",server->info.server_location);
		    fprintf(outfile,"\t<software>%s</software>\n",server->info.software);
		    fprintf(outfile,"\t<version>%s</version>\n",server->info.version);
		    fprintf(outfile,"\t<users>%lu</users>\n",(unsigned long)ntohl(server->info.users));
		    fprintf(outfile,"\t<channels>%lu</channels>\n",(unsigned long)ntohl(server->info.channels));
		    fprintf(outfile,"\t<games>%lu</games>\n",(unsigned long)ntohl(server->info.games));
		    fprintf(outfile,"\t<description>%s</description>\n",server->info.server_desc);
		    fprintf(outfile,"\t<platform>%s</platform>\n",server->info.platform);
		    fprintf(outfile,"\t<url>%s</url>\n",server->info.server_url);
		    fprintf(outfile,"\t<contact_name>%s</contact_name>\n",server->info.contact_name);
		    fprintf(outfile,"\t<contact_email>%s</contact_email>\n",server->info.contact_email);
		    fprintf(outfile,"\t<uptime>%lu</uptime>\n",(unsigned long)ntohl(server->info.uptime));
		    fprintf(outfile,"\t<total_games>%lu</total_games>\n",(unsigned long)ntohl(server->info.total_games));
		    fprintf(outfile,"\t<logins>%lu</logins>\n",(unsigned long)ntohl(server->info.total_logins));
		    fprintf(outfile,"</server>\n");
		  }
		  else
		  { 
		    fprintf(outfile,"%s\n##\n",inet_ntoa(server->address));
		    fprintf(outfile,"%hu\n##\n",(unsigned short)ntohs(server->info.port));
		    fprintf(outfile,"%s\n##\n",server->info.server_location);
		    fprintf(outfile,"%s\n##\n",server->info.software);
		    fprintf(outfile,"%s\n##\n",server->info.version);
		    fprintf(outfile,"%lu\n##\n",(unsigned long)ntohl(server->info.users));
		    fprintf(outfile,"%lu\n##\n",(unsigned long)ntohl(server->info.channels));
		    fprintf(outfile,"%lu\n##\n",(unsigned long)ntohl(server->info.games));
		    fprintf(outfile,"%s\n##\n",server->info.server_desc);
		    fprintf(outfile,"%s\n##\n",server->info.platform);
		    fprintf(outfile,"%s\n##\n",server->info.server_url);
		    fprintf(outfile,"%s\n##\n",server->info.contact_name);
		    fprintf(outfile,"%s\n##\n",server->info.contact_email);
		    fprintf(outfile,"%lu\n##\n",(unsigned long)ntohl(server->info.uptime));
		    fprintf(outfile,"%lu\n##\n",(unsigned long)ntohl(server->info.total_games));
		    fprintf(outfile,"%lu\n##\n",(unsigned long)ntohl(server->info.total_logins));
		    fprintf(outfile,"###\n");
		  }
		}
	    }
            if (fclose(outfile)<0)
                eventlog(eventlog_level_error,__FUNCTION__,"could not close output file \"%s\" after writing (fclose: %s)",prefs.outfile,pstrerror(errno));
	    
	    if (prefs.process[0]!='\0')
		system(prefs.process);
	}
	
	/* select socket to operate on */
	PSOCK_FD_ZERO(&rfds);
	PSOCK_FD_SET(sockfd,&rfds);
	tv.tv_sec = BNTRACKD_GRANULARITY;
	tv.tv_usec = 0;
	switch (psock_select(sockfd+1,&rfds,NULL,NULL,&tv))
        {
        case -1: /* error */
            if (
#ifdef PSOCK_EINTR
                errno!=PSOCK_EINTR &&
#endif
                1)
                eventlog(eventlog_level_error,__FUNCTION__,"select failed (select: %s)",pstrerror(errno));
        case 0: /* timeout and no sockets ready */
            continue;
        }
	
	/* New tracking packet */
	if (PSOCK_FD_ISSET(sockfd,&rfds))
	{
	    
	    len = sizeof(cliaddr);
	    if (psock_recvfrom(sockfd,&packet,sizeof(packet),0,(struct sockaddr *)&cliaddr,&len)>=0)
	    {
		
		if (ntohs(packet.packet_version)>=TRACK_VERSION)
		{
		    packet.software[sizeof(packet.software)-1] = '\0';
		    if (strstr(packet.software,"##"))
			fixup_str(packet.software);
		    packet.version[sizeof(packet.version)-1] = '\0';
		    if (strstr(packet.version,"##"))
			fixup_str(packet.version);
		    packet.platform[sizeof(packet.platform)-1] = '\0';
		    if (strstr(packet.platform,"##"))
			fixup_str(packet.platform);
		    packet.server_desc[sizeof(packet.server_desc)-1] = '\0';
		    if (strstr(packet.server_desc,"##"))
			fixup_str(packet.server_desc);
		    packet.server_location[sizeof(packet.server_location)-1] = '\0';
		    if (strstr(packet.server_location,"##"))
			fixup_str(packet.server_location);
		    packet.server_url[sizeof(packet.server_url)-1] = '\0';
		    if (strstr(packet.server_url,"##"))
			fixup_str(packet.server_url);
		    packet.contact_name[sizeof(packet.contact_name)-1] = '\0';
		    if (strstr(packet.contact_name,"##"))
			fixup_str(packet.contact_name);
		    packet.contact_email[sizeof(packet.contact_email)-1] = '\0';
		    if (strstr(packet.contact_email,"##"))
			fixup_str(packet.contact_email);
		    
		    /* Find this server's slot */
		    LIST_TRAVERSE(serverlist_head,curr)
		    {
			server = (t_server*)elem_get_data(curr);
			
			if (!memcmp(&server->address,&cliaddr.sin_addr,sizeof(struct in_addr)))
			{
			    if (ntohl(packet.flags)&TF_SHUTDOWN)
			    {
				list_remove_elem(serverlist_head,&curr);
				xfree(server);
			    }
			    else
			    {
				/* update in place */
				server->info = packet;
				server->updated = time(NULL);
			    }
			    break;
			}
		    }
		    
		    /* Not found? Make a new slot */
		    if (!(ntohl(packet.flags)&TF_SHUTDOWN) && !curr)
		    {
			server = (t_server*)xmalloc(sizeof(t_server));
			server->address = cliaddr.sin_addr;
			server->info = packet;
			server->updated = time(NULL);

			list_append_data(serverlist_head,server);
		    }
		    
		    eventlog(eventlog_level_debug,__FUNCTION__,
			     "Packet received from %s:"
			     " packet_version=%u"
			     " flags=0x%08lx"
			     " port=%hu"
			     " software=\"%s\""
			     " version=\"%s\""
			     " platform=\"%s\""
			     " server_desc=\"%s\""
			     " server_location=\"%s\""
			     " server_url=\"%s\""
			     " contact_name=\"%s\""
			     " contact_email=\"%s\""
			     " uptime=%lu"
			     " total_games=%lu"
			     " total_logins=%lu",
			     inet_ntoa(cliaddr.sin_addr),
			     ntohs(packet.packet_version),
			     (unsigned long)ntohl(packet.flags),
			     ntohs(packet.port),
			     packet.software,
			     packet.version,
			     packet.platform,
			     packet.server_desc,
			     packet.server_location,
			     packet.server_url,
			     packet.contact_name,
			     packet.contact_email,
			     (unsigned long)ntohl(packet.uptime),
			     (unsigned long)ntohl(packet.total_games),
			     (unsigned long)ntohl(packet.total_logins));
		}
		
	    }
	}
	
    }
}


static void usage(char const * progname)
{
    fprintf(stderr,"usage: %s [<options>]\n",progname);
    fprintf(stderr,
	   "  -c COMMAND, --command=COMMAND  execute COMMAND update\n"
           "  -d, --debug                    turn on debug mode\n"
           "  -e SECS, --expire SECS         forget a list entry after SEC seconds\n"
#ifdef DO_DAEMONIZE
           "  -f, --foreground               don't daemonize\n"
#else
           "  -f, --foreground               don't daemonize (default)\n"
#endif
           "  -l FILE, --logfile=FILE        write event messages to FILE\n"
           "  -o FILE, --outfile=FILE        write server list to FILE\n");
    fprintf(stderr,
           "  -p PORT, --port=PORT           listen for announcments on UDP port PORT\n"
           "  -P FILE, --pidfile=FILE        write pid to FILE\n"
           "  -u SECS, --update SECS         write output file every SEC seconds\n"
	   "  -x, --XML                      write output file in XML format\n"
           "  -h, --help, --usage            show this information and exit\n"
           "  -v, --version                  print version number and exit\n");
    exit(STATUS_FAILURE);
}


static void getprefs(int argc, char * argv[])
{
    int a;
    
    prefs.foreground = 0;
    prefs.debug      = 0;
    prefs.expire     = 0;
    prefs.update     = 0;
    prefs.port       = 0;
    prefs.XML_mode   = 0;
    prefs.outfile    = NULL;
    prefs.pidfile    = NULL;
    prefs.process    = NULL;
    prefs.logfile    = NULL;
    
    for (a=1; a<argc; a++)
	if (strncmp(argv[a],"--command=",10)==0)
	{
	    if (prefs.process)
	    {
		fprintf(stderr,"%s: processing command was already specified as \"%s\"\n",argv[0],prefs.process);
		usage(argv[0]);
	    }
	    prefs.process = &argv[a][10];
	}
	else if (strcmp(argv[a],"-c")==0)
	{
	    if (a+1>=argc)
	    {
		fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
		usage(argv[0]);
	    }
	    if (prefs.process)
	    {
		fprintf(stderr,"%s: processing command was already specified as \"%s\"\n",argv[0],prefs.process);
		usage(argv[0]);
	    }
	    a++;
	    prefs.process = argv[a];
	}
	else if (strcmp(argv[a],"-d")==0 || strcmp(argv[a],"--debug")==0)
	    prefs.debug = 1;
        else if (strncmp(argv[a],"--expire=",9)==0)
        {
            if (prefs.expire)
            {
                fprintf(stderr,"%s: expiration period was already specified as \"%u\"\n",argv[0],prefs.expire);
                usage(argv[0]);
            }
	    if (str_to_uint(&argv[a][9],&prefs.expire)<0)
	    {
		fprintf(stderr,"%s: \"%s\" should be a positive integer\n",argv[0],&argv[a][9]);
		usage(argv[0]);
	    }
        }
        else if (strcmp(argv[a],"-e")==0)
        {
            if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
            if (prefs.expire)
            {
                fprintf(stderr,"%s: expiration period was already specified as \"%u\"\n",argv[0],prefs.expire);
                usage(argv[0]);
            }
            a++;
	    if (str_to_uint(argv[a],&prefs.expire)<0)
	    {
		fprintf(stderr,"%s: \"%s\" should be a positive integer\n",argv[0],argv[a]);
		usage(argv[0]);
	    }
	}
	else if (strcmp(argv[a],"-f")==0 || strcmp(argv[a],"--foreground")==0)
	    prefs.foreground = 1;
        else if (strcmp(argv[a],"-x")==0 || strcmp(argv[a],"--XML")==0)
	    prefs.XML_mode = 1;
	else if (strncmp(argv[a],"--logfile=",10)==0)
	{
	    if (prefs.logfile)
	    {
		fprintf(stderr,"%s: eventlog file was already specified as \"%s\"\n",argv[0],prefs.logfile);
		usage(argv[0]);
	    }
	    prefs.logfile = &argv[a][10];
	}
	else if (strcmp(argv[a],"-l")==0)
	{
	    if (a+1>=argc)
	    {
		fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
		usage(argv[0]);
	    }
	    if (prefs.logfile)
	    {
		fprintf(stderr,"%s: eventlog file was already specified as \"%s\"\n",argv[0],prefs.logfile);
		usage(argv[0]);
	    }
	    a++;
	    prefs.logfile = argv[a];
	}
	else if (strncmp(argv[a],"--outfile=",10)==0)
	{
	    if (prefs.outfile)
	    {
		fprintf(stderr,"%s: output file was already specified as \"%s\"\n",argv[0],prefs.outfile);
		usage(argv[0]);
	    }
	    prefs.outfile = &argv[a][10];
	}
	else if (strcmp(argv[a],"-o")==0)
	{
	    if (a+1>=argc)
	    {
		fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
		usage(argv[0]);
	    }
	    if (prefs.outfile)
	    {
		fprintf(stderr,"%s: output file was already specified as \"%s\"\n",argv[0],prefs.outfile);
		usage(argv[0]);
	    }
	    a++;
	    prefs.outfile = argv[a];
	}
	else if (strncmp(argv[a],"--pidfile=",10)==0)
	{
	    if (prefs.pidfile)
	    {
		fprintf(stderr,"%s: pid file was already specified as \"%s\"\n",argv[0],prefs.pidfile);
		usage(argv[0]);
	    }
	    prefs.pidfile = &argv[a][10];
	}
        else if (strncmp(argv[a],"--port=",7)==0)
        {
            if (prefs.port)
            {
                fprintf(stderr,"%s: port number was already specified as \"%hu\"\n",argv[0],prefs.port);
                usage(argv[0]);
            }
	    if (str_to_ushort(&argv[a][7],&prefs.port)<0)
	    {
		fprintf(stderr,"%s: \"%s\" should be a positive integer\n",argv[0],&argv[a][7]);
		usage(argv[0]);
	    }
        }
        else if (strcmp(argv[a],"-p")==0)
        {
            if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
            if (prefs.port)
            {
                fprintf(stderr,"%s: port number was already specified as \"%hu\"\n",argv[0],prefs.port);
                usage(argv[0]);
            }
            a++;
	    if (str_to_ushort(argv[a],&prefs.port)<0)
	    {
		fprintf(stderr,"%s: \"%s\" should be a positive integer\n",argv[0],argv[a]);
		usage(argv[0]);
	    }
        }
	else if (strcmp(argv[a],"-P")==0)
	{
	    if (a+1>=argc)
	    {
		fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
		usage(argv[0]);
	    }
	    if (prefs.pidfile)
	    {
		fprintf(stderr,"%s: pid file was already specified as \"%s\"\n",argv[0],prefs.pidfile);
		usage(argv[0]);
	    }
	    a++;
	    prefs.pidfile = argv[a];
	}
        else if (strncmp(argv[a],"--update=",9)==0)
        {
            if (prefs.update)
            {
                fprintf(stderr,"%s: update period was already specified as \"%u\"\n",argv[0],prefs.expire);
                usage(argv[0]);
            }
	    if (str_to_uint(&argv[a][9],&prefs.update)<0)
	    {
		fprintf(stderr,"%s: \"%s\" should be a positive integer\n",argv[0],&argv[a][9]);
		usage(argv[0]);
	    }
        }
        else if (strcmp(argv[a],"-u")==0)
        {
            if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
            if (prefs.update)
            {
                fprintf(stderr,"%s: update period was already specified as \"%u\"\n",argv[0],prefs.expire);
                usage(argv[0]);
            }
            a++;
	    if (str_to_uint(argv[a],&prefs.update)<0)
	    {
		fprintf(stderr,"%s: \"%s\" should be a positive integer\n",argv[0],argv[a]);
		usage(argv[0]);
	    }
        }
        else if (strcmp(argv[a],"-h")==0 || strcmp(argv[a],"--help")==0 || strcmp(argv[a],"--usage")
==0)
            usage(argv[0]);
	else if (strcmp(argv[a],"-v")==0 || strcmp(argv[a],"--version")==0)
        {
            printf("version "PVPGN_VERSION"\n");
            exit(0);
        }
	else if (strcmp(argv[a],"--command")==0 || strcmp(argv[a],"--expire")==0 ||
		 strcmp(argv[a],"--logfile")==0 || strcmp(argv[a],"--outfile")==0 ||
		 strcmp(argv[a],"--port")==0 || strcmp(argv[a],"--pidfile")==0 ||
		 strcmp(argv[a],"--update")==0)
	{
	    fprintf(stderr,"%s: option \"%s\" requires and argument.\n",argv[0],argv[a]);
	    usage(argv[0]);
	}
        else
        {
            fprintf(stderr,"%s: unknown option \"%s\"\n",argv[0],argv[a]);
            usage(argv[0]);
        }
    
    if (!prefs.process)
	prefs.process = BNTRACKD_PROCESS;
    if (prefs.expire==0)
	prefs.expire  = BNTRACKD_EXPIRE;
    if (!prefs.logfile)
	prefs.logfile = BNTRACKD_LOGFILE;
    if (!prefs.outfile)
	prefs.outfile = BNTRACKD_OUTFILE;
    if (prefs.port==0)
	prefs.port    = BNTRACKD_SERVER_PORT;
    if (!prefs.pidfile)
	prefs.pidfile = BNTRACKD_PIDFILE;
    if (prefs.expire==0)
	prefs.update  = BNTRACKD_UPDATE;
    
    if (prefs.logfile[0]=='\0')
	prefs.logfile = NULL;
    if (prefs.pidfile[0]=='\0')
	prefs.pidfile = NULL;
}


static void fixup_str(char * str)
{
    char         prev;
    unsigned int i;
    
    for (prev='\0',i=0; i<strlen(str); prev=str[i],i++)
	if (prev=='#' && str[i]=='#')
	    str[i] = '%';
}
