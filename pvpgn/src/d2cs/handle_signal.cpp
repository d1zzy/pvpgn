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

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include <errno.h>
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
#include "compat/strdup.h"
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
#ifdef DO_POSIXSIG
# include <signal.h>
# include "compat/signal.h"
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

#include "d2gs.h"
#include "game.h"
#include "prefs.h"
#include "d2ladder.h"
#include "cmdline.h"
#include "handle_signal.h"
#include "common/trans.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

static void on_signal(int s);

static volatile struct
{
	unsigned char	do_quit;
	unsigned char	cancel_quit;
	unsigned char	reload_config;
	unsigned char	reload_ladder;
	unsigned char	restart_d2gs;
	unsigned int	exit_time;
} signal_data ={ 0, 0, 0, 0, 0, 0 };

extern int handle_signal(void)
{
	time_t		now;
    char const * levels;
    char *       temp;
    char const * tok;


	if (signal_data.cancel_quit) {
		signal_data.cancel_quit=0;
		if (!signal_data.exit_time) {
			eventlog(eventlog_level_info,__FUNCTION__,"there is no previous shutdown to be canceled");
		} else {
			signal_data.exit_time=0;
			eventlog(eventlog_level_info,__FUNCTION__,"shutdown was canceled due to signal");
		}
	}
	if (signal_data.do_quit) {
		signal_data.do_quit=0;
		now=time(NULL);
		if (!signal_data.exit_time) {
			signal_data.exit_time=now+d2cs_prefs_get_shutdown_delay();
		} else {
			signal_data.exit_time-=d2cs_prefs_get_shutdown_decr();
		}
		eventlog(eventlog_level_info,__FUNCTION__,"the server is going to shutdown in %lu minutes",(signal_data.exit_time-now)/60);
	}
	if (signal_data.exit_time) {
		now=time(NULL);
		if (now >= (signed)signal_data.exit_time) {
			signal_data.exit_time=0;
			eventlog(eventlog_level_info,__FUNCTION__,"shutdown server due to signal");
			return -1;
		}
	}
	if (signal_data.reload_config) {
		signal_data.reload_config=0;
		eventlog(eventlog_level_info,__FUNCTION__,"reloading configuartion file due to signal");
		if (prefs_reload(cmdline_get_preffile())<0) {
			eventlog(eventlog_level_error,__FUNCTION__,"error reload configuration file,exitting");
			return -1;
		}
		if (d2gslist_reload(prefs_get_d2gs_list())<0) {
			eventlog(eventlog_level_error,__FUNCTION__,"error reloading game server list,exitting");
			return -1;
		}
		if (trans_reload(d2cs_prefs_get_transfile(),TRANS_D2CS)<0) {
	    		eventlog(eventlog_level_error,__FUNCTION__,"could not reload trans list");
		}

        eventlog_clear_level();
        if ((levels = d2cs_prefs_get_loglevels()))
        {
            temp = xstrdup(levels);
            tok = strtok(temp,","); /* strtok modifies the string it is passed */

            while (tok)
            {
              if (eventlog_add_level(tok)<0)
              eventlog(eventlog_level_error,__FUNCTION__,"could not add log level \"%s\"",tok);
              tok = strtok(NULL,",");
            }

            xfree(temp);
        }
#ifdef DO_DAEMONIZE
		if (!cmdline_get_foreground())
#endif		
			eventlog_open(d2cs_prefs_get_logfile());
	}
	if (signal_data.reload_ladder) {
		signal_data.reload_ladder=0;
		eventlog(eventlog_level_info,__FUNCTION__,"reloading ladder data due to signal");
		d2ladder_refresh();
	}
	
	if (signal_data.restart_d2gs) {
		signal_data.restart_d2gs=0;
		eventlog(eventlog_level_info,__FUNCTION__,"restarting all game servers due to signal");
		d2gs_restart_all_gs();
	}
	
	return 0;
}
#ifdef WIN32
extern void signal_quit_wrapper(void)
{
  signal_data.do_quit=1;
}

extern void signal_reload_config_wrapper(void)
{
    signal_data.reload_config = 1;
}

extern void signal_load_ladder_wrapper(void)
{
    signal_data.reload_ladder = 1;
}

extern void signal_exit_wrapper(void)
{
    signal_data.exit_time = 1;
}

extern void signal_restart_d2gs_wrapper(void)
{
    signal_data.restart_d2gs = 1;
}
#else
extern int handle_signal_init(void)
{
	signal(SIGINT,on_signal);
	signal(SIGTERM,on_signal);
	signal(SIGABRT,on_signal);
	signal(SIGHUP,on_signal);
	signal(SIGUSR1,on_signal);
	signal(SIGUSR2,on_signal);
	signal(SIGPIPE,on_signal);
	return 0;
}

static void on_signal(int s)
{
	switch (s) {
		case SIGINT:
			eventlog(eventlog_level_debug,__FUNCTION__,"sigint received");
			signal_data.do_quit=1;
			break;
		case SIGTERM:
			eventlog(eventlog_level_debug,__FUNCTION__,"sigint received");
			signal_data.do_quit=1;
			break;
		case SIGABRT:
			eventlog(eventlog_level_debug,__FUNCTION__,"sigabrt received");
			signal_data.cancel_quit=1;
			break;
		case SIGHUP:
			eventlog(eventlog_level_debug,__FUNCTION__,"sighup received");
			signal_data.reload_config=1;
			break;
		case SIGUSR1:
			eventlog(eventlog_level_debug,__FUNCTION__,"sigusr1 received");
			signal_data.reload_ladder=1;
			break;
		case SIGUSR2:
			eventlog(eventlog_level_debug,__FUNCTION__,"sigusr2 received");
			signal_data.restart_d2gs=1;
			break;
		case SIGPIPE:
			eventlog(eventlog_level_debug,__FUNCTION__,"sigpipe received");
			break;
	}
	signal(s,on_signal);
}
#endif

