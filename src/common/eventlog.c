/*
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
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
#include <errno.h>
#include "compat/strerror.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strcasecmp.h"
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#else
# ifdef HAVE_VARARGS_H
#  include <varargs.h>
# endif
#endif
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
#include "common/eventlog.h"
#include "common/setup_after.h"

#ifdef WIN32_GUI
# include "../bnetd/winmain.h"
#endif

static FILE *           eventstrm=NULL;
static t_eventlog_level currlevel=eventlog_level_debug|
                                  eventlog_level_info|
                                  eventlog_level_warn|
                                  eventlog_level_error|
                                  eventlog_level_fatal;
/* FIXME: maybe this should be default for win32 */
static int eventlog_debugmode=0;

extern void eventlog_set_debugmode(int debugmode)
{
    eventlog_debugmode = debugmode;
}

extern void eventlog_set(FILE * fp)
{
    eventstrm = fp;
}

extern FILE * eventlog_get(void)
{
  return eventstrm;
}

extern int eventlog_close(void)
{
   fclose(eventstrm);
   return 0;
}

extern int eventlog_open(char const * filename)
{
    FILE * temp;
    
    if (!filename)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL filename");
	return -1;
    }
    
    if (!(temp = fopen(filename,"a")))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"could not open file \"%s\" for appending (fopen: %s)",filename,strerror(errno));
	return -1;
    }
    
    if (eventstrm && eventstrm!=stderr) /* close old one */
	if (fclose(eventstrm)<0)
	{
	    eventstrm = temp;
	    eventlog(eventlog_level_error,__FUNCTION__,"could not close previous logfile after writing (fclose: %s)",strerror(errno));
	    return 0;
	}
    eventstrm = temp;
    
    return 0;
}


extern void eventlog_clear_level(void)
{
    currlevel = eventlog_level_none;
}


extern int eventlog_add_level(char const * levelname)
{
    if (!levelname)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL levelname");
	return -1;
    }
    
    if (strcasecmp(levelname,"trace")==0)
    {
	currlevel |= eventlog_level_trace;
	return 0;
    }
    if (strcasecmp(levelname,"debug")==0)
    {
	currlevel |= eventlog_level_debug;
	return 0;
    }
    if (strcasecmp(levelname,"info")==0)
    {
	currlevel |= eventlog_level_info;
	return 0;
    }
    if (strcasecmp(levelname,"warn")==0)
    {
	currlevel |= eventlog_level_warn;
	return 0;
    }
    if (strcasecmp(levelname,"error")==0)
    {
	currlevel |= eventlog_level_error;
	return 0;
    }
    if (strcasecmp(levelname,"fatal")==0)
    {
	currlevel |= eventlog_level_fatal;
	return 0;
    }
    
    eventlog(eventlog_level_error,__FUNCTION__,"got bad levelname \"%s\"",levelname);
    return -1;
}


extern int eventlog_del_level(char const * levelname)
{
    if (!levelname)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL levelname");
	return -1;
    }
    
    if (strcasecmp(levelname,"trace")==0)
    {
	currlevel &= ~eventlog_level_trace;
	return 0;
    }
    if (strcasecmp(levelname,"debug")==0)
    {
	currlevel &= ~eventlog_level_debug;
	return 0;
    }
    if (strcasecmp(levelname,"info")==0)
    {
	currlevel &= ~eventlog_level_info;
	return 0;
    }
    if (strcasecmp(levelname,"warn")==0)
    {
	currlevel &= ~eventlog_level_warn;
	return 0;
    }
    if (strcasecmp(levelname,"error")==0)
    {
	currlevel &= ~eventlog_level_error;
	return 0;
    }
    if (strcasecmp(levelname,"fatal")==0)
    {
	currlevel &= ~eventlog_level_fatal;
	return 0;
    }
    
    
    eventlog(eventlog_level_error,__FUNCTION__,"got bad levelname \"%s\"",levelname);
    return -1;
}

extern char const * eventlog_get_levelname_str(t_eventlog_level level)
{
  switch (level)
  {
  case eventlog_level_trace:
    return "trace";
  case eventlog_level_debug:
    return "debug";
  case eventlog_level_info:
    return "info ";
  case eventlog_level_warn:
    return "warn ";
  case eventlog_level_error:
    return "error";
  case eventlog_level_fatal:
    return "fatal";
  default:
    return "unknown";
  } 
}


