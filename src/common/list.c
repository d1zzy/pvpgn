/*
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#define LIST_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "common/eventlog.h"
#include "common/list.h"
#include "common/setup_after.h"


static int nodata; /* if data points to this, then the element was actually deleted */


#ifdef USE_CHECK_ALLOC
extern t_list * list_create_real(char const * fn, unsigned int ln)
#else
extern t_list * list_create(void)
#endif
{
    t_list * new;
    
#ifdef USE_CHECK_ALLOC
    if (!(new = check_malloc_real(sizeof(t_list),fn,ln)))
#else
    if (!(new = malloc(sizeof(t_list))))
#endif
    {
	eventlog(eventlog_level_error,"list_create","could not allocate memory for new");
	return NULL;
    }
    
    new->head = NULL;
    new->tail = NULL;
    new->len = 0;
    
    return new;
}


extern int list_destroy(t_list * list)
{
    if (!list)
    {
        eventlog(eventlog_level_error,"list_destroy","got NULL list");
        return -1;
    }
    
    list_purge(list);
    if (list->head)
	eventlog(eventlog_level_error,"list_destroy","got non-empty list");
    
    free(list);
    
    return 0;
}


extern int list_purge(t_list * list)
{
    t_elem *   curr;
    t_elem *   head;
    t_elem *   tail;
    t_elem *   next;
    t_elem * * change;
    
    if (!list)
    {
        eventlog(eventlog_level_error,"list_purge","got NULL list");
        return -1;
    }
    
#ifdef LIST_DEBUG
    list_check(list);
#endif
    
    head = NULL;
    tail = NULL;
    change = NULL;
    for (curr=list->head; curr; curr=next)
    {
	next = curr->next;
	if (curr->data==&nodata)
	{
	    if (change)
		*change = next;
	    free(curr);
	}
	else
	{
	    tail = curr;
	    if (!head)
		head = curr;
	    change = &curr->next;
	}
    }
    
    list->head = head;
    list->tail = tail;
    
    return 0;
}


extern int list_check(t_list const * list)
{
    unsigned int   emptycnt;
    unsigned int   validcnt;
    t_elem const * tail;
    t_elem const * curr;
    int            ret=0;
    
    if (!list)
    {
        eventlog(eventlog_level_error,"list_check","got NULL list");
        return -1;
    }
    
    emptycnt=validcnt = 0;
    tail = NULL;
    for (curr=list->head; curr; curr=curr->next)
    {
	if (tail)
	{
	    if (curr==tail) /* tail is currently the previous node */
	    {
		eventlog(eventlog_level_error,"list_check","list is circular (curr==prev==%p)",curr);
		return -1;
	    }
	    if (curr->next==tail)
	    {
		eventlog(eventlog_level_error,"list_check","list is circular (curr->next==prev==%p)",curr);
		return -1;
	    }
	    if (curr==list->head)
	    {
		eventlog(eventlog_level_error,"list_check","list is circular (curr==list->head==%p)",curr);
		return -1;
	    }
	}
	if (curr->data==&nodata)
	    emptycnt++;
	else
	    validcnt++;
	tail = curr;
    }
    
    if (emptycnt>10 && emptycnt>validcnt+5) /* arbitrary heuristic to detect missing list_purge() calls */
	eventlog(eventlog_level_warn,"list_check","emptycnt=%u but validcnt=%u",emptycnt,validcnt);
    if (list->len!=validcnt)
    {
	eventlog(eventlog_level_error,"list_check","list->len=%u but validcnt=%u",list->len,validcnt);
	ret = -1;
    }
    if (list->head && !list->tail)
    {
	eventlog(eventlog_level_error,"list_check","list->head=%p but list->tail=%p (len=%u)",list->head,list->tail,list->len);
	ret = -1;
    }
    if (list->tail!=tail)
    {
	eventlog(eventlog_level_error,"list_check","list->tail=%p but tail=%p",list->tail,tail);
	ret = -1;
    }
    if (validcnt!=0 && !list->head)
    {
	eventlog(eventlog_level_error,"list_check","validcnt=%u but list->head=%p",validcnt,list->head);
	ret = -1;
    }
    
    return ret;
}


extern unsigned int list_get_length(t_list const * list)
{
    if (!list)
    {
	eventlog(eventlog_level_error,"list_get_length","got NULL list");
	return 0;
    }
    
    return list->len;
}


#ifdef USE_CHECK_ALLOC
extern int list_prepend_data_real(t_list * list, void * data, char const * fn, unsigned int ln)
#else
extern int list_prepend_data(t_list * list, void * data)
#endif
{
    t_elem * elem;
    
    if (!list)
    {
	eventlog(eventlog_level_error,"list_prepend_data","got NULL list");
	return -1;
    }
    
#ifdef USE_CHECK_ALLOC
    if (!(elem = check_malloc_real(sizeof(t_elem),fn,ln)))
#else
    if (!(elem = malloc(sizeof(t_elem))))
#endif
    {
	eventlog(eventlog_level_error,"list_prepend_data","could not allocate memory for elem");
	return -1;
    }
    elem->data = data;
    
    elem->next = list->head;
    list->head = elem;
    if (!list->tail)
	list->tail = elem;
    list->len++;
    
    return 0;
}


