/*
  * Abstraction API/layer for the various ways PvPGN can inspect sockets state
  * 2003 (C) dizzy@roedu.net
  *
  * Code is based on the ideas found in thttpd project.
  *
  * select() based backend
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
/* According to earlier standards */
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
/* According to POSIX 1003.1-2001 */
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#include "compat/psock.h"
#include "fdwatch.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

#ifdef HAVE_SELECT

static int sr;
static int smaxfd;
static t_psock_fd_set *rfds = NULL, *wfds = NULL, /* working sets (updated often) */
                      *trfds = NULL, *twfds = NULL; /* templates (updated rare) */
static int nofds; /* no of sockets watched */
static int *fds;  /* array of sockets watched */
static int *fdw_ridx; /* reverse index from fd to its position in fds array */

static int fdw_select_init(int nfds);
static int fdw_select_close(void);
static int fdw_select_add_fd(int fd, t_fdwatch_type rw);
static int fdw_select_del_fd(int fd);
static int fdw_select_watch(long timeout_msecs);
static void fdw_select_handle(void);

t_fdw_backend fdw_select = {
    fdw_select_init,
    fdw_select_close,
    fdw_select_add_fd,
    fdw_select_del_fd,
    fdw_select_watch,
    fdw_select_handle
};

static int fdw_select_init(int nfds)
{
    int i;

    if (nfds > FD_SETSIZE) return -1; /* this should not happen */

    rfds = xmalloc(sizeof(t_psock_fd_set));
    wfds = xmalloc(sizeof(t_psock_fd_set));
    trfds = xmalloc(sizeof(t_psock_fd_set));
    twfds = xmalloc(sizeof(t_psock_fd_set));
    fds = xmalloc(sizeof(int) * nfds);
    fdw_ridx = xmalloc(sizeof(int) * nfds);

    PSOCK_FD_ZERO(trfds); PSOCK_FD_ZERO(twfds);
    smaxfd = nofds = sr = 0;
    for(i = 0; i < nfds; i++) fdw_ridx[i] = -1;

    eventlog(eventlog_level_info, __FUNCTION__, "fdwatch select() based layer initialized (max %d sockets)", nfds);
    return 0;
}

static int fdw_select_close(void)
{
    if (rfds) { xfree((void *)rfds); rfds = NULL; }
    if (wfds) { xfree((void *)wfds); wfds = NULL; }
    if (trfds) { xfree((void *)trfds); trfds = NULL; }
    if (twfds) { xfree((void *)twfds); twfds = NULL; }
    if (fds) { xfree((void *)fds); fds = NULL; }
    if (fdw_ridx) { xfree((void *)fdw_ridx); fdw_ridx = NULL; }
    smaxfd = nofds = sr = 0;

    return 0;
}

static int fdw_select_add_fd(int fd, t_fdwatch_type rw)
{
//    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d rw: %d", fd, rw);
    if (rw & fdwatch_type_read) PSOCK_FD_SET(fd, trfds);
    else PSOCK_FD_CLR(fd, trfds);
    if (rw & fdwatch_type_write) PSOCK_FD_SET(fd, twfds);
    else PSOCK_FD_CLR(fd, twfds);
    if (smaxfd < fd) smaxfd = fd;

    if (fdw_ridx[fd] < 0) { /* new fd for the watch list */
//	eventlog(eventlog_level_trace, __FUNCTION__, "new fd: %d pos: %d", fd, nofds);
	fdw_ridx[fd] = nofds;
	fds[nofds++] = fd;
    }

    return 0;
}

static int fdw_select_del_fd(int fd)
{
//    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d", fd);
    if (sr > 0) 
	eventlog(eventlog_level_error, __FUNCTION__, "BUG: called while still handling sockets");
    PSOCK_FD_CLR(fd, trfds);
    PSOCK_FD_CLR(fd, twfds);
    if (fdw_ridx[fd] >= 0) {
	nofds--;
	if (fdw_ridx[fd] < nofds) {
//	    eventlog(eventlog_level_trace, __FUNCTION__, "not last moving %d from end to %d", fds[nofds], fdw_ridx[fd]);
	    fdw_ridx[fds[nofds]] = fdw_ridx[fd];
	    fds[fdw_ridx[fd]] = fds[nofds];
	}
	fdw_ridx[fd] = -1;
    }

    return 0;
}

static int fdw_select_watch(long timeout_msec)
{
    static struct timeval tv;

    tv.tv_sec  = timeout_msec / 1000;
    tv.tv_usec = timeout_msec % 1000;

    /* set the working sets based on the templates */
    memcpy(rfds, trfds, sizeof(t_psock_fd_set));
    memcpy(wfds, twfds, sizeof(t_psock_fd_set));

    return (sr = psock_select(smaxfd + 1, rfds, wfds, NULL, &tv));
}

static void fdw_select_handle(void)
{
    register unsigned i;
    register int fd;

//    eventlog(eventlog_level_trace, __FUNCTION__, "called nofds: %d", nofds);
    for(i = 0; i < nofds; i++) {
	fd = fds[i];
//	eventlog(eventlog_level_trace, __FUNCTION__, "i: %d fd: %d", i, fd);
	if (fdw_rw[fd] & fdwatch_type_read && PSOCK_FD_ISSET(fd, rfds)
	    && fdw_hnd[fd](fdw_data[fd], fdwatch_type_read) == -2) continue;
	if (fdw_rw[fd] & fdwatch_type_write && PSOCK_FD_ISSET(fd, wfds))
	    fdw_hnd[fd](fdw_data[fd], fdwatch_type_write);
    }
    sr = 0;
}

#endif /* HAVE_SELECT */
