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
#define CHECK_ALLOC_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef USE_CHECK_ALLOC
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "common/check_alloc.h"
#include "common/setup_after.h"


static FILE *    logfp;
static t_alist * head;


static int add_item(char const * func, void * ptr, unsigned int size, char const * file_name, int line_number);
static int del_item(char const * func, void * ptr, unsigned int size, char const * file_name, int line_number);
static unsigned int get_size(void const * ptr);


static int add_item(char const * func, void * ptr, unsigned int size, char const * file_name, int line_number)
{
    t_alist * new;
    t_alist * curr;
    t_alist * * change;
    
    change = &head;
    for (curr=head; curr; curr=curr->next)
    {
	if (curr->ptr==ptr)
	{
	    if (logfp)
		fprintf(logfp,"Missing free for %p = %s(%u) in %s:%d\n",curr->ptr,curr->func,curr->size,curr->file_name,curr->line_number);
	    curr->func = func;
	    curr->ptr = ptr;
	    curr->size = size;
	    curr->file_name = file_name;
	    curr->line_number = line_number;
	    return -1;
	}
	change = &curr->next;
    }
    
    if (!(new = malloc(sizeof(t_alist))))
    {
	if (logfp)
	    fprintf(logfp,"Internal allocation failure\n");
	return -1;
    }
    new->func = func;
    new->ptr = ptr;
    new->size = size;
    new->file_name = file_name;
    new->line_number = line_number;
    new->next = NULL;
    
    *change = new;
    
    return 0;
}


static int del_item(char const * func, void * ptr, unsigned int size, char const * file_name, int line_number)
{
    t_alist * curr;
    t_alist * * change;
    
    change = &head;
    for (curr=head; curr; curr=curr->next)
    {
	if (curr->ptr==ptr)
	    break;
	change = &curr->next;
    }
    
    if (!curr)
    {
	if (logfp)
	    fprintf(logfp,"Extra %s(%p) of object in %s:%d\n",func,ptr,file_name,line_number);
	return -1;
    }
    
    *change = curr->next;
    free(curr);
    
    return 0;
}


static unsigned int get_size(void const * ptr)
{
    t_alist * curr;
    
    for (curr=head; curr; curr=curr->next)
	if (curr->ptr==ptr)
	    return curr->size;
    
    return 0;
}


extern void check_set_file(FILE * fp)
{
    logfp = fp;
}


extern int check_report_usage(void)
{
    t_alist *     curr;
    unsigned long countmem;
    unsigned int  countobj;
    
    if (!logfp)
	return -1;
    
    countmem = 0;
    countobj = 0;
    for (curr=head; curr; curr=curr->next)
    {
	fprintf(logfp,"%s:%d:%p=%s(%u)\n",curr->file_name,curr->line_number,curr->ptr,curr->func,curr->size);
	countmem += curr->size;
        countobj++;
    }
    fprintf(logfp,"Total allocated memory is %lu bytes in %u objects.\n",countmem,countobj);
    
    return 0;
}


extern void check_cleanup(void)
{
    t_alist *     curr;
    t_alist *     next;
    unsigned long countmem;
    unsigned int  countobj;
    
    countmem = 0;
    countobj = 0;
    for (curr=head; curr; curr=next)
    {
	next = curr->next;
	if (logfp)
	    fprintf(logfp,"Missing free for %p = %s(%u) in %s:%d\n",curr->ptr,curr->func,curr->size,curr->file_name,curr->line_number);
	countmem += curr->size;
        countobj++;
	free(curr);
    }
    fprintf(logfp,"Total unfreed memory is %lu bytes in %u objects.\n",countmem,countobj);
}


extern void * check_malloc_real(unsigned int size, char const * file_name, int line_number)
{
    void * ptr;
    
    ptr = malloc(size);
    if (ptr)
	memset(ptr,'D',size); /* 0x44 */
    
    add_item("malloc",ptr,size,file_name,line_number);
    
    return ptr;
}


extern void * check_calloc_real(unsigned int nelem, unsigned int size, char const * file_name, int line_number)
{
    void * ptr;
    
    ptr = calloc(nelem,size);
    
    add_item("calloc",ptr,size*nelem,file_name,line_number);
    
    return ptr;
}


extern void * check_realloc_real(void * in_ptr, unsigned int size, char const * file_name, int line_number)
{
    unsigned int oldsize;
    void *       out_ptr;
    
    if (size)
    {
	out_ptr = malloc(size);
	if (out_ptr)
	    memset(out_ptr,'D',size); /* 0x44 */
	add_item("realloc",out_ptr,size,file_name,line_number);
    }
    else
	out_ptr = NULL;
    
    if (in_ptr)
    {
	oldsize = get_size(in_ptr);
	
	if (out_ptr)
	    memcpy(out_ptr,in_ptr,oldsize<size?oldsize:size);
	
	memset(in_ptr,'U',oldsize); /* 0x55 */
	free(in_ptr);
	
	del_item("realloc",in_ptr,oldsize,file_name,line_number);
    }
    
    return out_ptr;
}


extern void check_free_real(void * ptr, char const * file_name, int line_number)
{
    unsigned int oldsize;
    
    oldsize = get_size(ptr);
    memset(ptr,'U',oldsize); /* 0x55 */
    free(ptr);
    
    del_item("free",ptr,oldsize,file_name,line_number);
}


extern void check_cfree_real(void * ptr, char const * file_name, int line_number)
{
    unsigned int oldsize;
    
    oldsize = get_size(ptr);
    memset(ptr,'U',oldsize); /* 0x55 */
    cfree(ptr);
    
    del_item("cfree",ptr,oldsize,file_name,line_number);
}


extern void * check_strdup_real(char const * str, char const * file_name, int line_number)
{
    void * ptr;
    
    if (!str)
    {
	if (logfp)
	    fprintf(logfp,"Got NULL string to duplicate in %s:%d\n",file_name,line_number);
	return NULL;
    }
    
    ptr = strdup(str);
    
    add_item("strdup",ptr,strlen(str)+1,file_name,line_number);
    
    return ptr;
}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
