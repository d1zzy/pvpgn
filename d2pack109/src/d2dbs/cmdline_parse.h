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
#ifndef INCLUDED_CMDLINE_PARSE_H
#define INCLUDED_CMDLINE_PARSE_H

typedef struct
{
	char const	* prefs_file;
	char const	* logfile;
#ifdef USE_CHECK_ALLOC
	char const	* memlog_file;
#endif
	unsigned int	foreground;
	unsigned int	help;
	unsigned int	version;
	unsigned int	logstderr;
} t_param;

extern int d2dbs_cmdline_parse(int argc, char ** argv);
extern int d2dbs_cmdline_cleanup(void);
extern void d2dbs_cmdline_show_help(void);
extern void d2dbs_cmdline_show_version(void);
extern char const * d2dbs_cmdline_get_prefs_file(void);
extern char const * d2dbs_cmdline_get_logfile(void);
extern unsigned int d2dbs_cmdline_get_version(void);
extern unsigned int d2dbs_cmdline_get_help(void);
extern unsigned int d2dbs_cmdline_get_foreground(void);
extern unsigned int d2dbs_cmdline_get_logstderr(void);
#ifdef USE_CHECK_ALLOC
extern char const * cmdline_get_memlog_file(void);
#endif

#endif
