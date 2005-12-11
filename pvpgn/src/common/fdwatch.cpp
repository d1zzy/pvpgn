/*
  * Abstraction API/layer for the various ways PvPGN can inspect sockets state
  * 2003 (C)
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
#define FDWATCH_BACKEND
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

namespace pvpgn
{

unsigned fdw_maxcons;
t_fdwatch_fd *fdw_fds = NULL;

static t_fdw_backend * fdw = NULL;
static DECLARE_ELIST_INIT(freelist);
static DECLARE_ELIST_INIT(uselist);

extern int fdwatch_init(int maxcons)
{
    unsigned i;
    int maxsys;

    maxsys = get_socket_limit();
    if (maxsys > 0) maxcons = (maxcons < maxsys) ? maxcons : maxsys;
    if (maxcons < 32) {
	eventlog(eventlog_level_fatal, __FUNCTION__, "too few sockets available (%d)",maxcons);
	return -1;
    }
    fdw_maxcons = maxcons;

    fdw_fds = (t_fdwatch_fd*)xmalloc(sizeof(t_fdwatch_fd) * fdw_maxcons);
    memset(fdw_fds, 0, sizeof(t_fdwatch_fd) * fdw_maxcons);
    /* add all slots to the freelist */
    for(i = 0; i < fdw_maxcons; i++)
	elist_add_tail(&freelist,&(fdw_fds[i].freelist));

#ifdef HAVE_EPOLL
    fdw = &fdw_epoll;
    if (!fdw->init(fdw_maxcons)) goto ok;
#endif
#ifdef HAVE_KQUEUE
    fdw = &fdw_kqueue;
    if (!fdw->init(fdw_maxcons)) goto ok;
#endif
#ifdef HAVE_POLL
    fdw = &fdw_poll;
    if (!fdw->init(fdw_maxcons)) goto ok;
    goto ok;
#endif
#ifdef HAVE_SELECT
    fdw = &fdw_select;
    if (!fdw->init(fdw_maxcons)) goto ok;
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
    if (fdw_fds) { xfree((void*)fdw_fds); fdw_fds = NULL; }
    elist_init(&freelist);
    elist_init(&uselist);

    return 0;
}

extern int fdwatch_add_fd(int fd, unsigned rw, fdwatch_handler h, void *data)
{
    t_fdwatch_fd *cfd;

    if (elist_empty(&freelist)) return -1;	/* max sockets reached */

    cfd = elist_entry(elist_next(&freelist),t_fdwatch_fd,freelist);
    fdw_fd(cfd) = fd;

    if (fdw->add_fd(fdw_idx(cfd), rw)) return -1;

    /* add it to used sockets list, remove it from free list */
    elist_add_tail(&uselist,&cfd->uselist);
    elist_del(&cfd->freelist);

    fdw_rw(cfd) = rw;
    fdw_data(cfd) = data;
    fdw_hnd(cfd) = h;

    return fdw_idx(cfd);
}

extern int fdwatch_update_fd(int idx, unsigned rw)
{
    if (idx<0 || idx>=fdw_maxcons) {
	eventlog(eventlog_level_error,__FUNCTION__,"out of bounds idx [%d] (max: %d)",idx, fdw_maxcons);
	return -1;
    }
    /* do not allow completly reset the access because then backend codes
     * can get confused */
    if (!rw) {
	eventlog(eventlog_level_error,__FUNCTION__,"tried to reset rw, not allowed");
	return -1;
    }

    if (!fdw_rw(fdw_fds + idx)) {
	eventlog(eventlog_level_error,__FUNCTION__,"found reseted rw");
	return -1;
    }

    if (fdw->add_fd(idx, rw)) return -1;
    fdw_rw(&fdw_fds[idx]) = rw;

    return 0;
}

extern int fdwatch_del_fd(int idx)
{
    t_fdwatch_fd *cfd;

    if (idx<0 || idx>=fdw_maxcons) {
	eventlog(eventlog_level_error,__FUNCTION__,"out of bounds idx [%d] (max: %d)",idx, fdw_maxcons);
	return -1;
    }

    cfd = fdw_fds + idx;
    if (!fdw_rw(cfd)) {
	eventlog(eventlog_level_error,__FUNCTION__,"found reseted rw");
	return -1;
    }

    fdw->del_fd(idx);

    /* remove it from uselist, add it to freelist */
    elist_del(&cfd->uselist);
    elist_add_tail(&freelist,&cfd->freelist);

    fdw_fd(cfd) = 0;
    fdw_rw(cfd) = 0;
    fdw_data(cfd) = NULL;
    fdw_hnd(cfd) = NULL;

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

extern void fdwatch_traverse(t_fdw_cb cb, void *data)
{
    t_elist *curr;

    elist_for_each(curr,&uselist)
    {
	if (cb(elist_entry(curr,t_fdwatch_fd,uselist),data)) break;
    }
}

}
