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
#ifndef INCLUDED_PREFS_H
#define INCLUDED_PREFS_H

typedef struct
{
	char const	* logfile;
	char const	* loglevels;
	char const	* servaddrs;
	char const	* gameservlist;
	char const	* bnetdaddr;
	char const	* charsavedir;
	char const	* charinfodir;
        char const      * bak_charsavedir;
        char const      * bak_charinfodir;
	char const	* ladderdir;
	char const	* newbiefile;
	char const	* motd;
	char const	* realmname;
	char const	* d2gs_password;
	char const	* transfile;
	char const      * account_allowed_symbols;
	unsigned int	ladder_refresh_interval;
	unsigned int	maxchar;
	unsigned int	listpurgeinterval;
	unsigned int	gqcheckinterval;
	unsigned int	s2s_retryinterval;
	unsigned int	s2s_timeout;
	unsigned int	s2s_idletime;
	unsigned int	sq_checkinterval;
	unsigned int	sq_timeout;
	unsigned int	maxgamelist;
	unsigned int	max_game_idletime;
	unsigned int	gamelist_showall;
	unsigned int	game_maxlifetime;
	unsigned int	allow_gamelimit;
	unsigned int	allow_newchar;
	unsigned int	idletime;
	unsigned int	shutdown_delay;
	unsigned int	shutdown_decr;
	unsigned int	d2gs_checksum;
	unsigned int	d2gs_version;
	unsigned int	check_multilogin;
	unsigned int	timeout_checkinterval;
	unsigned int	s2s_keepalive_interval;
	unsigned int	lod_realm;
	unsigned int	allow_convert;
	unsigned int	d2gs_restart_delay;
        char const      * charlist_sort;
        char const      * charlist_sort_order;
        unsigned int    max_connections;
} t_prefs;

extern int d2cs_prefs_load(char const * filename);
extern int prefs_reload(char const * filename);
extern int d2cs_prefs_unload(void);

extern char const * d2cs_prefs_get_transfile(void);
extern char const * d2cs_prefs_get_logfile(void);
extern char const * prefs_get_servaddrs(void);
extern char const * prefs_get_charsave_dir(void);
extern char const * prefs_get_charinfo_dir(void);
extern char const * prefs_get_bak_charsave_dir(void);
extern char const * prefs_get_bak_charinfo_dir(void);
extern char const * prefs_get_charsave_newbie(void);
extern char const * prefs_get_motd(void);
extern char const * prefs_get_realmname(void);
extern char const * prefs_get_d2gs_list(void);
extern unsigned int prefs_get_maxchar(void);
extern unsigned int prefs_get_list_purgeinterval(void);
extern unsigned int prefs_get_maxgamelist(void);
extern unsigned int prefs_allow_newchar(void);
extern unsigned int prefs_get_gamequeue_checkinterval(void);
extern unsigned int prefs_get_idletime(void);
extern unsigned int d2cs_prefs_get_shutdown_delay(void);
extern unsigned int d2cs_prefs_get_shutdown_decr(void);
extern char const * prefs_get_bnetdaddr(void);
extern unsigned int prefs_get_s2s_retryinterval(void);
extern unsigned int prefs_get_s2s_timeout(void);
extern unsigned int prefs_get_sq_timeout(void);
extern unsigned int prefs_get_sq_checkinterval(void);
extern unsigned int prefs_get_d2gs_checksum(void);
extern unsigned int prefs_get_d2gs_version(void);
extern unsigned int prefs_get_ladderlist_count(void);
extern unsigned int prefs_get_d2ladder_refresh_interval(void);
extern unsigned int prefs_get_game_maxlifetime(void);
extern char const * prefs_get_ladder_dir(void);
extern char const * d2cs_prefs_get_loglevels(void);
extern unsigned int prefs_allow_gamelist_showall(void);
extern unsigned int prefs_allow_gamelimit(void);
extern unsigned int prefs_check_multilogin(void);
extern char const * prefs_get_d2gs_password(void);
extern unsigned int prefs_get_s2s_idletime(void);
extern unsigned int prefs_get_s2s_keepalive_interval(void);
extern unsigned int prefs_get_timeout_checkinterval(void);
extern unsigned int prefs_get_max_game_idletime(void);
extern unsigned int prefs_get_lod_realm(void);
extern unsigned int prefs_get_allow_convert(void);
extern char const * prefs_get_d2cs_account_allowed_symbols(void);
extern unsigned int prefs_get_d2gs_restart_delay(void);
extern char const * prefs_get_charlist_sort(void);
extern char const * prefs_get_charlist_sort_order(void);
extern unsigned int prefs_get_max_connections(void);

#endif
