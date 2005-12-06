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
#ifndef HAVE_STRFTIME

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "strftime.h"
#include "common/setup_after.h"


/* We are not even trying to copy the functionality.
   We are just trying to get a timestamp. */
extern int strftime(char * buf, int bufsize, char const * fmt, struct tm const * tm)
{
    if (!buf || !fmt || !tm)
        return 0;
    
    if (bufsize>24)
	sprintf(buf,"%.4d %.2d %.2d %02.2d:%02.2d:%02.2d",
		tm->tm_year,
		tm->tm_mon,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec);
    else if (bufsize>10)
	sprintf(buf,"%02.2d:%02.2d:%02.2d",
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec);
    else
	buf[0] = '\0';
    
    return strlen(buf);
}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
