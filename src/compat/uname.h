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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef INCLUDED_UNAME_H
#define INCLUDED_UNAME_H

#ifdef HAVE_UNAME
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#else

#ifndef SYS_NMLN
#define SYS_NMLN 64
#endif

namespace pvpgn
{
	struct utsname
	{
		char sysname[SYS_NMLN + 1];
		char nodename[SYS_NMLN + 1];
		char release[SYS_NMLN + 1];
		char version[SYS_NMLN + 1];
		char machine[SYS_NMLN + 1];
		char domainname[SYS_NMLN + 1];
	};

	int uname(struct utsname* buf);
}

#endif // HAVE_UNAME
#endif // INCLUDED_UNAME_H