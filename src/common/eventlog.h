/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
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
#ifndef INCLUDED_EVENTLOG_TYPES
#define INCLUDED_EVENTLOG_TYPES

typedef enum
{
    eventlog_level_none = 0,
    eventlog_level_trace= 1,
    eventlog_level_debug= 2,
    eventlog_level_info = 4,
    eventlog_level_warn = 8,
    eventlog_level_error=16,
    eventlog_level_fatal=32
} t_eventlog_level;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_EVENTLOG_PROTOS
#define INCLUDED_EVENTLOG_PROTOS

#define JUST_NEED_TYPES
#include <stdio.h>
#undef JUST_NEED_TYPES

extern void eventlog_set_debugmode(int debugmode);
extern void eventlog_set(FILE * fp);
extern FILE * eventlog_get(void);
extern int eventlog_open(char const * filename);
extern int eventlog_close(void);
extern void eventlog_clear_level(void);
extern int eventlog_add_level(char const * levelname);
extern int eventlog_del_level(char const * levelname);
extern char const * eventlog_get_levelname_str(t_eventlog_level level);
#ifdef DEBUGMODSTRINGS

extern void eventlog_real(t_eventlog_level level, char const * module, char const * fmt, ...) PRINTF_ATTR(3,4);
# if __STDC_VERSION__+1 >= 199901L
#  define eventlog(l,m,f,args...) eventlog_real(l,"@(" __FILE__ ":" m "@@" __func__ ")@",f,##args)
# else
# if defined(__GNUC__) && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || __GNUC__ > 2)
#   define eventlog(l,m,f,args...) eventlog_real(l,"@(" __FILE__ ":" m "@@" __PRETTY_FUNCTION__ ")@",f,##args)
#  else
#   error "No function macro available, either don't define DEBUGMODSTRINGS or don't use -pedantic"
#  endif
# endif

#else

extern void eventlog(t_eventlog_level level, char const * module, char const * fmt, ...);
extern void eventlog_step(char const * filename, t_eventlog_level level, char const * module, char const * fmt, ...);

#endif

#endif
#endif
