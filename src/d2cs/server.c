/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
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
#include "setup.h"

#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "compat/memset.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/psock.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include "compat/inet_ntoa.h"
#include "compat/psock.h"

#include "d2gs.h"
#include "net.h"
#include "s2s.h"
#include "gamequeue.h"
#include "game.h"
#include "connection.h"
#include "serverqueue.h"
#include "common/fdwatch.h"
#include "server.h"
#include "prefs.h"
#include "d2ladder.h"
#ifndef WIN32
#include "handle_signal.h"
#endif
#include "common/addr.h"
#include "common/list.h"
#include "common/hashtable.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static int server_purge_list(void);
static int server_listen(void);
static int server_accept(int sock);
static int server_handle_timed_event(void);
static int server_handle_socket(void);
static int server_loop(void);
static int server_cleanup(void);

t_addrlist		* server_listen_addrs;

static int server_listen(void)
{
	t_addr		* curr_laddr;
	t_addr_data	laddr_data;
	int		sock;

	if (!(server_listen_addrs=addrlist_create(prefs_get_servaddrs(),INADDR_ANY,D2CS_SERVER_PORT))) {
		eventlog(eventlog_level_error,__FUNCTION__,"error create listening address list");
		return -1;
	}
	BEGIN_LIST_TRAVERSE_DATA(server_listen_addrs,curr_laddr)
	{
		sock=net_listen(addr_get_ip(curr_laddr),addr_get_port(curr_laddr),PSOCK_SOCK_STREAM);
		if (sock<0) {
			eventlog(eventlog_level_error,__FUNCTION__,"error listen socket");
			return -1;
		} 
		eventlog(eventlog_level_info,__FUNCTION__,"listen on %s", addr_num_to_addr_str(addr_get_ip(curr_laddr),addr_get_port(curr_laddr)));
		if (psock_ctl(sock,PSOCK_NONBLOCK)<0) {
			eventlog(eventlog_level_error,__FUNCTION__,"error set listen socket in non-blocking mode");
		}
		laddr_data.i = sock;
		addr_set_data(curr_laddr,laddr_data);
		fdwatch_add_fd(sock, fdwatch_type_read, d2cs_server_handle_accept, curr_laddr);
	}
	END_LIST_TRAVERSE_DATA()
	return 0;
}

static int server_accept(int sock)
{
	int			csock;
	struct sockaddr_in	caddr, raddr;
	psock_t_socklen		caddr_len, raddr_len;
	int			val;
	unsigned int		ip;
	unsigned short		port;
	t_connection	       *cc;

	caddr_len=sizeof(caddr);
	memset(&caddr,0,sizeof(caddr));
	csock=psock_accept(sock,(struct sockaddr *)&caddr,&caddr_len);
	if (csock<0) {
		eventlog(eventlog_level_error,__FUNCTION__,"error accept new connection");
		return -1;
	} else eventlog(eventlog_level_info,__FUNCTION__,"accept connection from %s",inet_ntoa(caddr.sin_addr));

	val=1;
	if (psock_setsockopt(csock, PSOCK_SOL_SOCKET, PSOCK_SO_KEEPALIVE, &val,sizeof(val))<0) {
		eventlog(eventlog_level_warn,__FUNCTION__,"error set sock option keep alive");
	}
	if (psock_ctl(csock, PSOCK_NONBLOCK)<0) {
		eventlog(eventlog_level_error,__FUNCTION__,"error set socket to non-blocking mode");
		psock_close(csock);
		return -1;
	}

	raddr_len=sizeof(raddr);
	memset(&raddr,0,sizeof(raddr));
	ip=port=0;
	if (psock_getsockname(csock,(struct sockaddr *)&raddr,&raddr_len)<0) {
		eventlog(eventlog_level_warn,__FUNCTION__,"unable to get local socket info");
	} else {
		if (raddr.sin_family!=PSOCK_AF_INET) {
			eventlog(eventlog_level_warn,__FUNCTION__,"got bad socket family %d",raddr.sin_family);
		} else {
			ip=ntohl(raddr.sin_addr.s_addr);
			port=ntohs(raddr.sin_port);
		}
	}
	if (!(cc = d2cs_conn_create(csock,ip,port,ntohl(caddr.sin_addr.s_addr),ntohs(caddr.sin_port)))) {
		eventlog(eventlog_level_error,__FUNCTION__,"error create new connection");
		psock_close(csock);
		return -1;
	}
	fdwatch_add_fd(csock, fdwatch_type_read, d2cs_server_handle_tcp, cc);
	return 0;
}

static int server_purge_list(void)
{
	hashtable_purge(d2cs_connlist());
	list_purge(d2cs_gamelist());
	list_purge(d2gslist());
	list_purge(sqlist());
	list_purge(gqlist());
	return 0;
}

