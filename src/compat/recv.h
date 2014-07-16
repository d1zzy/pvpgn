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
#ifndef INCLUDED_RECV_PROTOS
#define INCLUDED_RECV_PROTOS

#ifndef HAVE_RECV

/* some recvfrom()s don't handle NULL, but recv is called depreciated on BSD */
#ifdef HAVE_RECVFROM
# include <cstddef>
# define recv(s, b, l, f) recvfrom(s, b, l, f, NULL, NULL)
#else
# ifdef HAVE_WINSOCK2_H
#  include <winsock2.h>
#  define recv(s, b, l, f) recvfrom(s, b, l, f, NULL, NULL)
# else
#   error "This program requires recvfrom()"
# endif
#endif

#endif

#endif
