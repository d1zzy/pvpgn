/*
  * Abstraction API/layer for the various ways PvPGN can inspect sockets state
  * 2003 (C) dizzy@roedu.net
  *
  * Code is based on the ideas found in thttpd project.
  *
  * *BSD kqueue() based backend
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
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_EVENT_H
# include <sys/event.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include "fdwatch.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

#ifdef HAVE_KQUEUE
static int sr;
static int kq;
static struct kevent *kqchanges = NULL;		/* changes to make to kqueue */
static struct kevent *kqevents = NULL;		/* events to investigate */
static int *fdw_rridx, *fdw_wridx;
static unsigned nofds;

static int fdw_kqueue_init(int nfds);
static int fdw_kqueue_close(void);
static int fdw_kqueue_add_fd(int fd, t_fdwatch_type rw);
static int fdw_kqueue_del_fd(int fd);
static int fdw_kqueue_watch(long timeout_msecs);
static void fdw_kqueue_handle(void);

t_fdw_backend fdw_kqueue = {
    fdw_kqueue_init,
    fdw_kqueue_close,
    fdw_kqueue_add_fd,
    fdw_kqueue_del_fd,
    fdw_kqueue_watch,
    fdw_kqueue_handle
};

static int fdw_kqueue_init(int nfds)
{
    int i;

    if ((kq = kqueue()) == -1)
	return -1;
    kqevents = (struct kevent *) xmalloc(sizeof(struct kevent) * nfds);
    kqchanges = (struct kevent *) xmalloc(sizeof(struct kevent) * nfds * 2);
    fdw_rridx = (int *) xmalloc(sizeof(int) * nfds);
    fdw_wridx = (int *) xmalloc(sizeof(int) * nfds);

    memset(kqchanges, 0, sizeof(struct kevent) * nfds);
    for (i = 0; i < nfds; i++)
    {
	fdw_rridx[i] = -1;
	fdw_wridx[i] = -1;
    }
    sr = 0;
    nofds = 0;

    eventlog(eventlog_level_info, __FUNCTION__, "fdwatch kqueue() based layer initialized (max %d sockets)", nfds);
    return 0;
}

static int fdw_kqueue_close(void)
{
    if (fdw_rridx) { xfree((void *) fdw_rridx); fdw_rridx = NULL; }
    if (fdw_wridx) { xfree((void *) fdw_wridx); fdw_wridx = NULL; }
    if (kqchanges) { xfree((void *) kqchanges); kqchanges = NULL; }
    if (kqevents) { xfree((void *) kqevents); kqevents = NULL; }
    sr = 0;
    nofds = 0;

    return 0;
}

static int fdw_kqueue_add_fd(int fd, t_fdwatch_type rw)
{
    static int ridx;

/*    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d rw: %d", fd, rw); */

    /* adding read event filter */
    if (!(fdw_rw[fd] & fdwatch_type_read) && rw & fdwatch_type_read)
    {
	if (fdw_rridx[fd] >= 0 && fdw_rridx[fd] < nofds && kqchanges[fdw_rridx[fd]].ident == fd)
	{
	    ridx = fdw_rridx[fd];
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "updating change event (read) fd on %d", ridx); */
	} else {
	    ridx = nofds++;
	    fdw_rridx[fd] = ridx;
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "adding new change event (read) fd on %d", ridx); */
	}
	EV_SET(kqchanges + ridx, fd, EVFILT_READ, EV_ADD, 0, 0, 0);
    } 
    else if (fdw_rw[fd] & fdwatch_type_read && !( rw & fdwatch_type_read))
    {
	if (fdw_rridx[fd] >= 0 && fdw_rridx[fd] < nofds && kqchanges[fdw_rridx[fd]].ident == fd)
	{
	    ridx = fdw_rridx[fd];
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "updating change event (read) fd on %d", ridx); */
	} else {
	    ridx = nofds++;
	    fdw_rridx[fd] = ridx;
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "adding new change event (read) fd on %d", ridx); */
	}
	EV_SET(kqchanges + ridx, fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
    }

    /* adding write event filter */
    if (!(fdw_rw[fd] & fdwatch_type_write) && rw & fdwatch_type_write)
    {
	if (fdw_wridx[fd] >= 0 && fdw_wridx[fd] < nofds && kqchanges[fdw_wridx[fd]].ident == fd)
	{
	    ridx = fdw_wridx[fd];
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "updating change event (write) fd on %d", ridx); */
	} else {
	    ridx = nofds++;
	    fdw_wridx[fd] = ridx;
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "adding new change event (write) fd on %d", ridx); */
	}
	EV_SET(kqchanges + ridx, fd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
    }
    else if (fdw_rw[fd] & fdwatch_type_write && !(rw & fdwatch_type_write))
    {
	if (fdw_wridx[fd] >= 0 && fdw_wridx[fd] < nofds && kqchanges[fdw_wridx[fd]].ident == fd)
	{
	    ridx = fdw_wridx[fd];
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "updating change event (write) fd on %d", ridx); */
	} else {
	    ridx = nofds++;
	    fdw_wridx[fd] = ridx;
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "adding new change event (write) fd on %d", ridx); */
	}
	EV_SET(kqchanges + ridx, fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
    }

    return 0;
}

