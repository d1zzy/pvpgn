/*
  * Abstraction API/layer for the various ways PvPGN can inspect sockets state
  * 2003 (C) 
  *
  * Code is based on the ideas found in thttpd project.
  *
  * Linux epoll(4) based backend
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
#ifdef HAVE_EPOLL

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef HAVE_SYS_EPOLL_H
# include "compat/uint.h"
# include <sys/epoll.h>
#endif
#include "fdwatch.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

static int sr;
static int epfd;
static struct epoll_event *epevents  = NULL;	/* events to investigate */
static struct epoll_event tmpev;

static int fdw_epoll_init(int nfds);
static int fdw_epoll_close(void);
static int fdw_epoll_add_fd(int idx, unsigned rw);
static int fdw_epoll_del_fd(int idx);
static int fdw_epoll_watch(long timeout_msecs);
static void fdw_epoll_handle(void);

t_fdw_backend fdw_epoll = {
    fdw_epoll_init,
    fdw_epoll_close,
    fdw_epoll_add_fd,
    fdw_epoll_del_fd,
    fdw_epoll_watch,
    fdw_epoll_handle
};

static int fdw_epoll_init(int nfds)
{
    if ((epfd = epoll_create(nfds)) < 0)
	return -1;
    epevents = (struct epoll_event *) xmalloc(sizeof(struct epoll_event) * nfds);

    memset(epevents, 0, sizeof(struct epoll_event) * nfds);
    sr = 0;

    eventlog(eventlog_level_info, __FUNCTION__, "fdwatch epoll() based layer initialized (max %d sockets)", nfds);
    return 0;
}

static int fdw_epoll_close(void)
{
    if (epevents != NULL)
	xfree((void *) epevents);
    sr = 0;

    return 0;
}

static int fdw_epoll_add_fd(int idx, unsigned rw)
{
    int op;
//    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d rw: %d", fd, rw);

    tmpev.events = 0;
    if (rw & fdwatch_type_read)
	tmpev.events |= EPOLLIN;
    if (rw & fdwatch_type_write)
	tmpev.events |= EPOLLOUT;

    if (fdw_rw(fdw_fds + idx)) op = EPOLL_CTL_MOD;
    else op = EPOLL_CTL_ADD;

    tmpev.data.fd = idx;
    if (epoll_ctl(epfd, op, fdw_fd(fdw_fds + idx), &tmpev)) {
	eventlog(eventlog_level_error, __FUNCTION__, "got error from epoll_ctl()");
	return -1;
    }

    return 0;
}

static int fdw_epoll_del_fd(int idx)
{
//    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d", fd);
    if (sr > 0) 
	eventlog(eventlog_level_error, __FUNCTION__, "BUG: called while still handling sockets");

    if (fdw_rw(fdw_fds + idx)) {
	tmpev.events = 0;
	tmpev.data.fd = idx;
	if (epoll_ctl(epfd, EPOLL_CTL_DEL, fdw_fd(fdw_fds + idx), &tmpev)) {
	    eventlog(eventlog_level_error, __FUNCTION__, "got error from epoll_ctl()");
	    return -1;
	}
    }

    return 0;
}

static int fdw_epoll_watch(long timeout_msec)
{
    return (sr = epoll_wait(epfd, epevents, fdw_maxcons, timeout_msec));
}

static void fdw_epoll_handle(void)
{
    struct epoll_event *ev;
    t_fdwatch_fd *cfd;

//    eventlog(eventlog_level_trace, __FUNCTION__, "called");
    for (ev = epevents; sr; sr--, ev++)
    {
//      eventlog(eventlog_level_trace, __FUNCTION__, "checking %d ident: %d read: %d write: %d", i, kqevents[i].ident, kqevents[i].filter & EVFILT_READ, kqevents[i].filter & EVFILT_WRITE);
        cfd = fdw_fds + ev->data.fd;

        if (fdw_rw(cfd) & fdwatch_type_read && ev->events & (EPOLLIN | EPOLLERR | EPOLLHUP))
            if (fdw_hnd(cfd) (fdw_data(cfd), fdwatch_type_read) == -2)
		continue;

        if (fdw_rw(cfd) & fdwatch_type_write && ev->events & (EPOLLOUT | EPOLLERR | EPOLLHUP))
            fdw_hnd(cfd) (fdw_data(cfd), fdwatch_type_write);
    }
    sr = 0;
}

#endif				/* HAVE_EPOLL */