#ifdef DEBUGMODSTRINGS
extern void eventlog_real(t_eventlog_level level, char const * module, char const * fmt, ...)
#else
extern void eventlog(t_eventlog_level level, char const * module, char const * fmt, ...)
#endif
{
    va_list     args;
    char        time_string[EVENT_TIME_MAXLEN];
    struct tm * tmnow;
    time_t      now;
    
    if (!(level&currlevel))
	return;
    if (!eventstrm)
	return;
    
    /* get the time before parsing args */
    time(&now);
    if (!(tmnow = localtime(&now)))
	strcpy(time_string,"?");
    else
	strftime(time_string,EVENT_TIME_MAXLEN,EVENT_TIME_FORMAT,tmnow);
    
    if (!module)
    {
	fprintf(eventstrm,"%s [error] eventlog: got NULL module\n",time_string);
#ifdef WIN32_GUI
        gui_lprintf(eventlog_level_error,"%s [error] eventlog: got NULL module\n",time_string);
#endif
	fflush(eventstrm);
	return;
    }

    if (!fmt)
    {
	fprintf(eventstrm,"%s [error] eventlog: got NULL fmt\n",time_string);
#ifdef WIN32_GUI
        gui_lprintf(eventlog_level_error,"%s [error] eventlog: got NULL fmt\n",time_string);
#endif
	fflush(eventstrm);
	return;
    }
    
    fprintf(eventstrm,"%s [%s] %s: ",time_string,eventlog_get_levelname_str(level),module);
#ifdef WIN32_GUI
    gui_lprintf(level,"%s [%s] %s: ",time_string,eventlog_get_levelname_str(level),module);
#endif

    va_start(args,fmt);

#ifdef HAVE_VPRINTF
    vfprintf(eventstrm,fmt,args);
#ifdef WIN32_GUI
    gui_lvprintf(level,fmt,args);
#endif
#else
# if HAVE__DOPRNT
    _doprnt(fmt,args,eventstrm);
# else
    fprintf(eventstrm,"sorry, vfprintf() and _doprnt() are not available on this system");
# endif
#endif
    va_end(args);
    fprintf(eventstrm,"\n");
#ifdef WIN32_GUI
    gui_lprintf(level,"\n");
#endif

    if (eventlog_debugmode) {
    	printf("%s [%s] %s: ",time_string,eventlog_get_levelname_str(level),module);
    	va_start(args,fmt);
#ifdef HAVE_VPRINTF
    	vprintf(fmt,args);
#else
# if HAVE__DOPRNT
    	_doprnt(fmt,args,stdout);
# else
    	printf("sorry, vfprintf() and _doprnt() are not available on this system");
# endif
#endif
    	va_end(args);
    	printf("\n");
        fflush(stdout);
    }
    fflush(eventstrm);
}


extern void eventlog_step(char const * filename, t_eventlog_level level, char const * module, char const * fmt, ...)
{
    va_list args;
    char        time_string[EVENT_TIME_MAXLEN];
    struct tm * tmnow;
    time_t      now;
    FILE *      fp;
    
    if (!(level&currlevel))
	return;
    if (!eventstrm)
	return;
    
    if (!(fp = fopen(filename, "a")))
	return;
    
    /* get the time before parsing args */
    time(&now);
    if (!(tmnow = localtime(&now)))
	strcpy(time_string,"?");
    else
	strftime(time_string,EVENT_TIME_MAXLEN,EVENT_TIME_FORMAT,tmnow);
    
    if (!module)
    {
	fprintf(fp,"%s [error] eventlog_step: got NULL module\n",time_string);
	fclose(fp);
	return;
    }
    if (!fmt)
    {
	fprintf(fp,"%s [error] eventlog_step: got NULL fmt\n",time_string);
	fclose(fp);
	return;
    }
    
    fprintf(fp,"%s [%s] %s: ",time_string,eventlog_get_levelname_str(level),module);
    va_start(args,fmt);
#ifdef HAVE_VPRINTF
    vfprintf(fp,fmt,args);
#else
# if HAVE__DOPRNT
    _doprnt(fmt,args,fp);
# else
    fprintf(fp,"sorry, vfprintf() and _doprnt() are not available on this system");
# endif
#endif
    va_end(args);
    fprintf(fp,"\n");
    fclose(fp);
}
