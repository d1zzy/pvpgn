/*
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
#include <stdio.h>
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
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#include "common/eventlog.h"
#include "common/bn_type.h"
#include "common/setup_after.h"


/* file descriptor limit has different name on BSD and Linux */
#ifdef RLIMIT_NOFILE
# define RLIM_NUMFILES RLIMIT_NOFILE
#else
# ifdef RLIMIT_OFILE
#  define RLIM_NUMFILES RLIMIT_OFILE
# endif
#endif

extern int get_socket_limit(void)
{
	int socklimit = 0;
#ifdef HAVE_GETRLIMIT
	struct rlimit rlim;
	if(getrlimit(RLIM_NUMFILES, &rlim) < 0)
		eventlog(eventlog_level_error, __FUNCTION__, "getrlimit returned error: %s", pstrerror(errno));
	socklimit = rlim.rlim_cur;
#else
	/* FIXME: WIN32: somehow get WSAData win32 socket limit here */
#endif

#if !(defined HAVE_POLL || defined HAVE_KQUEUE || defined HAVE_EPOLL)
	if(!socklimit || FD_SETSIZE < socklimit)
		socklimit = FD_SETSIZE;
#endif

	/* make socket limit smaller than file limit to make sure log files,
	   db connections and save files will still work */
	socklimit -= 64;

	return socklimit;
}
