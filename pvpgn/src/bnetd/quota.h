/*
 * Copyright (C) 2000  Dizzy
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
#ifndef INCLUDED_QUOTA_TYPES
#define INCLUDED_QUOTA_TYPES

#ifdef JUST_NEED_TYPES
# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#   if HAVE_SYS_TIME_H
#    include <sys/time.h>
#   else
#    include <time.h>
#   endif
# endif
# include "common/list.h"
#else
# define JUST_NEED_TYPES
# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#   if HAVE_SYS_TIME_H
#    include <sys/time.h>
#   else
#    include <time.h>
#   endif
# endif
# include "common/list.h"
# undef JUST_NEED_TYPES
#endif

typedef struct
{
    time_t	 inf;
    unsigned int count;
} t_qline;

typedef struct
{
    unsigned int totcount;
    t_list *     list;
} t_quota;

#endif
