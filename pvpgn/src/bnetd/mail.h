/*
 * Copyright (C) 2001            Dizzy
 * Copyright (C) 2004            Donny Redmond (dredmond@linuxmail.org)
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


/*****/
#ifndef INCLUDED_MAIL_TYPES
#define INCLUDED_MAIL_TYPES


#define MAX_FUNC_LEN 10
#define MAX_MAIL_QUOTA 10
#define MAIL_FUNC_SEND 1
#define MAIL_FUNC_READ 2
#define MAIL_FUNC_DELETE 3
#define MAIL_FUNC_HELP 4
#define MAIL_FUNC_UNKNOWN 5


#ifdef MAIL_INTERNAL_ACCESS

#include <ctime>

#ifdef JUST_NEED_TYPES
# include "compat/pdir.h"
#else
# include "compat/pdir.h"
# undef JUST_NEED_TYPES
#endif

namespace pvpgn
{

namespace bnetd
{

typedef enum {
    mbox_mode_read = 0x01,
    mbox_mode_write = 0x02
} t_mbox_mode;

typedef struct mailbox_struct {
   t_pdir *     maildir;
   unsigned int uid;
   char *       path;
} t_mailbox;

typedef struct mail_struct {
   char * sender;
   char * message;
   std::time_t timestamp;
} t_mail;

typedef struct maillist_struct {
   int    idx;
   char * sender;
   std::time_t timestamp;
   struct maillist_struct * next;
} t_maillist;

}

}

#endif

#endif

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_MAIL_PROTOS
#define INCLUDED_MAIL_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#undef JUST_NEED_TYPES

namespace pvpgn
{

namespace bnetd
{

extern int handle_mail_command(t_connection *, char const *);
extern char const * check_mail(t_connection const * c);

}

}

#endif
#endif
