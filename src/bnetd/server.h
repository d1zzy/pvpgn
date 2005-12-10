/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_SERVER_TYPES
#define INCLUDED_SERVER_TYPES

namespace pvpgn
{

namespace bnetd
{

#ifdef SERVER_INTERNAL_ACCESS

typedef enum
{
    laddr_type_bnet,	/* classic battle.net service (usually on port 6112) */
    laddr_type_w3route, /* warcraft 3 playgame routing (def. port 6200) */
    laddr_type_irc,   	/* Internet Relay Chat service (port is varying; mostly on port 6667 or 7000) */
    laddr_type_wol,	    /* Westwood Online (IRC) Chat Services (port is 4005) */
    laddr_type_telnet 	/* telnet service (usually on port 23) */
} t_laddr_type;


/* listen address structure */
typedef struct
{
    int          ssocket; /* TCP listen socket */
    int          usocket; /* UDP socket */
    t_laddr_type type;
} t_laddr_info;

#endif

#ifdef HAVE_TIME_H
# include <time.h>
#endif
extern time_t now;

}

}

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_SERVER_PROTOS
#define INCLUDED_SERVER_PROTOS

namespace pvpgn
{

namespace bnetd
{

extern unsigned int server_get_uptime(void);
extern unsigned int server_get_starttime(void);
extern void server_quit_delay(int delay);
extern void server_set_hostname(void);
extern char const * server_get_hostname(void);
extern void server_clear_hostname(void);
extern int server_process(void);

extern void server_quit_wraper(void);
extern void server_restart_wraper(void);
extern void server_save_wraper(void);

}

}

#endif
#endif
