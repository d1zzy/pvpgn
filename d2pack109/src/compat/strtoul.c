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
#ifndef HAVE_STRTOUL

#include "strtoul.h"
#include "common/setup_after.h"


extern unsigned long strtoul(char const * str, char * * endptr, int base)
{
    unsigned int  pos;
    unsigned int  i;
    unsigned long val;
    
    if (!str)
	return 0; /* EINVAL */
    
    for (pos=0; str[pos]==' ' || str[pos]=='\t'; pos++);
    if (str[pos]=='-' || str[pos]=='+')
        pos++;
    if ((base==0 || base==16) && str[pos]=='0' && (str[pos+1]=='x' || str[pos+1]=='X'))
    {
	base = 16;
	pos += 2; /* skip 0x prefix */
    }
    else if ((base==0 || base==8) && str[pos]=='0')
	{
	    base = 8;
	    pos += 1;
	}
    else if (base==0)
    	{
    	    base = 10;	
    	}
    	
    if (base<2 || base>16) /* sorry, not complete emulation (should do up to 36) */
	return 0; /* EINVAL */
    
    val = 0;
    for (i=pos; str[i]!='\0'; i++)
    {
        val *= base;
	
	/* we could assume ASCII and subtraction but this isn't too hard */
	if (str[i]=='0')
	    val += 0;
	else if (str[i]=='1')
	    val += 1;
	else if (base>2 && str[i]=='2')
	    val += 2;
	else if (base>3 && str[i]=='3')
	    val += 3;
	else if (base>4 && str[i]=='4')
	    val += 4;
	else if (base>5 && str[i]=='5')
	    val += 5;
	else if (base>6 && str[i]=='6')
	    val += 6;
	else if (base>7 && str[i]=='7')
	    val += 7;
	else if (base>8 && str[i]=='8')
	    val += 8;
	else if (base>9 && str[i]=='9')
	    val += 9;
	else if (base>10 && (str[i]=='a' || str[i]=='A'))
	    val += 10;
	else if (base>11 && (str[i]=='b' || str[i]=='B'))
	    val += 11;
	else if (base>12 && (str[i]=='c' || str[i]=='C'))
	    val += 12;
	else if (base>13 && (str[i]=='d' || str[i]=='D'))
	    val += 13;
	else if (base>14 && (str[i]=='e' || str[i]=='E'))
	    val += 14;
	else if (base>15 && (str[i]=='f' || str[i]=='F'))
	    val += 15;
	else
	    break;
    }
    
    if (endptr)
	*endptr = (void *)&str[i]; /* avoid warning */
    
    return val;
}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
