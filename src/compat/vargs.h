/* ANSI and traditional C compatability macros
   Copyright 1991, 1992, 1993, 1994, 1995, 1996, 1998, 1999, 2000, 2001
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef	_VARGS_H
#define _VARGS_H	1

#ifdef HAVE_STDARG_H
# include <stdarg.h>
# define VA_START(VA_LIST, VAR)	va_start(VA_LIST, VAR)
#else	/* Not ANSI C.  */
# ifdef HAVE_VARAGRS_H
#  include <varargs.h>
#  define VA_START(va_list, var)	va_start(va_list)
# else
#  error "Your system neither offers stdarg.h nor varargs.h!"
# endif
#endif

#endif	/* vargs.h	*/