#ifdef USE_CHECK_ALLOC
extern int list_append_data_real(t_list * list, void * data, char const * fn, unsigned int ln)
#else
extern int list_append_data(t_list * list, void * data)
#endif
{
    t_elem * elem;
    
    if (!list)
    {
	eventlog(eventlog_level_error,"list_append_data","got NULL list");
	return -1;
    }
    
#ifdef USE_CHECK_ALLOC
    if (!(elem = check_malloc_real(sizeof(t_elem),fn,ln)))
#else
    if (!(elem = malloc(sizeof(t_elem))))
#endif
    {
	eventlog(eventlog_level_error,"list_append_data","could not allocate memory for elem");
	return -1;
    }
    elem->data = data;
    
    elem->next = NULL;
    if (!list->head)
	list->head = elem;
    if (list->tail)
	list->tail->next = elem;
    list->tail = elem;
    list->len++;
    
    return 0;
}


extern t_elem * list_get_elem_by_data(t_list const * list, void const * data)
{
    t_elem * curr;
    
    if (!list)
    {
	eventlog(eventlog_level_error,"list_get_elem_by_data","got NULL list");
	return NULL;
    }
    
    LIST_TRAVERSE(list,curr)
	if (curr->data==data)
	    return curr;
    
    return NULL;
}


extern t_elem const * list_get_elem_by_data_const(t_list const * list, void const * data)
{
    t_elem const * curr;
    
    if (!list)
    {
	eventlog(eventlog_level_error,"list_get_elem_by_data_const","got NULL list");
	return NULL;
    }
    
    LIST_TRAVERSE_CONST(list,curr)
	if (curr->data==data)
	    return curr;
    
    return NULL;
}


extern int list_remove_elem(t_list * list, t_elem * elem)
{
    if (!list)
    {
	eventlog(eventlog_level_error,"list_remove_elem","got NULL list");
	return -1;
    }
    if (!elem)
    {
	eventlog(eventlog_level_error,"list_remove_elem","got NULL elem");
	return -1;
    }
    if (elem->data==&nodata)
    {
	eventlog(eventlog_level_error,"list_remove_elem","got deleted elem");
	return -1;
    }
    
    elem->data = &nodata;
    list->len--;
    
    return 0;
}


extern int list_remove_data(t_list * list, void const * data)
{
    t_elem * elem;
    
    if (!list)
    {
	eventlog(eventlog_level_error,"list_remove_data","got NULL list");
	return -1;
    }
    
    if (!(elem = list_get_elem_by_data(list,data)))
	return -1;
    
    return list_remove_elem(list,elem);
}


extern int elem_set_data(t_elem * elem, void * data)
{
    if (!elem)
    {
	eventlog(eventlog_level_error,"elem_set_data","got NULL elem");
	return -1;
    }
    if (elem->data==&nodata)
    {
	eventlog(eventlog_level_error,"elem_set_data","got deleted elem");
	return -1;
    }
    if (data==&nodata)
    {
	eventlog(eventlog_level_error,"elem_set_data","got bad data");
	return -1;
    }
    
    elem->data = data;
    
    return 0;
}


extern void * elem_get_data(t_elem const * elem)
{
    if (!elem)
    {
	eventlog(eventlog_level_error,"elem_get_data","got NULL elem");
	return NULL;
    }
    if (elem->data==&nodata)
    {
	eventlog(eventlog_level_error,"elem_get_data","got deleted elem");
	return NULL;
    }
    
    return elem->data;
}


extern void * list_get_data_by_pos(t_list const * list, unsigned int pos)
{
    t_elem const * curr;
    unsigned int   len;
    
    len = 0;
    LIST_TRAVERSE_CONST(list,curr)
	if (len++==pos)
	    return curr->data;
    
    eventlog(eventlog_level_error,"list_get_data_by_pos","requested position %u but len=%u",pos,len);
    return NULL;
}


#ifdef LIST_DEBUG
extern t_elem * list_get_first_real(t_list const * list, char const * fn, unsigned int ln)
#else
extern t_elem * list_get_first(t_list const * list)
#endif
{
    t_elem * curr;
    
    if (!list)
    {
#ifdef LIST_DEBUG
	eventlog(eventlog_level_error,"list_get_first","got NULL list from %s:%u",fn,ln);
#else
	eventlog(eventlog_level_error,"list_get_first","got NULL list");
#endif
	return NULL;
    }
    
    for (curr=list->head; curr; curr=curr->next)
	if (curr->data!=&nodata)
	    return curr;
    
    return curr;
}


#ifdef LIST_DEBUG
extern t_elem const * list_get_first_const_real(t_list const * list, char const * fn, unsigned int ln)
#else
extern t_elem const * list_get_first_const(t_list const * list)
#endif
{
    t_elem const * curr;
    
    if (!list)
    {
#ifdef LIST_DEBUG
	eventlog(eventlog_level_error,"list_get_first_const","got NULL list from %s:%u",fn,ln);
#else
	eventlog(eventlog_level_error,"list_get_first_const","got NULL list");
#endif
	return NULL;
    }
    
    for (curr=list->head; curr; curr=curr->next)
	if (curr->data!=&nodata)
	    return curr;
    
    return curr;
}


extern t_elem * elem_get_next(t_elem const * elem)
{
    t_elem * curr;
    
    if (!elem)
    {
	eventlog(eventlog_level_error,"elem_get_next","got NULL elem");
	return NULL;
    }
    
    for (curr=elem->next; curr; curr=curr->next)
	if (curr->data!=&nodata)
	    return curr;
    
    return curr;
}


extern t_elem const * elem_get_next_const(t_elem const * elem)
{
    t_elem const * curr;
    
    if (!elem)
    {
	eventlog(eventlog_level_error,"elem_get_next_const","got NULL elem");
	return NULL;
    }
    
    for (curr=elem->next; curr; curr=curr->next)
	if (curr->data!=&nodata)
	    return curr;
    
    return curr;
}
