/*
 * Copyright (C) 2000  Dizzy
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
#endif
#include "compat/strcasecmp.h"
#include <ctype.h>
#include <errno.h>
#include "compat/strerror.h"
#include "message.h"
#include "connection.h"
#include "common/util.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "helpfile.h"
#include "common/setup_after.h"
#include "account_wrap.h"
#include "command_groups.h"

namespace pvpgn
{

namespace bnetd
{

static FILE* hfd=NULL; /* helpfile descriptor */

static int list_commands(t_connection *);
static int describe_command(t_connection *, char const *);


extern int helpfile_init(char const *filename)
{
    if (!filename)
    {
        eventlog(eventlog_level_error,__FUNCTION__,"got NULL filename");
	return -1;
    }
    if (!(hfd = fopen(filename,"r")))
    {
        eventlog(eventlog_level_error,__FUNCTION__,"could not open help file \"%s\" for reading (fopen: %s)",filename,pstrerror(errno));
        return -1;
    }
    return 0;
}


extern int helpfile_unload(void)
{
    if (hfd!=NULL)
    {
	if (fclose(hfd)<0)
	    eventlog(eventlog_level_error,__FUNCTION__,"could not close help file after reading (fclose: %s)",pstrerror(errno));
	hfd = NULL;
    }
    return 0;
}


extern int handle_help_command(t_connection * c, char const * text)
{
    unsigned int i,j;
    char         comm[MAX_COMMAND_LEN];

    if (hfd == NULL)
    { /* an error ocured opening readonly the help file, helpfile_unload was called, or helpfile_init hasn't been called */
        message_send_text(c,message_type_error,c,"Oops ! There is a problem with the help file. Please contact the administrator of the server.");
        return 0;
    }

    rewind(hfd);
    for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip command */
    for (; text[i]==' '; i++);
    if (text[i]=='/') /* skip / in front of command (if present) */
        i++;
    for (j=0; text[i]!=' ' && text[i]!='\0'; i++) /* get comm */
    if (j<sizeof(comm)-1) comm[j++] = text[i];
    comm[j] = '\0';

    /* just read the whole file and dump only the commands */
    if (comm[0]=='\0')
    {
        list_commands(c);
        return 0;
    }

    if (describe_command(c,comm)==0) return 0;
    /* no description was found for this command. inform the user */
    message_send_text(c,message_type_error,c," no help available for that command");
    return 0;
}


static int list_commands(t_connection * c)
{
    char * line;
    int    i;

    message_send_text(c,message_type_info,c,"Chat commands:");
    while ((line=file_get_line(hfd))!=NULL)
    {
        for (i=0;line[i]==' ' && line[i]!='\0';i++); /* skip spaces in front of %command */
        if (line[i]=='%')  /* is this a command ? */
        {
            char *p,*buffer;
            int al;
	    int skip;
	    unsigned int length,position;

            /* ok. now we must see if there are any aliases */
            length=MAX_COMMAND_LEN+1; position=0;
            buffer=(char*)xmalloc(length+1); /* initial memory allocation = pretty fair */
            p=line+i;
            do
	    {
                al=0;
		skip = 0;
                for (i=1;p[i]!=' ' && p[i]!='\0' && p[i]!='#';i++); /* skip command */
                if (p[i]==' ') al=1; /* we have something after the command.. must remember that */
                p[i]='\0'; /* end the string at the end of the command */
                p[0]='/'; /* change the leading character (% or space) read from the help file to / */
		if (!(command_get_group(p) & account_get_command_groups(conn_get_account(c)))) skip=1;
                if (length<strlen(p)+position+1)
                /* if we don't have enough space in the buffer then get some */
                length=strlen(p)+position+1; /* the new length */
                buffer=(char*)xrealloc(buffer,length+1);
                buffer[position++]=' '; /* put a space before each alias */
                /* add the alias to the output string */
                strcpy(buffer+position,p); position+=strlen(p);
                if (al)
		{
                    for (;p[i+1]==' ' && p[i+1]!='\0' && p[i+1]!='#';i++); /* skip spaces */
                    if (p[i+1]=='\0' || p[i+1]=='#')
		    {
			al=0; continue;
                    }
                    p+=i; /* jump to the next command */
                }
            } while (al);
            if (!skip) message_send_text(c,message_type_info,c,buffer); /* print out the buffer */
            xfree(buffer);
        }
    }
    file_get_line(NULL); // clear file_get_line buffer
    return 0;
}


static int describe_command(t_connection * c, char const * comm)
{
    char * line;
    int    i;

    /* ok. the client requested help for a specific command */
    while ((line=file_get_line(hfd))!=NULL)
    {
        for (i=0;line[i]==' ' && line[i]!='\0';i++); /* skip spaces in front of %command */
        if (line[i]=='%') /* is this a command ? */
        {
            char *p;
            int al;
            /* ok. now we must see if there are any aliases */
            p=line+i;
            do
	    {
                al=0;
                for (i=1;p[i]!=' ' && p[i]!='\0' && p[i]!='#';i++); /* skip command */
                if (p[i]==' ') al=1; /* we have something after the command.. must remember that */
                p[i]='\0'; /* end the string at the end of the command */
                if (strcasecmp(comm,p+1)==0) /* is this the command the user asked for help ? */
                {
                    while ((line=file_get_line(hfd))!=NULL)
                    { /* write everything until we get another % or EOF */
                        for (i=0;line[i]==' ';i++); /* skip spaces in front of a possible % */
                        if (line[i]=='%')
			{
                            break; /* we reached another command */
                        }
                        if (line[0]!='#')
			{ /* is this a whole line comment ? */
                            /* truncate the line when a comment starts */
                            for (;line[i]!='\0' && line[i]!='#';i++);
                            if (line[i]=='#') line[i]='\0';
                            message_send_text(c,message_type_info,c,line);
                        }
                    }
                    return 0;
                }
                if (al)
		{
                    for (;p[i+1]==' ' && p[i+1]!='\0';i++); /* skip spaces */
                    if (p[i+1]=='\0')
		    {
                        al=0; continue;
                    }
                    p+=i; /* jump to the next command */
                }
            } while (al);
        }
    }
    file_get_line(NULL); // clear file_get_line buffer

    return -1;
}

}

}
