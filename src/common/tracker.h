/*
 * Copyright (C) 1999  Mark Baysinger (mbaysing@ucsd.edu)
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
#ifndef INCLUDED_TRACKER_TYPES
#define INCLUDED_TRACKER_TYPES

/*
 *
 * Here's how this works:
 *
 * Bnetd sends a UDP ClientPacket to each track server port on a regular basis.
 * Simple.
 *
 */

#ifdef TRACKER_INTERNAL_ACCESS
typedef struct
{
  unsigned short packet_version;      /* set to TRACK_VERSION, network byte order */
  unsigned short port;                /* port server is listening on, network byte order */
  unsigned long  flags;               /* see below, network byte order */
  char           software[32];        /* example: Bnetd, NUL terminated */
  char           version[16];         /* example: 0.4, NUL terminated */
  char           platform[32];        /* Windows, Linux, etc., NUL terminated */
  char           server_desc[64];     /* description, NUL terminated */
  char           server_location[64]; /* geographical location, NUL terminated */
  char           server_url[96];      /* web address: http://..., NUL terminated */
  char           contact_name[64];    /* name of operator, NUL terminated */
  char           contact_email[64];   /* e-mail address of operator, NUL terminated */
  unsigned long  users;               /* current number of users, network byte order */
  unsigned long  channels;            /* current number of channels, network byte order */
  unsigned long  games;               /* current number of games, network byte order */
  unsigned long  uptime;              /* daemon uptime in seconds, network byte order */
  unsigned long  total_games;         /* total number of games served */
  unsigned long  total_logins;        /* total number of client logins */

  /* new versions will add fields to end of packet */
} t_trackpacket;

/* packet_version */
#define TRACK_VERSION 2

/* flags */
#define TF_SHUTDOWN 0x1     /* send packet with this flag set when
                             * shutting down (Currently ignored) */
#define TF_PRIVATE  0x2     /* server is private and should not be
                             * listed (Currently ignored) */
#endif

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_TRACKER_PROTOS
#define INCLUDED_TRACKER_PROTOS

#define JUST_NEED_TYPES
#include "addr.h"
#undef JUST_NEED_TYPES

extern int tracker_set_servers(char const * servers);
extern int tracker_send_report(t_addrlist const * addrs);

#endif
#endif
