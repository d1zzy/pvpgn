/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_EXITSTATUS_PROTOS
#define INCLUDED_EXITSTATUS_PROTOS

#ifdef EXIT_FAILURE
# if !EXIT_FAILURE
#  define STATUS_FAILURE 1
# else
#  define STATUS_FAILURE EXIT_FAILURE
# endif
#else
# define STATUS_FAILURE 1
#endif

#ifdef EXIT_SUCCESS
# define STATUS_SUCCESS EXIT_SUCCESS
#else
# define STATUS_SUCCESS 0
#endif

#endif
