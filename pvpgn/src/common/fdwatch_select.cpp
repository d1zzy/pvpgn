/*
  * Abstraction API/layer for the various ways PvPGN can inspect sockets state
  * 2003 (C)
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

#define FDWATCH_BACKEND
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
#include "common/elist.h"
#include "common/setup_after.h"

#ifdef HAVE_SELECT

namespace pvpgn
{

static int sr;
static int smaxfd;
static t_psock_fd_set *rfds = NULL, *wfds = NULL, /* working sets (updated often) */
                      *trfds = NULL, *twfds = NULL; /* templates (updated rare) */

static int fdw_select_init(int nfds);
static int fdw_select_close(void);
static int fdw_select_add_fd(int idx, unsigned rw);
static int fdw_select_del_fd(int idx);
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
    if (nfds > FD_SETSIZE) return -1; /* this should not happen */

    rfds = (t_psock_fd_set*)xmalloc(sizeof(t_psock_fd_set));
    wfds = (t_psock_fd_set*)xmalloc(sizeof(t_psock_fd_set));
    trfds = (t_psock_fd_set*)xmalloc(sizeof(t_psock_fd_set));
    twfds = (t_psock_fd_set*)xmalloc(sizeof(t_psock_fd_set));

    PSOCK_FD_ZERO(trfds); PSOCK_FD_ZERO(twfds);
    smaxfd = sr = 0;

    eventlog(eventlog_level_info, __FUNCTION__, "fdwatch select() based layer initialized (max %d sockets)", nfds);
    return 0;
}

static int fdw_select_close(void)
{
    if (rfds) { xfree((void *)rfds); rfds = NULL; }
    if (wfds) { xfree((void *)wfds); wfds = NULL; }
    if (trfds) { xfree((void *)trfds); trfds = NULL; }
    if (twfds) { xfree((void *)twfds); twfds = NULL; }
    smaxfd = sr = 0;

    return 0;
}

static int fdw_select_add_fd(int idx, unsigned rw)
{
    int fd;

//    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d rw: %d", fd, rw);
    fd = fdw_fd(fdw_fds + idx);

    /* select() interface is limited by FD_SETSIZE max socket value */
    if (fd >= FD_SETSIZE) return -1;

    if (rw & fdwatch_type_read) PSOCK_FD_SET(fd, trfds);
    else PSOCK_FD_CLR(fd, trfds);
    if (rw & fdwatch_type_write) PSOCK_FD_SET(fd, twfds);
    else PSOCK_FD_CLR(fd, twfds);
    if (smaxfd < fd) smaxfd = fd;

    return 0;
}

static int fdw_select_del_fd(int idx)
{
    int fd;

    fd = fdw_fd(fdw_fds + idx);
//    eventlog(eventlog_level_trace, __FUNCTION__, "called fd: %d", fd);
    if (sr > 0)
	eventlog(eventlog_level_error, __FUNCTION__, "BUG: called while still handling sockets");
    PSOCK_FD_CLR(fd, trfds);
    PSOCK_FD_CLR(fd, twfds);

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

static int fdw_select_cb(t_fdwatch_fd *cfd, void *data)
{
//    eventlog(eventlog_level_trace, __FUNCTION__, "idx: %d fd: %d", idx, fdw_fd->fd);
    if (fdw_rw(cfd) & fdwatch_type_read && PSOCK_FD_ISSET(fdw_fd(cfd), rfds)
        && fdw_hnd(cfd)(fdw_data(cfd), fdwatch_type_read) == -2) return 0;
    if (fdw_rw(cfd) & fdwatch_type_write && PSOCK_FD_ISSET(fdw_fd(cfd), wfds))
        fdw_hnd(cfd)(fdw_data(cfd), fdwatch_type_write);

    return 0;
}

static void fdw_select_handle(void)
{
//    eventlog(eventlog_level_trace, __FUNCTION__, "called nofds: %d", fdw_nofds);
    fdwatch_traverse(fdw_select_cb,NULL);
    sr = 0;
}

}

#endif /* HAVE_SELECT */
