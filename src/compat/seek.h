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
#ifndef INCLUDED_SEEK_PROTOS
#define INCLUDED_SEEK_PROTOS

#ifndef SEEK_SET
# ifdef L_SET
#  define SEEK_SET L_SET
# else
#  define SEEK_SET 0
# endif
#endif

#ifndef SEEK_CUR
# ifdef L_INCR
#  define SEEK_CUR L_INCR
# else
#  define SEEK_CUR 1
# endif
#endif

#ifndef SEEK_END
# ifdef L_XTND
#  define SEEK_END L_XTND
# else
#  define SEEK_END 2
# endif
#endif

#endif
