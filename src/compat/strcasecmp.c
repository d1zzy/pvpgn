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
#ifndef HAVE_STRCASECMP

#include <ctype.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "strcasecmp.h"
#include "common/setup_after.h"


extern int strcasecmp(char const * str1, char const * str2)
{
    unsigned int i;
    int          a;
    int          b;
    
    if (!str1 || !str2)
        return -1;
    
    /* some versions of tolower() break when given already lowercase characters */
    for (i=0; str1[i]!='\0' && str2[i]!='\0'; i++)
    {
	if (isupper((int)str1[i]))
	    a = (int)tolower((int)str1[i]);
	else
	    a = (int)str1[i];
	
	if (isupper((int)str2[i]))
	    b = (int)tolower((int)str2[i]);
	else
	    b = (int)str2[i];
	
	if (a<b)
	    return -1;
	if (a>b)
	    return +1;
    }
    
    if (str1[i]!='\0')
	return -1;
    if (str2[i]!='\0')
	return +1;
    return 0;
}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
