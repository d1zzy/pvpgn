/*
  * Abstraction API/layer for the various ways PvPGN can inspect sockets state
  * 2003 (C) dizzy@roedu.net
  *
  * Code is based on the ideas found in thttpd project.
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
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "common/eventlog.h"
#include "fdwatch.h"
#ifdef HAVE_SELECT
#include "fdwatch_select.h"
#endif
#ifdef HAVE_POLL
#include "fdwatch_poll.h"
#endif
#ifdef HAVE_KQUEUE
#include "fdwatch_kqueue.h"
#endif
#ifdef HAVE_EPOLL
#include "fdwatch_epoll.h"
#endif
#include "common/rlimit.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

int fdw_maxfd;
int *fdw_rw = NULL;
void ** fdw_data = NULL;
fdwatch_handler *fdw_hnd;

static t_fdw_backend * fdw = NULL;

extern int fdwatch_init(void)
{
    fdw_maxfd = get_socket_limit();
    if (fdw_maxfd < 1) {
	eventlog(eventlog_level_fatal, __FUNCTION__, "too few sockets available (%d)",fdw_maxfd);
	return -1;
    }

    fdw_rw = xmalloc(sizeof(int) * fdw_maxfd);
    fdw_data = xmalloc(sizeof(void *) * fdw_maxfd);
    fdw_hnd = xmalloc(sizeof(fdwatch_handler) * fdw_maxfd);

    /* initilize the arrays (poisoning) */
    memset(fdw_rw, 0, sizeof(int) * fdw_maxfd);
    memset(fdw_data, 0, sizeof(void*) * fdw_maxfd);
    memset(fdw_hnd, 0, sizeof(fdwatch_handler) * fdw_maxfd);

#ifdef HAVE_EPOLL
    fdw = &fdw_epoll;
    if (!fdw->init(fdw_maxfd)) goto ok;
#endif
#ifdef HAVE_KQUEUE
    fdw = &fdw_kqueue;
    if (!fdw->init(fdw_maxfd)) goto ok;
#endif
#ifdef HAVE_POLL
    fdw = &fdw_poll;
    if (!fdw->init(fdw_maxfd)) goto ok;
    goto ok;
#endif
#ifdef HAVE_SELECT
    fdw = &fdw_select;
    if (!fdw->init(fdw_maxfd)) goto ok;
#endif

    eventlog(eventlog_level_fatal, __FUNCTION__, "Found no working fdwatch layer");
    fdw = NULL;
    fdwatch_close();
    return -1;

ok:
    return 0;
}

extern int fdwatch_close(void)
{
    if (fdw) { fdw->close(); fdw = NULL; }
    if (fdw_rw) { xfree((void*)fdw_rw); fdw_rw = NULL; }
    if (fdw_data) { xfree((void*)fdw_data); fdw_data = NULL; }
    if (fdw_hnd) { xfree((void*)fdw_hnd); fdw_hnd = NULL; }

    return 0;
}

extern int fdwatch_add_fd(int fd, t_fdwatch_type rw, fdwatch_handler h, void *data)
{
    if (fdw->add_fd(fd, rw)) return -1;
    fdw_rw[fd] = rw;
    fdw_data[fd] = data;
    fdw_hnd[fd] = h;

    return 0;
}

extern int fdwatch_update_fd(int fd, t_fdwatch_type rw)
{
    if (fdw->add_fd(fd, rw)) return -1;
    fdw_rw[fd] = rw;

    return 0;
}

extern int fdwatch_del_fd(int fd)
{
    fdw_rw[fd] = fdwatch_type_none;
    fdw_data[fd] = NULL;
    fdw_hnd[fd] = NULL;
    fdw->del_fd(fd);

    return 0;
}

extern int fdwatch(long timeout_msec)
{
    return fdw->watch(timeout_msec);
}

extern void fdwatch_handle(void)
{
    fdw->handle();
}