static int server_handle_timed_event(void)
{
	static  time_t	prev_list_purgetime=0;
	static  time_t	prev_gamequeue_checktime=0;
	static	time_t	prev_s2s_checktime=0;
	static	time_t	prev_sq_checktime=0;
	static  time_t	prev_d2ladder_refresh_time=0;
	static  time_t	prev_s2s_keepalive_time=0;
	static  time_t	prev_timeout_checktime;
	time_t		now;

	now=time(NULL);
	if (now-prev_list_purgetime>(signed)prefs_get_list_purgeinterval()) {
		server_purge_list();
		d2cs_gamelist_check_voidgame();
		prev_list_purgetime=now;
	}
	if (now-prev_gamequeue_checktime>(signed)prefs_get_gamequeue_checkinterval()) {
		gqlist_update_all_clients();
		prev_gamequeue_checktime=now;
	}
	if (now-prev_s2s_checktime>(signed)prefs_get_s2s_retryinterval()) {
		s2s_check();
		prev_s2s_checktime=now;
	}
	if (now-prev_sq_checktime>(signed)prefs_get_sq_checkinterval()) {
		sqlist_check_timeout();
		prev_sq_checktime=now;
	}
	if (now-prev_d2ladder_refresh_time>(signed)prefs_get_d2ladder_refresh_interval()) {
		d2ladder_refresh();
		prev_d2ladder_refresh_time=now;
	}
	if (now-prev_s2s_keepalive_time>(signed)prefs_get_s2s_keepalive_interval()) {
		d2gs_keepalive();
		prev_s2s_keepalive_time=now;
	}
	if (now-prev_timeout_checktime>(signed)prefs_get_timeout_checkinterval()) {
		connlist_check_timeout();
		prev_timeout_checktime=now;
	}
	return 0;
}

extern int d2cs_server_handle_accept(void *data, t_fdwatch_type rw)
{
    int sock;

    sock = addr_get_data((t_addr *)data).i;
    server_accept(sock);
    return 0;
}

extern int d2cs_server_handle_tcp(void *data, t_fdwatch_type rw)
{
    t_connection *c = (t_connection *)data;

    if (rw & fdwatch_type_read) conn_add_socket_flag(c,SOCKET_FLAG_READ);
    if (rw & fdwatch_type_write) conn_add_socket_flag(c,SOCKET_FLAG_WRITE);
    if (conn_handle_socket(c)<0)
        d2cs_conn_set_state(c, conn_state_destroy);

    return 0;
}

static int server_handle_socket(void)
{
	switch (fdwatch(BNETD_POLL_INTERVAL)) {
		case -1:
			if (
#ifdef PSOCK_EINTR
			    psock_errno()!=PSOCK_EINTR &&
#endif
			    1) {
				eventlog(eventlog_level_error,__FUNCTION__,"select failed (select: %s)",strerror(psock_errno()));
				return -1;
			}
			/* fall through */
		case 0:
			return 0;
		default:
			break;
	}

	fdwatch_handle();
	return 0;
}

static int server_loop(void)
{
	unsigned int count;

	count=0;
	while (1) {
#ifndef WIN32
		if (handle_signal()<0) break;
#endif
		if (++count>=(1000/BNETD_POLL_INTERVAL)) {
			server_handle_timed_event();
			count=0;
		}
		server_handle_socket();
		d2cs_connlist_reap();
	}
	return 0;
}

static int server_cleanup(void)
{
	t_addr		* curr_laddr;
	int		sock;

	BEGIN_LIST_TRAVERSE_DATA(server_listen_addrs,curr_laddr)
	{
		sock=addr_get_data(curr_laddr).i;
		psock_close(sock);
	}
	END_LIST_TRAVERSE_DATA()
	addrlist_destroy(server_listen_addrs);
	return 0;
}

extern int d2cs_server_process(void)
{
#ifndef WIN32
	handle_signal_init();
#endif
	if (psock_init()<0) {
		eventlog(eventlog_level_error,__FUNCTION__,"failed to init network");
		return -1;
	}
	eventlog(eventlog_level_info,__FUNCTION__,"network initialized");
	if (s2s_init()<0) {
		eventlog(eventlog_level_error,__FUNCTION__,"failed to init s2s connection");
		return -1;
	}
	if (server_listen()<0) {
		eventlog(eventlog_level_error,__FUNCTION__,"failed to setup listen socket");
		return -1;
	}
	eventlog(eventlog_level_info,__FUNCTION__,"entering server loop");
	server_loop();
	eventlog(eventlog_level_info,__FUNCTION__,"exit from server loop,cleanup");
	server_cleanup();
	return 0;
}
