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
 *
 * win32 part based on win32-uname.c from libguile
 *
 */
#include "common/setup_before.h"
#ifndef HAVE_UNAME

#include <cstring>
#include <cerrno>
#ifdef WIN32
# include <windows.h>
#endif
#include "uname.h"
#include "common/setup_after.h"


namespace pvpgn
{

extern int uname(struct utsname * buf)
{
    if (!buf)
    {
	errno = EFAULT;
	return -1;
    }

#ifdef WIN32
    {
         OSVERSIONINFO osver;
         
         osver.dwOSVersionInfoSize = sizeof (osver);
         GetVersionEx (&osver);
         
         switch (osver.dwPlatformId)
         {
          case VER_PLATFORM_WIN32_NT: /* NT, Windows 2000 or Windows XP */
               if (osver.dwMajorVersion == 4)
                      strcpy (buf->sysname, "Windows NT4x"); /* NT4x */
               else if (osver.dwMajorVersion <= 3)
                    strcpy (buf->sysname, "Windows NT3x"); /* NT3x */
               else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion < 1)
                    strcpy (buf->sysname, "Windows 2000"); /* 2k */
               else if (osver.dwMajorVersion >= 5)
                    strcpy (buf->sysname, "Windows XP");   /* XP */
               break;

          case VER_PLATFORM_WIN32_WINDOWS: /* Win95, Win98 or WinME */
               if ((osver.dwMajorVersion > 4) || 
                  ((osver.dwMajorVersion == 4) && (osver.dwMinorVersion > 0)))
               {
                   if (osver.dwMinorVersion >= 90)
	                   strcpy (buf->sysname, "Windows ME"); /* ME */
                   else
	                   strcpy (buf->sysname, "Windows 98"); /* 98 */
               }else{
                       strcpy (buf->sysname, "Windows 95"); /* 95 */
               }
               break;

          case VER_PLATFORM_WIN32s: /* Windows 3.x */
               strcpy (buf->sysname, "Windows");
               break;
               
          default:
               std::strcpy(buf->sysname,"Win32");
               break;
          }
    }
#else
    std::strcpy(buf->sysname,"");
#endif
    std::strcpy(buf->nodename,"");
    std::strcpy(buf->release,"");
    std::strcpy(buf->version,"");
    std::strcpy(buf->machine,"");
    std::strcpy(buf->domainname,"");

    return 0;
}

}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
