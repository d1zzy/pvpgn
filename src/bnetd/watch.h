/*
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_WATCH_TYPES
#define INCLUDED_WATCH_TYPES

#ifdef WATCH_INTERNAL_ACCESS

#ifdef JUST_NEED_TYPES
# include "account.h"
# include "connection.h"
#else
# define JUST_NEED_TYPES
# include "account.h"
# include "connection.h"
# undef JUST_NEED_TYPES
#endif
#include "common/tag.h"

#endif

typedef enum
{
    watch_event_login=1,
    watch_event_logout=2,
    watch_event_joingame=4,
    watch_event_leavegame=8
} t_watch_event;

#ifdef WATCH_INTERNAL_ACCESS
typedef struct
{
    t_connection * owner; /* who to notify */
    t_account *    who;   /* when this account */
    t_watch_event  what;  /* does one of these things */
    t_clienttag clienttag;
} t_watch_pair;
#endif

#endif

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_WATCH_PROTOS
#define INCLUDED_WATCH_PROTOS

#define JUST_NEED_TYPES
#include "account.h"
#include "connection.h"
#undef JUST_NEED_TYPES

extern int watchlist_create(void);
extern int watchlist_destroy(void);
extern int watchlist_add_events(t_connection * owner, t_account * who, t_clienttag clienttag, t_watch_event events);
extern int watchlist_del_events(t_connection * owner, t_account * who, t_clienttag clienttag, t_watch_event events);
extern int watchlist_del_all_events(t_connection * owner);
extern int watchlist_del_by_account(t_account * account);
extern int watchlist_notify_event(t_account * who, char const * gamename, t_clienttag clienttag, t_watch_event event);

#endif
#endif
