/*
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#include "compat/strftime.h"
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#include "compat/termios.h"
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
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include "compat/inet_ntoa.h"
#include "compat/psock.h"
#include "common/packet.h"
#include "common/init_protocol.h"
#include "common/udp_protocol.h"
#include "common/bnet_protocol.h"
#include "common/tag.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/network.h"
#include "common/version.h"
#include "common/util.h"
#ifdef CLIENTDEBUG
# include "common/eventlog.h"
#endif
#include "common/bnettime.h"
#include "client.h"
#include "udptest.h"
#include "client_connect.h"
#include "common/setup_after.h"


#ifdef CLIENTDEBUG
# define dprintf printf
#else
# define dprintf if (0) printf
#endif

static char const * bnclass_get_str(unsigned int class); /* FIXME: this is also in command.c */
static void usage(char const * progname);


static char const * bnclass_get_str(unsigned int class)
{
    switch (class)
    {
    case PLAYERINFO_DRTL_CLASS_WARRIOR:
	return "warrior";
    case PLAYERINFO_DRTL_CLASS_ROGUE:
	return "rogue";
    case PLAYERINFO_DRTL_CLASS_SORCERER:
	return "sorcerer";
    default:
	return "unknown";
    }
}


static void usage(char const * progname)
{
    fprintf(stderr,"usage: %s [<options>] [<servername> [<TCP portnumber>]]\n",progname);
    fprintf(stderr,
	    "    -b, --client=SEXP           report client as Brood Wars\n"
            "    -d, --client=DRTL           report client as Diablo Retail\n"
            "    --client=DSHR               report client as Diablo Shareware\n"
            "    -s, --client=STAR           report client as Starcraft (default)\n");
    fprintf(stderr,
            "    --client=SSHR               report client as Starcraft Shareware\n"
	    "    -w, --client=W2BN           report client as Warcraft II BNE\n"
            "    --client=D2DV               report client as Diablo II\n"
            "    --client=D2XP               report client as Diablo II: LoD\n"
            "    --client=WAR3               report client as Warcraft III\n");
    fprintf(stderr,
	    "    -o NAME, --owner=NAME       report CD owner as NAME\n"
	    "    -k KEY, --cdkey=KEY         report CD key as KEY\n"
	    "    -p PLR, --player=PLR        print stats for player PLR\n"
	    "    --bnetd                     also print BNETD-specific stats\n"
	    "    --fsgs                      also print FSGS-specific stats\n"
            "    -h, --help, --usage         show this information and exit\n"
            "    -v, --version               print version number and exit\n");
    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    int                a;
    int                sd;
    int                gotplayer=0;
    struct sockaddr_in saddr;
    t_packet *         packet;
    t_packet *         rpacket;
    char const *       cdowner=NULL;
    char const *       cdkey=NULL;
    char const *       clienttag=NULL;
    char const *       servname=NULL;
    unsigned short     servport=0;
    char               text[MAX_MESSAGE_LEN]="";
    unsigned int       commpos;
    struct termios     in_attr_old;
    struct termios     in_attr_new;
    int                changed_in;
    unsigned int       count;
    unsigned int       sessionkey;
    unsigned int       sessionnum;
    unsigned int       val;
    int                fd_stdin=0;
    int                use_bnetd=0;
    int                use_fsgs=0;
    unsigned int       screen_width,screen_height;
    int                munged=0;
    
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
        else if (strcmp(argv[a],"-b")==0 || strcmp(argv[a],"--client=SEXP")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_BROODWARS;
	}
        else if (strcmp(argv[a],"-d")==0 || strcmp(argv[a],"--client=DRTL")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_DIABLORTL;
	}
        else if (strcmp(argv[a],"--client=DSHR")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_DIABLOSHR;
	}
        else if (strcmp(argv[a],"-s")==0 || strcmp(argv[a],"--client=STAR")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_STARCRAFT;
	}
        else if (strcmp(argv[a],"--client=SSHR")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_SHAREWARE;
	}
	else if (strcmp(argv[a],"-w")==0 || strcmp(argv[a],"--client=W2BN")==0)
	{
            if (clienttag)
            {
                fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
                usage(argv[0]);
            }
            clienttag = CLIENTTAG_WARCIIBNE;
	}
        else if (strcmp(argv[a],"--client=D2DV")==0)
        {
            if (clienttag)
            {
                fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag
);
                usage(argv[0]);
            }
            clienttag = CLIENTTAG_DIABLO2DV;
        }
        else if (strcmp(argv[a],"--client=D2XP")==0)
        {
            if (clienttag)
            {
                fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag
);
                usage(argv[0]);
            }
            clienttag = CLIENTTAG_DIABLO2XP;
        }
        else if (strcmp(argv[a],"--client=WAR3")==0)
        {
            if (clienttag)
            {
                fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag
);
                usage(argv[0]);
            }
            clienttag = CLIENTTAG_WARCRAFT3;
        }
	else if (strncmp(argv[a],"--client=",9)==0)
	{
	    fprintf(stderr,"%s: unknown client tag \"%s\"\n",argv[0],&argv[a][9]);
	    usage(argv[0]);
	}
        else if (strcmp(argv[a],"-o")==0)
        {
            if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
            if (cdowner)
            {
                fprintf(stderr,"%s: CD owner was already specified as \"%s\"\n",argv[0],cdowner);
                usage(argv[0]);
            }
            cdowner = argv[++a];
        }
        else if (strncmp(argv[a],"--owner=",8)==0)
        {
            if (cdowner)
            {
                fprintf(stderr,"%s: CD owner was already specified as \"%s\"\n",argv[0],cdowner);
                usage(argv[0]);
            }
            cdowner = &argv[a][8];
        }
	else if (strcmp(argv[a],"-k")==0)
	{
	    if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
	    if (cdkey)
	    {
		fprintf(stderr,"%s: CD key was already specified as \"%s\"\n",argv[0],cdkey);
		usage(argv[0]);
	    }
	    cdkey = argv[++a];
	}
	else if (strncmp(argv[a],"--cdkey=",8)==0)
	{
	    if (cdkey)
	    {
		fprintf(stderr,"%s: CD key was already specified as \"%s\"\n",argv[0],cdkey);
		usage(argv[0]);
	    }
	    cdkey = &argv[a][8];
	}
	else if (strcmp(argv[a],"-p")==0)
	{
	    if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
	    if (gotplayer)
	    {
		fprintf(stderr,"%s: player was already specified as \"%s\"\n",argv[0],text);
		usage(argv[0]);
	    }
	    a++;
	    if (argv[a][0]=='\0')
	    {
		fprintf(stderr,"%s: player name can not be empty\n",argv[0]);
		usage(argv[0]);
	    }
	    strncpy(text,argv[a],sizeof(text));
	    text[sizeof(text)-1] = '\0';
	    gotplayer = 1;
	}
	else if (strncmp(argv[a],"--player=",9)==0)
	{
	    if (gotplayer)
	    {
		fprintf(stderr,"%s: player was already specified as \"%s\"\n",argv[0],text);
		usage(argv[0]);
	    }
	    if (argv[a][9]=='\0')
	    {
		fprintf(stderr,"%s: player name can not be empty\n",argv[0]);
		usage(argv[0]);
	    }
	    strncpy(text,&argv[a][9],sizeof(text));
	    text[sizeof(text)-1] = '\0';
	    gotplayer = 1;
	}
        else if (strcmp(argv[a],"--bnetd")==0)
	    use_bnetd = 1;
        else if (strcmp(argv[a],"--fsgs")==0)
	    use_fsgs = 1;
	else if (strcmp(argv[a],"-h")==0 || strcmp(argv[a],"--help")==0 || strcmp(argv[a],"--usage")
==0)
            usage(argv[0]);
	else if (strcmp(argv[a],"-v")==0 || strcmp(argv[a],"--version")==0)
	{
            printf("version "PVPGN_VERSION"\n");
            return STATUS_SUCCESS;
	}
        else if (strcmp(argv[a],"--client")==0 || strcmp(argv[a],"--owner")==0 ||
		 strcmp(argv[a],"--cdkey")==0 || strcmp(argv[a],"--player")==0)
	{
	    fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
	    usage(argv[0]);
	}
	else
	{
	    fprintf(stderr,"%s: unknown option \"%s\"\n",argv[0],argv[a]);
	    usage(argv[0]);
	}
    
    if (servport==0)
	servport = BNETD_SERV_PORT;
    if (!cdowner)
	cdowner = BNETD_DEFAULT_OWNER;
    if (!cdkey)
	cdkey = BNETD_DEFAULT_KEY;
    if (!clienttag)
	clienttag = CLIENTTAG_STARCRAFT;
    if (!servname)
	servname = BNETD_DEFAULT_HOST;
    
    if (gotplayer)
	changed_in = 0; /* no need to change terminal attributes */
    else
    {
	fd_stdin = fileno(stdin);
	if (tcgetattr(fd_stdin,&in_attr_old)>=0)
	{
	    in_attr_new = in_attr_old;
	    in_attr_new.c_lflag &= ~(ECHO | ICANON); /* turn off ECHO and ICANON */
	    in_attr_new.c_cc[VMIN]  = 1; /* require reads to return at least one byte */
	    in_attr_new.c_cc[VTIME] = 0; /* no timeout */
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
	    fprintf(stderr,"%s: could not determine screen size, assuming 80x24\n",argv[0]);
	    screen_width  = 80;
	    screen_height = 24;
	}
    }
    
    if ((sd = client_connect(argv[0],servname,servport,cdowner,cdkey,clienttag,&saddr,&sessionkey,&sessionnum,ARCHTAG_WINX86,CLIENT_COUNTRYINFO_109_GAMELANG))<0)
    {
	fprintf(stderr,"%s: fatal error during handshake\n",argv[0]);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    /* reuse this same packet over and over */
    if (!(rpacket = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	psock_close(sd);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	psock_close(sd);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    packet_set_size(packet,sizeof(t_client_fileinforeq));
    packet_set_type(packet,CLIENT_FILEINFOREQ);
    bn_int_set(&packet->u.client_fileinforeq.type,CLIENT_FILEINFOREQ_TYPE_TOS);
    bn_int_set(&packet->u.client_fileinforeq.unknown2,CLIENT_FILEINFOREQ_UNKNOWN2);
    packet_append_string(packet,CLIENT_FILEINFOREQ_FILE_TOSUSA);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed connection\n",argv[0]);
	    psock_close(sd);
	    if (changed_in)
		tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	    return STATUS_FAILURE;
	}
    while (packet_get_type(rpacket)!=SERVER_FILEINFOREPLY);
    dprintf("Got FILEINFOREPLY\n");
    
    if (!gotplayer)
    {
	munged = 1;
	commpos = 0;
	text[0] = '\0';
    }
    
    for (;;)
    {
	unsigned int i;
	
	if (!gotplayer)
	{
	    switch (client_get_comm("player: ",text,sizeof(text),&commpos,1,munged,screen_width))
	    {
	    case -1: /* cancel or error */
                break;
		
	    case 0: /* timeout */
		munged = 0;
                continue;
		
	    case 1:
		munged = 0;
                if (text[0]=='\0')
            	    continue;
	    }
	    
	    if (text[0]=='\0')
	        break;
            printf("\r");
            for (i=0; i<strlen(text) && i<screen_width; i++)
                printf(" ");
	    printf("\r");
	}
	
	if (!(packet = packet_create(packet_class_bnet)))
	{
	    fprintf(stderr,"%s: could not create packet\n",argv[0]);
	    psock_close(sd);
	    if (changed_in)
		tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	    return STATUS_FAILURE;
	}
	packet_set_size(packet,sizeof(t_client_statsreq));
	packet_set_type(packet,CLIENT_STATSREQ);
	bn_int_set(&packet->u.client_statsreq.name_count,1);
	bn_int_set(&packet->u.client_statsreq.requestid,0);
	packet_append_string(packet,text);
	count = 0;
	
	if (use_bnetd)
	{
	    packet_append_string(packet,"BNET\\acct\\username");
	    packet_append_string(packet,"BNET\\acct\\userid");
	    packet_append_string(packet,"BNET\\acct\\lastlogin_clienttag");
	    packet_append_string(packet,"BNET\\acct\\lastlogin_connection");
	    packet_append_string(packet,"BNET\\acct\\lastlogin_time");
	    packet_append_string(packet,"BNET\\acct\\firstlogin_clienttag");
	    packet_append_string(packet,"BNET\\acct\\firstlogin_connection");
	    packet_append_string(packet,"BNET\\acct\\firstlogin_time");
	    count += 8;
	}
	if (use_fsgs)
	{
	    packet_append_string(packet,"FSGS\\Created");
	    count += 1;
	}
	
	packet_append_string(packet,"profile\\sex");
	packet_append_string(packet,"profile\\age");
	packet_append_string(packet,"profile\\location");
	packet_append_string(packet,"profile\\description");
	count += 4;
	
	if (strcmp(clienttag,"STAR")==0)
	{
	    packet_append_string(packet,"Record\\STAR\\0\\last game");
	    packet_append_string(packet,"Record\\STAR\\0\\last game result");
	    packet_append_string(packet,"Record\\STAR\\0\\wins");
	    packet_append_string(packet,"Record\\STAR\\0\\losses");
	    packet_append_string(packet,"Record\\STAR\\0\\disconnects");
	    packet_append_string(packet,"Record\\STAR\\0\\draws");
	    count += 6;
	    
	    packet_append_string(packet,"Record\\STAR\\1\\last game");
	    packet_append_string(packet,"Record\\STAR\\1\\last game result");
	    packet_append_string(packet,"Record\\STAR\\1\\rating");
	    packet_append_string(packet,"Record\\STAR\\1\\active rating");
	    packet_append_string(packet,"Record\\STAR\\1\\high rating");
	    packet_append_string(packet,"Record\\STAR\\1\\rank");
	    packet_append_string(packet,"Record\\STAR\\1\\active rank");
	    packet_append_string(packet,"Record\\STAR\\1\\high rank");
	    packet_append_string(packet,"Record\\STAR\\1\\wins");
	    packet_append_string(packet,"Record\\STAR\\1\\losses");
	    packet_append_string(packet,"Record\\STAR\\1\\disconnects");
	    packet_append_string(packet,"Record\\STAR\\1\\draws");
	    count += 12;
	}
	else if (strcmp(clienttag,"SEXP")==0)
	{
	    packet_append_string(packet,"Record\\SEXP\\0\\last game");
	    packet_append_string(packet,"Record\\SEXP\\0\\last game result");
	    packet_append_string(packet,"Record\\SEXP\\0\\wins");
	    packet_append_string(packet,"Record\\SEXP\\0\\losses");
	    packet_append_string(packet,"Record\\SEXP\\0\\disconnects");
	    packet_append_string(packet,"Record\\SEXP\\0\\draws");
	    count += 6;
	    
	    packet_append_string(packet,"Record\\SEXP\\1\\last game");
	    packet_append_string(packet,"Record\\SEXP\\1\\last game result");
	    packet_append_string(packet,"Record\\SEXP\\1\\rating");
	    packet_append_string(packet,"Record\\SEXP\\1\\active rating");
	    packet_append_string(packet,"Record\\SEXP\\1\\high rating");
	    packet_append_string(packet,"Record\\SEXP\\1\\rank");
	    packet_append_string(packet,"Record\\SEXP\\1\\active rank");
	    packet_append_string(packet,"Record\\SEXP\\1\\high rank");
	    packet_append_string(packet,"Record\\SEXP\\1\\wins");
	    packet_append_string(packet,"Record\\SEXP\\1\\losses");
	    packet_append_string(packet,"Record\\SEXP\\1\\disconnects");
	    packet_append_string(packet,"Record\\SEXP\\1\\draws");
	    count += 12;
	}
	else if (strcmp(clienttag,"SSHR")==0)
	{
	    packet_append_string(packet,"Record\\SSHR\\0\\last game");
	    packet_append_string(packet,"Record\\SSHR\\0\\last game result");
	    packet_append_string(packet,"Record\\SSHR\\0\\wins");
	    packet_append_string(packet,"Record\\SSHR\\0\\losses");
	    packet_append_string(packet,"Record\\SSHR\\0\\disconnects");
	    packet_append_string(packet,"Record\\SSHR\\0\\draws");
	    count += 6;
	}
	else if (strcmp(clienttag,"DSHR")==0 ||
		 strcmp(clienttag,"DRTL")==0)
	{
	    if (use_bnetd)
	    {
		packet_append_string(packet,"BNET\\Record\\DRTL\\0\\level");
		packet_append_string(packet,"BNET\\Record\\DRTL\\0\\class");
		packet_append_string(packet,"BNET\\Record\\DRTL\\0\\strength");
		packet_append_string(packet,"BNET\\Record\\DRTL\\0\\magic");
		packet_append_string(packet,"BNET\\Record\\DRTL\\0\\dexterity");
		packet_append_string(packet,"BNET\\Record\\DRTL\\0\\vitality");
		packet_append_string(packet,"BNET\\Record\\DRTL\\0\\gold");
		packet_append_string(packet,"BNET\\Record\\DRTL\\0\\diablo kills");
		count += 8;
	    }
	}
	else if (strcmp(clienttag,"W2BN")==0)
	{
	    packet_append_string(packet,"Record\\W2BN\\0\\last game");
	    packet_append_string(packet,"Record\\W2BN\\0\\last game result");
	    packet_append_string(packet,"Record\\W2BN\\0\\wins");
	    packet_append_string(packet,"Record\\W2BN\\0\\losses");
	    packet_append_string(packet,"Record\\W2BN\\0\\disconnects");
	    packet_append_string(packet,"Record\\W2BN\\0\\draws");
	    count += 6;
	    
	    packet_append_string(packet,"Record\\W2BN\\1\\last game");
	    packet_append_string(packet,"Record\\W2BN\\1\\last game result");
	    packet_append_string(packet,"Record\\W2BN\\1\\rating");
	    packet_append_string(packet,"Record\\W2BN\\1\\active rating");
	    packet_append_string(packet,"Record\\W2BN\\1\\high rating");
	    packet_append_string(packet,"Record\\W2BN\\1\\rank");
	    packet_append_string(packet,"Record\\W2BN\\1\\active rank");
	    packet_append_string(packet,"Record\\W2BN\\1\\high rank");
	    packet_append_string(packet,"Record\\W2BN\\1\\wins");
	    packet_append_string(packet,"Record\\W2BN\\1\\losses");
	    packet_append_string(packet,"Record\\W2BN\\1\\disconnects");
	    packet_append_string(packet,"Record\\W2BN\\1\\draws");
	    count += 12;
	    
	    packet_append_string(packet,"Record\\W2BN\\3\\last game");
	    packet_append_string(packet,"Record\\W2BN\\3\\last game result");
	    packet_append_string(packet,"Record\\W2BN\\3\\rating");
	    packet_append_string(packet,"Record\\W2BN\\3\\active rating");
	    packet_append_string(packet,"Record\\W2BN\\3\\high rating");
	    packet_append_string(packet,"Record\\W2BN\\3\\rank");
	    packet_append_string(packet,"Record\\W2BN\\3\\active rank");
	    packet_append_string(packet,"Record\\W2BN\\3\\high rank");
	    packet_append_string(packet,"Record\\W2BN\\3\\wins");
	    packet_append_string(packet,"Record\\W2BN\\3\\losses");
	    packet_append_string(packet,"Record\\W2BN\\3\\disconnects");
	    packet_append_string(packet,"Record\\W2BN\\3\\draws");
	    count += 12;
	}
	bn_int_set(&packet->u.client_statsreq.key_count,count);
	client_blocksend_packet(sd,packet);
	packet_del_ref(packet);
	
	do
	    if (client_blockrecv_packet(sd,rpacket)<0)
	    {
		fprintf(stderr,"%s: server closed connection\n",argv[0]);
		if (changed_in)
		    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		return STATUS_FAILURE;
	    }
	while (packet_get_type(rpacket)!=SERVER_STATSREPLY);
	dprintf("Got STATSREPLY\n");
	
	{
	    unsigned int   names,keys;
	    unsigned int   j;
	    unsigned int   strpos;
	    char const *   temp;
	    unsigned int   uid;
	    struct in_addr laddr;
	    time_t         tm;
	    char           timestr[STAT_TIME_MAXLEN];
	    
	    names = bn_int_get(rpacket->u.server_statsreply.name_count);
	    keys = bn_int_get(rpacket->u.server_statsreply.key_count);
	    
	    printf("----\n");
	    
	    strpos = sizeof(t_server_statsreply);
	    for (i=0; i<names; i++)
	    {
		j = 0;
		if (use_bnetd)
		{
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (temp[0]=='\0')
			printf("                        Username: UNKNOWN\n");
		    else
			printf("                        Username: %s\n",temp);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&uid)<0 || uid<1)
			printf("                         Account: UNKNOWN\n");
		    else
			printf("                         Account: "UID_FORMAT"\n",uid);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    printf("               Last login client: %s\n",temp);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&val)<0)
			printf("                 Last login host: UNKNOWN\n");
		    else
		    {
			laddr.s_addr = htonl(val);
			printf("                 Last login host: %s\n",inet_ntoa(laddr));
		    }
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&val)<0 || val==0)
			printf("                 Last login time: NEVER\n");
		    else
		    {
			tm = (time_t)val;
			strftime(timestr,STAT_TIME_MAXLEN,STAT_TIME_FORMAT,localtime(&tm));
			printf("                 Last login time: %s\n",timestr);
		    }
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    printf("              First login client: %s\n",temp);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&val)<0)
			printf("                First login host: UNKNOWN\n");
		    else
		    {
			laddr.s_addr = htonl(val);
			printf("                First login host: %s\n",inet_ntoa(laddr));
		    }
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&val)<0 || val==0)
			printf("                First login time: NEVER\n");
		    else
		    {
			tm = (time_t)val;
			strftime(timestr,STAT_TIME_MAXLEN,STAT_TIME_FORMAT,localtime(&tm));
			printf("                First login time: %s\n",timestr);
		    }
		}
		else
		    printf("                        Username: %s\n",text);
		if (use_fsgs)
		{
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&val)<0 || val==0)
			printf("                   Creation time: NEVER\n");
		    else
		    {
			tm = (time_t)val;
			strftime(timestr,STAT_TIME_MAXLEN,STAT_TIME_FORMAT,localtime(&tm));
			printf("                   Creation time: %s\n",timestr);
		    }
		}
		
		if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
		    strpos += strlen(temp)+1;
		else
		    temp = "";
		j++;
    		printf("                             Sex: %s\n",temp);
		
		if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
		    strpos += strlen(temp)+1;
		else
		    temp = "";
		j++;
    		printf("                             Age: %s\n",temp);
		
		if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
		    strpos += strlen(temp)+1;
		else
		    temp = "";
		j++;
    		printf("                        Location: %s\n",temp);
		
		if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
		    strpos += strlen(temp)+1;
		else
		    temp = "";
		j++;
    		printf("                     Description: %s\n",temp);
		
		if (strcmp(clienttag,"STAR")==0 ||
		    strcmp(clienttag,"SSHR")==0 ||
		    strcmp(clienttag,"SEXP")==0 ||
		    strcmp(clienttag,"W2BN")==0)
		{
		    t_bnettime   bntime;
		    unsigned int wins,losses,disconnects,draws;
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (bnettime_set_str(&bntime,temp)<0)
			strcpy(timestr,"NEVER");
		    else
		    {
			tm = bnettime_to_time(bntime);
			strftime(timestr,STAT_TIME_MAXLEN,STAT_TIME_FORMAT,localtime(&tm));
		    }
	    	    printf("                  Last game time: %s\n",timestr);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
	    	    printf("                Last game result: %s\n",temp);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&wins)<0)
			wins = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&losses)<0)
			losses = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&disconnects)<0)
			disconnects = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&draws)<0)
			draws = 0;
	            printf("                          Record: (%u/%u/%u) %u draws\n",wins,losses,disconnects,draws);
		}
		    
		if (strcmp(clienttag,"STAR")==0 ||
		    strcmp(clienttag,"SEXP")==0 ||
		    strcmp(clienttag,"W2BN")==0)
		{
		    t_bnettime   bntime;
		    unsigned int wins,losses,disconnects,draws;
		    unsigned int rating,rank;
		    unsigned int active_rating,active_rank;
		    unsigned int high_rating,high_rank;
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (bnettime_set_str(&bntime,temp)<0)
			strcpy(timestr,"NEVER");
		    else
		    {
			tm = bnettime_to_time(bntime);
			strftime(timestr,STAT_TIME_MAXLEN,STAT_TIME_FORMAT,localtime(&tm));
		    }
	    	    printf("  Last standard ladder game time: %s\n",timestr);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
	    	    printf("Last standard ladder game result: %s\n",temp);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&rating)<0)
			rating = 0;
		    if (rating)
	    		printf("  Current standard ladder rating: %u\n",rating);
		    else
	    		printf("  Current standard ladder rating: UNRATED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&active_rating)<0)
			active_rating = 0;
		    if (active_rating)
	    		printf("   Active standard ladder rating: %u\n",active_rating);
		    else
	    		printf("   Active standard ladder rating: UNRATED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&high_rating)<0)
			high_rating = 0;
		    if (high_rating)
	    		printf("     High standard ladder rating: %u\n",high_rating);
		    else
	    		printf("     High standard ladder rating: UNRATED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&rank)<0)
			rank = 0;
		    if (rank)
	  		printf("    Current standard ladder rank: #%u\n",rank);
		    else
			printf("    Current standard ladder rank: UNRANKED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&active_rank)<0)
			active_rank = 0;
		    if (active_rank)
	    		printf("     Active standard ladder rank: #%u\n",active_rank);
		    else
			printf("     Active standard ladder rank: UNRANKED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&high_rank)<0)
			high_rank = 0;
		    if (high_rank)
	    		printf("       High standard ladder rank: #%u\n",high_rank);
		    else
			printf("       High standard ladder rank: UNRANKED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&wins)<0)
			wins = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&losses)<0)
			losses = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&disconnects)<0)
			disconnects = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&draws)<0)
			draws = 0;
	            printf("          Standard ladder record: (%u/%u/%u) %u draws\n",wins,losses,disconnects,draws);
		}
		
		if (strcmp(clienttag,"W2BN")==0)
		{
		    t_bnettime   bntime;
		    unsigned int wins,losses,disconnects,draws;
		    unsigned int rating,rank;
		    unsigned int active_rating,active_rank;
		    unsigned int high_rating,high_rank;
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (bnettime_set_str(&bntime,temp)<0)
			strcpy(timestr,"NEVER");
		    else
		    {
			tm = bnettime_to_time(bntime);
			strftime(timestr,STAT_TIME_MAXLEN,STAT_TIME_FORMAT,localtime(&tm));
		    }
	    	    printf("   Last Ironman ladder game time: %s\n",timestr);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
	    	    printf(" Last Ironman ladder game result: %s\n",temp);
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&rating)<0)
			rating = 0;
		    if (rating)
	    		printf("   Current Ironman ladder rating: %u\n",rating);
		    else
	    		printf("   Current Ironman ladder rating: UNRATED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&active_rating)<0)
			active_rating = 0;
		    if (active_rating)
	    		printf("    Active Ironman ladder rating: %u\n",active_rating);
		    else
	    		printf("    Active Ironman ladder rating: UNRATED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&high_rating)<0)
			high_rating = 0;
		    if (high_rating)
	    		printf("      High Ironman ladder rating: %u\n",high_rating);
		    else
	    		printf("      High Ironman ladder rating: UNRATED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&rank)<0)
			rank = 0;
		    if (rank)
	  		printf("     Current Ironman ladder rank: #%u\n",rank);
		    else
			printf("     Current Ironman ladder rank: UNRANKED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&active_rank)<0)
			active_rank = 0;
		    if (active_rank)
	    		printf("      Active Ironman ladder rank: #%u\n",active_rank);
		    else
			printf("      Active Ironman ladder rank: UNRANKED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&high_rank)<0)
			high_rank = 0;
		    if (high_rank)
	    		printf("        High Ironman ladder rank: #%u\n",high_rank);
		    else
			printf("        High Ironman ladder rank: UNRANKED\n");
		    
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&wins)<0)
			wins = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&losses)<0)
			losses = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&disconnects)<0)
			disconnects = 0;
		    if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			strpos += strlen(temp)+1;
		    else
			temp = "";
		    j++;
		    if (str_to_uint(temp,&draws)<0)
			draws = 0;
	            printf("           Ironman ladder record: (%u/%u/%u) %u draws\n",wins,losses,disconnects,draws);
		}
		
		if (strcmp(clienttag,"DSHR")==0 ||
		    strcmp(clienttag,"DRTL")==0)
		{
		    unsigned int level;
		    unsigned int class;
		    unsigned int diablo_kills;
		    unsigned int strength;
		    unsigned int magic;
		    unsigned int dexterity;
		    unsigned int vitality;
		    unsigned int gold;
		    
		    if (use_bnetd)
		    {
			if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			    strpos += strlen(temp)+1;
			else
			    temp = "";
			j++;
			if (str_to_uint(temp,&level)<0)
			    level = 0;
			if (level>0)
			    printf("                           Level: %u\n",level);
			else
			    printf("                           Level: NONE\n");
			
			if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			    strpos += strlen(temp)+1;
			else
			    temp = "";
			j++;
			if (str_to_uint(temp,&class)<0)
			    class = 99;
			printf("                           Class: %s\n",bnclass_get_str(class));
			
			if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			    strpos += strlen(temp)+1;
			else
			    temp = "";
			j++;
			if (str_to_uint(temp,&strength)<0)
			    strength = 0;
			
			if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			    strpos += strlen(temp)+1;
			else
			    temp = "";
			j++;
			if (str_to_uint(temp,&magic)<0)
			    magic = 0;
			
			if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			    strpos += strlen(temp)+1;
			else
			    temp = "";
			j++;
			if (str_to_uint(temp,&dexterity)<0)
			    dexterity = 0;
			
			if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			    strpos += strlen(temp)+1;
			else
			    temp = "";
			j++;
			if (str_to_uint(temp,&vitality)<0)
			    vitality = 0;
			
			if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			    strpos += strlen(temp)+1;
			else
			    temp = "";
			j++;
			if (str_to_uint(temp,&gold)<0)
			    gold = 0;
			
			printf("                           Stats: %u str  %u mag  %u dex  %u vit  %u gld\n",
			       strength,
			       magic,
			       dexterity,
			       vitality,
			       gold);
			
			if (j<keys && (temp = packet_get_str_const(rpacket,strpos,256)))
			    strpos += strlen(temp)+1;
			else
			    temp = "";
			j++;
			if (str_to_uint(temp,&diablo_kills)<0)
			    diablo_kills = 0;
			printf("                    Diablo Kills: %u\n",diablo_kills);
		    }
		}
	    }
	    printf("----\n");
	}
	
	if (gotplayer)
	    break;
	commpos = 0;
	text[0] = '\0';
    }
    
    psock_close(sd);
    if (changed_in)
	tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
    return STATUS_SUCCESS;
}
