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
#ifndef HAVE_UNAME

#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <errno.h>
#include "uname.h"
#include "common/setup_after.h"


extern int uname(struct utsname * buf)
{
    if (!buf)
    {
	errno = EFAULT;
	return -1;
    }
    
#ifdef WIN32
/* FIXME: distinguish between: */
/*   Microsoft Windows 3.1-32s/3.11-32s/95/95SP1/95A/95B/95OSR2/95OSR2.1/95OSR2.5/95B+MSIE/98/98OEM/98SE/2000ME "New Technology" 3.1/3.5 Server/3.51 Server/4.0 Server/4.0SP1 Server/4.0SP2 Server/4.0SP3 Server/4.0SP4 Server/4.0SP6 Server/3.5 Workstation/3.51 Workstation/4.0 Workstation/4.0SP1 Workstation/4.0SP2 Workstation/4.0SP3 Workstation/4.0SP4 Workstation/4.0SP6 Workstation/"2000 Professional Edition"/"2000 Server"/"2000 Advanced Server"/"Windows 2000 Terminal Server" Win CE/"Windows Powered"/"Pocket OS" 1.0/1.1/2.0/2.1 */
/* maybe report the build number too */
    strcpy(buf->sysname,"Win32");
#else
    strcpy(buf->sysname,"");
#endif
    strcpy(buf->nodename,"");
    strcpy(buf->release,"");
    strcpy(buf->version,"");
    strcpy(buf->machine,"");
    strcpy(buf->domainname,"");
    
    return 0;
}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
