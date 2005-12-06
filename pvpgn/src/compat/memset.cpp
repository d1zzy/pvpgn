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
#ifndef HAVE_MEMSET

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#include "memset.h"
#include "common/setup_after.h"

/* very slow, but we don't care */
extern void * memset(void * dest, int c, unsigned long n)
{
    unsigned char * temp=dest;
    unsigned long   i;
    
    if (!temp)
	return NULL;
    
    for (i=0; i<n; i++)
	temp[i] = (unsigned char)c;
    return dest;
}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
