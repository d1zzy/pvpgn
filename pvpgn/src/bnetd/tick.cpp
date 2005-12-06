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
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
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
#include "compat/gettimeofday.h"
#include "common/eventlog.h"
#include "tick.h"
#include "common/setup_after.h"


/*
 * This routine returns the number of miliseconds that have passed since one second
 * before it is first called. This is used for timing fields in some packets.
 */
extern unsigned int get_ticks(void)
{
    static int first=1;
    static long beginsec;
    struct timeval tv;
    
    if (gettimeofday(&tv,NULL)<0)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"could not get time (gettimeofday: %s)",pstrerror(errno));
	return 0;
    }
    if (first)
    {
	beginsec = tv.tv_sec-1;
	first = 0;
    }
    
    return (unsigned int)((tv.tv_sec-beginsec)*1000+tv.tv_usec/1000);
}
