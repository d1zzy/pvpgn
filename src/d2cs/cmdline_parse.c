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
#include "compat/memset.h"

#include "conf.h"
#include "version.h"
#include "cmdline_parse.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static t_conf_table param_conf_table[]={
	{ "-c",			offsetof(t_param,prefs_file),	conf_type_str,	0,	D2CS_DEFAULT_CONF_FILE  },
	{ "-l",			offsetof(t_param,logfile),	conf_type_str,  0,	NULL		},
	{ "-h",			offsetof(t_param,help),		conf_type_bool, 0,	NULL		},
	{ "--help",		offsetof(t_param,help),		conf_type_bool,	0,	NULL		},
	{ "-v",			offsetof(t_param,version),	conf_type_bool,	0,	NULL		},
	{ "--version",		offsetof(t_param,version),	conf_type_bool,	0,	NULL		},
	{ "-f",			offsetof(t_param,foreground),	conf_type_bool,	0,	NULL		},
	{ "--foreground",	offsetof(t_param,foreground),	conf_type_bool,	0,	NULL		},
	{ "-s",			offsetof(t_param,logstderr),	conf_type_bool,	0,	NULL		},
	{ "--stderr",		offsetof(t_param,logstderr),	conf_type_bool,	0,	NULL		},
	{ NULL,			0,				conf_type_none,	0,	NULL		}
};

static t_param cmdline_param;

static char help_message[]="\n"
"Usage: d2cs [<options>]\n"
"	-c <FILE>:		set configuration file to FILE\n"
"	-l <FILE>:		set log to FILE\n"
"	-h, --help:		show this help message and exit\n"
"	-v, --version:		show version information and exit\n"
"	-f, --foreground:	start in foreground mode (don`t daemonize)\n"
"	-s, --stderr:		log to stderr instead of logging to file\n"
"\n"
"Notes:\n"
"	1.You should always use absolute path here for all FILE names\n";

extern void cmdline_show_help(void)
{
	fputs(help_message,stderr);
	return;
}

extern void cmdline_show_version(void)
{
	fputs(D2CS_VERSION,stderr);
	fputs("\n\n",stderr);
	return;
}

extern int cmdline_parse(int argc, char ** argv)
{
	memset(&cmdline_param,0, sizeof(cmdline_param));
	if (conf_parse_param(argc, argv, param_conf_table, &cmdline_param, sizeof(cmdline_param))<0) {
		return -1;
	}
	return 0;
}

extern int cmdline_cleanup(void)
{
	return conf_cleanup(param_conf_table, &cmdline_param, sizeof(cmdline_param));
}

extern char const * cmdline_get_prefs_file(void)
{
	return cmdline_param.prefs_file;
}

extern unsigned int cmdline_get_help(void)
{
	return cmdline_param.help;
}

extern unsigned int cmdline_get_version(void)
{
	return cmdline_param.version;
}

extern unsigned int cmdline_get_foreground(void)
{
	return cmdline_param.foreground;
}

extern unsigned int cmdline_get_logstderr(void)
{
	return cmdline_param.logstderr;
}

extern char const * cmdline_get_logfile(void)
{
	return cmdline_param.logfile;
}

#ifdef USE_CHECK_ALLOC
extern char const * cmdline_get_memlog_file(void)
{
	return cmdline_param.memlog_file;
}
#endif
