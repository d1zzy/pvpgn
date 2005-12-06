/*
 * Copyright (C) 2005 Olaf Freyer (aaron@cs.tu-berlin.de)
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
#ifndef INCLUDED_SNPRINTF_PROTOS
#define INCLUDED_SNPRINTF_PROTOS

#include <stdio.h>
#include "compat/vargs.h"

#if !defined(HAVE_SNPRINTF) 
#ifdef HAVE__SNPRINTF
#define snprintf _snprintf
#else
#if defined(HAVE_VSNPRINTF) || defined(HAVE__VSNPRINTF)
extern int snprintf(char *str, size_t size, const char *format, ...);
#else
#error "Your system lacks ANY kind of snprintf support!"
#endif
#endif
#endif

#endif /* INCLUDED_SNPRINTF_PROTOS */
