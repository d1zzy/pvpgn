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
#ifndef INCLUDED_CMDLINE_PARSE_H
#define INCLUDED_CMDLINE_PARSE_H

typedef struct
{
	char const	* prefs_file;
	char const	* logfile;
	unsigned int	foreground;
	unsigned int	help;
	unsigned int	version;
	unsigned int	debugmode;
	unsigned int	run_as_service;
	char const *	make_service;
} t_param;

extern int cmdline_parse(int argc, char ** argv);
extern int cmdline_cleanup(void);
extern void cmdline_show_help(void);
extern void cmdline_show_version(void);
extern char const * cmdline_get_prefs_file(void);
extern char const * cmdline_get_logfile(void);
extern unsigned int cmdline_get_version(void);
extern unsigned int cmdline_get_help(void);
extern unsigned int cmdline_get_foreground(void);
extern unsigned int cmdline_get_debugmode(void);
#ifdef WIN32
extern unsigned int cmdline_get_run_as_service(void);
extern char const * cmdline_get_make_service(void);
#endif

#endif