static int fdw_kqueue_del_fd(int fd)
{
/*    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d", fd); */
    if (sr > 0) 
	eventlog(eventlog_level_error, __FUNCTION__, "BUG: called while still handling sockets");

    /* the last event changes about this fd has not yet been sent to kernel */
    if (fdw_rw[fd] & fdwatch_type_read &&
        nofds && fdw_rridx[fd] >= 0 && fdw_rridx[fd] < nofds && 
	kqchanges[fdw_rridx[fd]].ident == fd)
    {
	nofds--;
	if (fdw_rridx[fd] < nofds)
	{
	    int tmp;

	    tmp = kqchanges[nofds].ident;
	    if (kqchanges[nofds].filter == EVFILT_READ && 
		fdw_rridx[tmp] == nofds)
	    {
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "not last, moving %d", kqchanges[rnfds].ident); */
		fdw_rridx[tmp] = fdw_rridx[fd];
		memcpy(kqchanges + fdw_rridx[fd], kqchanges + nofds, sizeof(struct kevent));
	    }

	    if (kqchanges[nofds].filter == EVFILT_WRITE &&
		fdw_wridx[tmp] == nofds)
	    {
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "not last, moving %d", kqchanges[rnfds].ident); */
		fdw_wridx[tmp] = fdw_rridx[fd];
		memcpy(kqchanges + fdw_rridx[fd], kqchanges + nofds, sizeof(struct kevent));
	    }
	}
	fdw_rridx[fd] = -1;
    }

    if (fdw_rw[fd] & fdwatch_type_write &&
        nofds && fdw_wridx[fd] >= 0 && fdw_wridx[fd] < nofds && 
	kqchanges[fdw_wridx[fd]].ident == fd)
    {
	nofds--;
	if (fdw_wridx[fd] < nofds)
	{
	    int tmp;

	    tmp = kqchanges[nofds].ident;
	    if (kqchanges[nofds].filter == EVFILT_READ && 
		fdw_rridx[tmp] == nofds)
	    {
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "not last, moving %d", kqchanges[rnfds].ident); */
		fdw_rridx[tmp] = fdw_wridx[fd];
		memcpy(kqchanges + fdw_wridx[fd], kqchanges + nofds, sizeof(struct kevent));
	    }

	    if (kqchanges[nofds].filter == EVFILT_WRITE &&
		fdw_wridx[tmp] == nofds)
	    {
/*	    eventlog(eventlog_level_trace, __FUNCTION__, "not last, moving %d", kqchanges[rnfds].ident); */
		fdw_wridx[tmp] = fdw_wridx[fd];
		memcpy(kqchanges + fdw_wridx[fd], kqchanges + nofds, sizeof(struct kevent));
	    }
	}
	fdw_wridx[fd] = -1;
    }

/* here we presume the calling code does close() on the socket and if so
 * it is automatically removed from any kernel kqueues */

    return 0;
}

static int fdw_kqueue_watch(long timeout_msec)
{
    static struct timespec ts;

    ts.tv_sec = timeout_msec / 1000L;
    ts.tv_nsec = (timeout_msec % 1000L) * 1000000L;
    sr = kevent(kq, nofds > 0 ? kqchanges : NULL, nofds, kqevents, fdw_maxfd, &ts);
    nofds = 0;
    return sr;
}

static void fdw_kqueue_handle(void)
{
    register unsigned i;

/*    eventlog(eventlog_level_trace, __FUNCTION__, "called"); */
    for (i = 0; i < sr; i++)
    {
/*      eventlog(eventlog_level_trace, __FUNCTION__, "checking %d ident: %d read: %d write: %d", i, kqevents[i].ident, kqevents[i].filter & EVFILT_READ, kqevents[i].filter & EVFILT_WRITE); */

	if (fdw_rw[kqevents[i].ident] & fdwatch_type_read && kqevents[i].filter == EVFILT_READ)
	    if (fdw_hnd[kqevents[i].ident] (fdw_data[kqevents[i].ident], fdwatch_type_read) == -2)
		continue;

	if (fdw_rw[kqevents[i].ident] & fdwatch_type_write && kqevents[i].filter == EVFILT_WRITE)
	    fdw_hnd[kqevents[i].ident] (fdw_data[kqevents[i].ident], fdwatch_type_write);

    }
    sr = 0;
}

#endif				/* HAVE_KQUEUE */
