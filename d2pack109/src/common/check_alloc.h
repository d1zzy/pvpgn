/*
 * Copyright (C) 2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifdef USE_CHECK_ALLOC
#ifndef INCLUDED_CHECK_ALLOC_TYPES
#define INCLUDED_CHECK_ALLOC_TYPES

#ifdef CHECK_ALLOC_INTERNAL_ACCESS
typedef struct alist
{
    char const *   func;
    void *         ptr;
    unsigned int   size;
    char const *   file_name;
    int            line_number;
    struct alist * next;
} t_alist;
#endif

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_CHECK_ALLOC_PROTOS
#define INCLUDED_CHECK_ALLOC_PROTOS

#define JUST_NEED_TYPES
#include <stdio.h>
#undef JUST_NEED_TYPES

extern void check_set_file(FILE * fp);
extern int check_report_usage(void);
extern void check_cleanup(void);

#ifndef CHECK_ALLOC_INTERNAL_ACCESS
#undef malloc
#define malloc(size)       check_malloc_real((size),__FILE__,__LINE__)
#undef realloc
#define realloc(ptr,size)  check_realloc_real((ptr),(size),__FILE__,__LINE__)
#undef free
#define free(ptr)          check_free_real((ptr),__FILE__,__LINE__)
#undef cfree
#define cfree(ptr)         check_cfree_real((ptr),__FILE__,__LINE__)
#undef strdup
#define strdup(str)        check_strdup_real((str),__FILE__,__LINE__)
#undef calloc
#define calloc(nelem,size) check_calloc_real((nelem),(size),__FILE__,__LINE__)
#endif

extern void * check_malloc_real(unsigned int size, char const * file_name, int line_number) ;
extern void * check_calloc_real(unsigned int nelem, unsigned int size, char const * file_name, int line_number) ;
extern void * check_realloc_real(void * ptr, unsigned int size, char const * file_name, int line_number);
extern void check_free_real(void * ptr, char const * file_name, int line_number);
extern void check_cfree_real(void * ptr, char const * file_name, int line_number);
extern void * check_strdup_real(char const * s1, char const * file_name, int line_number) ;

#endif
#endif
#endif
