/*
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
#include "common/setup_before.h"
#ifndef HAVE_MEMCPY

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef HAVE_BCOPY
# ifdef HAVE_STRING_H
#  include <string.h>
# endif
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#endif
#include "memcpy.h"
#include "common/setup_after.h"

extern void * memcpy(void * dest, void const * src, unsigned long n)
{
#ifdef HAVE_BCOPY
    bcopy(src,dest,n);
    return dest;
#else
/* very slow, but we don't care */
    unsigned char * td=dest;
    unsigned char * ts=src;
    unsigned long   i;
    
    if (!td || !ts)
	return NULL;
    
    for (i=0; i<n; i++)
	td[i] = ts[i];
    return dest;
#endif
}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
