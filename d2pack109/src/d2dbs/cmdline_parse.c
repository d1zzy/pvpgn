/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
 * Copyright (C) 2001		sousou	(liupeng.cs@263.net)
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

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#include <stdio.h>
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

#include "version.h"
#include "cmdline_parse.h"
#include "d2cs/conf.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static t_conf_table param_conf_table[]={
	{ "-c",          offsetof(t_param,prefs_file),    conf_type_str, 0, D2DBS_DEFAULT_CONF_FILE},
	{ "-l",          offsetof(t_param,logfile),       conf_type_str, 0, NULL                   },
	{ "-h",          offsetof(t_param,help),          conf_type_bool,0, NULL                   },
	{ "--help",      offsetof(t_param,help),          conf_type_bool,0, NULL                   },
	{ "-v",          offsetof(t_param,version),       conf_type_bool,0, NULL                   },
	{ "--version",   offsetof(t_param,version),       conf_type_bool,0, NULL                   },
	{ "-f",          offsetof(t_param,foreground),    conf_type_bool,0, NULL                   },
	{ "--foreground",offsetof(t_param,foreground),    conf_type_bool,0, NULL                   },
	{ "-D",          offsetof(t_param,debugmode),     conf_type_bool,0, NULL                   },
	{ "--debug",     offsetof(t_param,debugmode),     conf_type_bool,0, NULL                   },
#ifdef WIN32
	{ "--service",	 offsetof(t_param,run_as_service),conf_type_bool,0, NULL                   },
	{ "-s",          offsetof(t_param,make_service),  conf_type_str, 0, NULL                   },
#endif
	{ NULL,          0,                               conf_type_none,0, NULL                   }
};

static t_param cmdline_param;

static char help_message[]="Usage: d2dbs [<options>]\n"
"	-m <FILE>:		set memory debug logging file to FILE\n"
"	-c <FILE>:		set configuration file to FILE\n"
"	-l <FILE>:		set log to FILE\n"
"	-h, --help:		show this help message and exit\n"
"	-v, --version:		show version information and exit\n"
"	-f, --foreground:	start in foreground mode (don`t daemonize)\n"
"	-D, --debug:		run in debug mode (run in foreground and log to stdout)\n"
#ifdef WIN32
"    Running as service functions:\n"
"	--service		run as service\n"
"	-s install		install service\n"
"	-s uninstall		uninstall service\n"
#endif
"\n"
"Notes:\n"
"	1.You should always use absolute path here for all FILE names\n\n";

extern void d2dbs_cmdline_show_help(void)
{
	fprintf(stderr,help_message);
	return;
}

extern void d2dbs_cmdline_show_version(void)
{
	fprintf(stderr,D2DBS_VERSION);
	fprintf(stderr,"\n\n");
	return;
}

extern int d2dbs_cmdline_parse(int argc, char ** argv)
{
	memset(&cmdline_param,0, sizeof(cmdline_param));
	if (conf_parse_param(argc, argv, param_conf_table, &cmdline_param, sizeof(cmdline_param))<0) {
		return -1;
	}
	return 0;
}

extern int d2dbs_cmdline_cleanup(void)
{
	return conf_cleanup(param_conf_table, &cmdline_param, sizeof(cmdline_param));
}

extern char const * d2dbs_cmdline_get_prefs_file(void)
{
	return cmdline_param.prefs_file;
}

extern unsigned int d2dbs_cmdline_get_help(void)
{
	return cmdline_param.help;
}

extern unsigned int d2dbs_cmdline_get_version(void)
{
	return cmdline_param.version;
}

extern unsigned int d2dbs_cmdline_get_foreground(void)
{
	return cmdline_param.foreground;
}

extern char const * d2dbs_cmdline_get_logfile(void)
{
	return cmdline_param.logfile;
}

extern unsigned int d2dbs_cmdline_get_debugmode(void)
{
	return cmdline_param.debugmode;
}

#ifdef WIN32
extern unsigned int d2dbs_cmdline_get_run_as_service(void)
{
	return cmdline_param.run_as_service;
}

extern char const * d2dbs_cmdline_get_make_service(void)
{
	return cmdline_param.make_service;
}

#endif
