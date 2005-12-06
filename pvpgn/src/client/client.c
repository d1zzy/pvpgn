/*
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999  Oleg Drokin (green@ccssu.ccssu.crimea.ua)
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
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strrchr.h"
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "compat/read.h"
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#include "compat/psock.h"
#include "common/packet.h"
#include "common/network.h"
#ifdef WIN32
# include <conio.h> /* for kbhit() and getch() */
#endif
#include "client.h"
#include "common/setup_after.h"


extern int client_blocksend_packet(int sock, t_packet const * packet)
{
    unsigned int size=0;
    
    for (;;)
        switch (net_send_packet(sock,packet,&size))
	{
	case -1:
	    return -1;
	    
	case 0:
	    continue;
	    
	default:
	    return 0;
	}
}


extern int client_blockrecv_packet(int sock, t_packet * packet)
{
    unsigned int size=0;
    
    for (;;)
        switch (net_recv_packet(sock,packet,&size))
        {
        case -1:
            return -1;
            
        case 0:
            continue;

        default:
/*	    printf("Got packet len %u/%u type 0x%04x\n",size,packet_get_size(packet),packet_get_type(packet)); */
            return 0;
        }
}


extern int client_blockrecv_raw_packet(int sock, t_packet * packet, unsigned int len)
{
    unsigned int size=0;
    
    packet_set_size(packet,len);
    for (;;)
        switch (net_recv_packet(sock,packet,&size))
        {
        case -1:
            return -1;
        case 0:
            continue;
        case 1:
	default:
	    return 0;
        }
}


extern int client_get_termsize(int fd, unsigned int * w, unsigned int * h)
{
    if (fd<0 || !w || !h)
	return -1;
    
    *w = 0;
    *h = 0;
    
#ifdef HAVE_IOCTL
#ifdef TIOCGWINSZ
    {
	struct winsize ws;
	
	if (ioctl(fd,TIOCGWINSZ,&ws)>=0)
	{
	    *w = (unsigned int)ws.ws_col;
	    *h = (unsigned int)ws.ws_row;
	}
    }
#endif
#endif
    
#ifdef HAVE_GETENV
    {
	char const * str;
	int          val;
	
	if (!*w && (str = getenv("COLUMNS")) && (val = atoi(str))>0)
	    *w = val;
	if (!*h && (str = getenv("LINES")) && (val = atoi(str))>0)
	    *h = val;
    }
#endif
    
    if (!*w)
	*w = DEF_SCREEN_WIDTH;
    if (!*h)
	*h = DEF_SCREEN_HEIGHT;
    
    return 0;
}


/* This routine gets keyboard input. It handles printing the prompt, cursor positioning, and
   text scrolling. It unfortunatly assumes that the chars read from stdin are in ASCII. */
/* visible: -1=nothing, 0=prompt only, 1=prompt and text */
extern int client_get_comm(char const * prompt, char * text, unsigned int maxlen, unsigned int * curpos, int visible, int redraw, unsigned int width)
{
    char         temp;
    unsigned int addlen;
    unsigned int i;
    unsigned int count;
    int          beg_pos;
    
    for (count=0; count<16; count++)
    {
	beg_pos = 0;
	if (strlen(prompt)+1>=width)
	    visible = 0; /* no room to show any of the text */
	else if (*curpos+strlen(prompt)+1>=width)
	    beg_pos = (int)(*curpos+strlen(prompt)+1-width);
	
	if (redraw)
	{
	    if (visible!=-1)
		printf("\r%s",prompt);
	    if (visible==1)
		printf("%s",text+beg_pos);
	}
	
	fflush(stdout);
#ifndef WIN32
	addlen = read(fileno(stdin),&temp,1);
#else
	if (kbhit())
	{
	  temp = getch();
	  addlen=1;
	}
	else
	{
	  temp = 0;
	  addlen=0;
	}
#endif

	if (addlen<1)
	    return addlen;
	
	switch (temp)
	{
	case '\033': /* ESC */
	    return -1;
	case '\r':
	case '\n':
	    return 1;
	case '\b':   /* BS */
	case '\177': /* DEL */
	    if (*curpos<1) /* already empty */
	    {
		if (visible==1)
		    printf("\a");
		continue;
	    }
	    (*curpos)--;
	    text[*curpos] = '\0';
	    if (visible==1)
	    {
		if (beg_pos>0)
		{
		    beg_pos--;
		    printf("\r%s%s",prompt,text+beg_pos);
		}
		else
		  { printf("\b \b"); }
	    }
	    continue;
	case '\024': /* ^T */
	    if (*curpos<2)
	    {
		if (visible==1)
		    printf("\a");
		continue;
	    }
	    {
		char swap;
		
		swap = text[*curpos-1];
		text[*curpos-1] = text[*curpos-2];
		text[*curpos-2] = swap;
		
		if (visible==1)
		{
		    printf("\b\b");
		    printf("%s",&text[*curpos-2]);
		}
	    }
	    continue;
	case '\027': /* ^W */
	    if (*curpos<1)
	    {
		if (visible==1)
		    printf("\a");
		continue;
	    }
	    {
		char * t=strrchr(text,' ');
		unsigned int t1=beg_pos,t2;
		
		addlen = t ? t - text : 0;
		text[addlen] = '\0';
		beg_pos -= *curpos - addlen;
		if (beg_pos < 0)
		    beg_pos=0;
		if (visible==1)
		{
		    if (t1 == 0)
			for (i=0; i < *curpos - addlen; i++)
			    printf("\b \b");
		    else
		    {
			/* the \r is counted in the return value, but that's ok
			   because the last column is always blank */
			t2 = printf("\r%s%s",prompt,text+beg_pos);
			if (t1 > 0 && beg_pos == 0)
			{
			    for (i=0; i<width-t2; i++)
				printf(" ");
			    for (i=0; i<width-t2; i++)
				printf("\b");
			}
		    }
		}
	    }
	    *curpos = addlen;
	    continue;
	case '\025': /* ^U */
	    if (visible==1)
	    {
		unsigned int t2;
		
		t2 = printf("\r%s",prompt);
		for (i=0; i<width-t2; i++)
		    printf(" ");
		printf("\r%s",prompt);
	    }
	    *curpos = 0;
	    text[0] = '\0';
	    continue;
	default:
	    if (temp>0 && temp<32) /* unhandled control char */
	    {
		if (visible==1)
		    printf("\a");
		continue;
	    }
	    if ((*curpos+1)>=maxlen) /* too full */
	    {
		if (visible==1)
		    printf("\a");
		continue;
	    }
	    if (visible==1)
	    {
		if (beg_pos>0 || (*curpos + strlen(prompt) + 2 > width))
		{
		    beg_pos++;
		    printf("\r%s%s",prompt,text+beg_pos);
		}
		printf("%c",temp);
	    }
	    text[*curpos] = temp;
	    (*curpos)++;
	    text[*curpos] = '\0';
	    continue;
	}
    }
    
    return 0;
}
