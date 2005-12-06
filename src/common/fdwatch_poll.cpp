/*
  * Abstraction API/layer for the various ways PvPGN can inspect sockets state
  * 2003 (C) 
  *
  * Code is based on the ideas found in thttpd project.
  *
  * poll(2) based backend
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
#ifdef HAVE_POLL_H
# include <poll.h>
#else
# ifdef HAVE_SYS_POLL_H
#  include <sys/poll.h>
# endif
#endif
#include "fdwatch.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

#ifdef HAVE_POLL
static int sr;
static struct pollfd *fds = NULL; /* working set */
static int *_rridx = NULL;
static int *_ridx = NULL;
static unsigned nofds;

static int fdw_poll_init(int nfds);
static int fdw_poll_close(void);
static int fdw_poll_add_fd(int idx, unsigned rw);
static int fdw_poll_del_fd(int idx);
static int fdw_poll_watch(long timeout_msecs);
static void fdw_poll_handle(void);

t_fdw_backend fdw_poll = {
    fdw_poll_init,
    fdw_poll_close,
    fdw_poll_add_fd,
    fdw_poll_del_fd,
    fdw_poll_watch,
    fdw_poll_handle
};

static int fdw_poll_init(int nfds)
{
    int i;

    _ridx = (int*)xmalloc(sizeof(int) * nfds);
    fds = (struct pollfd*)xmalloc(sizeof(struct pollfd) * nfds);
    _rridx = (int*)xmalloc(sizeof(int) * nfds);

    memset(fds, 0, sizeof(struct pollfd) * nfds);
    memset(_rridx, 0, sizeof(int) * nfds);
/* I would use a memset with 255 but that is dirty and doesnt gain us anything */
    for(i = 0; i < nfds; i++) _ridx[i] = -1;
    nofds = sr = 0;

    eventlog(eventlog_level_info, __FUNCTION__, "fdwatch poll() based layer initialized (max %d sockets)", nfds);
    return 0;
}

static int fdw_poll_close(void)
{
    if (fds) { xfree((void *)fds); fds = NULL; }
    if (_ridx) { xfree((void *)_ridx); _ridx = NULL; }
    if (_rridx) { xfree((void *)_rridx); _rridx = NULL; }
    nofds = sr = 0;

    return 0;
}

static int fdw_poll_add_fd(int idx, unsigned rw)
{
    static int ridx;

//    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d rw: %d", fd, rw);
    if (_ridx[idx] < 0) {
	ridx = nofds++;
	fds[ridx].fd = fdw_fd(fdw_fds + idx);
	_ridx[idx] = ridx;
	_rridx[ridx] = idx;
//	eventlog(eventlog_level_trace, __FUNCTION__, "adding new fd on %d", ridx);
    } else {
	if (fds[_ridx[idx]].fd != fdw_fd(fdw_fds + idx)) {
	    eventlog(eventlog_level_error,__FUNCTION__,"BUG: found existent poll_fd entry for same idx with different fd");
	    return -1;
	}
	ridx = _ridx[idx];
//	eventlog(eventlog_level_trace, __FUNCTION__, "updating fd on %d", ridx);
    }

    fds[ridx].events = 0;
    if (rw & fdwatch_type_read) fds[ridx].events |= POLLIN;
    if (rw & fdwatch_type_write) fds[ridx].events |= POLLOUT;

    return 0;
}

static int fdw_poll_del_fd(int idx)
{
//    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d", fd);
    if (_ridx[idx] < 0 || !nofds) return -1;
    if (sr > 0) 
	eventlog(eventlog_level_error, __FUNCTION__, "BUG: called while still handling sockets");

    /* move the last entry to the deleted one and decrement nofds count */
    nofds--;
    if (_ridx[idx] < nofds) {
//	eventlog(eventlog_level_trace, __FUNCTION__, "not last, moving %d", tfds[nofds].fd);
	_ridx[_rridx[nofds]] = _ridx[idx];
	_rridx[_ridx[idx]] = _rridx[nofds];
	memcpy(fds + _ridx[idx], fds + nofds, sizeof(struct pollfd));
    }
    _ridx[idx] = -1;

    return 0;
}

static int fdw_poll_watch(long timeout_msec)
{
    return (sr = poll(fds, nofds, timeout_msec));
}

static void fdw_poll_handle(void)
{
    register unsigned i;
    int changed;
    t_fdwatch_fd *cfd;

    for(i = 0; i < nofds && sr; i++) {
	changed = 0;
	cfd = fdw_fds + _rridx[i];

	if (fdw_rw(cfd) & fdwatch_type_read && 
	    fds[i].revents & (POLLIN  | POLLERR | POLLHUP | POLLNVAL))
	{
	    if (fdw_hnd(cfd)(fdw_data(cfd), fdwatch_type_read) == -2) {
		sr--;
		continue;
	    }
	    changed = 1;
	}

	if (fdw_rw(cfd) & fdwatch_type_write && 
	    fds[i].revents & (POLLOUT  | POLLERR | POLLHUP | POLLNVAL))
	{
	    fdw_hnd(cfd)(fdw_data(cfd), fdwatch_type_write);
	    changed = 1;
	}

	if (changed) sr--;
    }
}

#endif /* HAVE_POLL */
