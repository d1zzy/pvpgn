/*
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_LIST_TYPES
#define INCLUDED_LIST_TYPES

typedef struct elem
#ifdef LIST_INTERNAL_ACCESS
{
    void *        data;
    struct elem * next;
}
#endif
t_elem;

typedef struct list
#ifdef LIST_INTERNAL_ACCESS
{
    unsigned int  len;
    t_elem *      head;
    t_elem *      tail;
}
#endif
t_list;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_LIST_PROTOS
#define INCLUDED_LIST_PROTOS

#ifdef USE_CHECK_ALLOC
extern t_list * list_create_real(char const * fn, unsigned int ln) ;
# define list_create() list_create_real(__FILE__"{list_create}",__LINE__)
#else
extern t_list * list_create(void) ;
#endif
extern int list_destroy(t_list * list);
extern int list_purge(t_list * list);
extern int list_check(t_list const * list);
extern unsigned int list_get_length(t_list const * list);
#ifdef USE_CHECK_ALLOC
extern int list_prepend_data_real(t_list * list, void * data, char const * fn, unsigned int ln);
# define list_prepend_data(L,D) list_prepend_data_real(L,D,__FILE__"{list_prepend_data}",__LINE__)
#else
extern int list_prepend_data(t_list * list, void * data);
#endif
#ifdef USE_CHECK_ALLOC
extern int list_append_data_real(t_list * list, void * data, char const * fn, unsigned int ln);
# define list_append_data(L,D) list_append_data_real(L,D,__FILE__"{list_append_data}",__LINE__)
#else
extern int list_append_data(t_list * list, void * data);
#endif
extern t_elem * list_get_elem_by_data(t_list const * list, void const * data);
extern t_elem const * list_get_elem_by_data_const(t_list const * list, void const * data);
extern int list_remove_data(t_list * list, void const * data); /* delete matching item */
extern int list_remove_elem(t_list * list, t_elem * elem);
extern void * list_get_data_by_pos(t_list const * list, unsigned int pos);
#ifdef LIST_DEBUG
extern t_elem * list_get_first_real(t_list const * list, char const * fn, unsigned int ln);
# define list_get_first(L) list_get_first_real(L,__FILE__,__LINE__)
#else
extern t_elem * list_get_first(t_list const * list);
#endif
#ifdef LIST_DEBUG
extern t_elem const * list_get_first_const_real(t_list const * list, char const * fn, unsigned int ln);
# define list_get_first_const(L) list_get_first_const_real(L,__FILE__,__LINE__)
#else
extern t_elem const * list_get_first_const(t_list const * list);
#endif

extern void * elem_get_data(t_elem const * elem);
extern int elem_set_data(t_elem * elem, void * data);
extern t_elem * elem_get_next(t_elem const * elem);
extern t_elem const * elem_get_next_const(t_elem const * elem);

#define LIST_TRAVERSE(list,curr) for (curr=(list)?list_get_first(list):(NULL); curr; curr=elem_get_next(curr))
#define LIST_TRAVERSE_CONST(list,curr) for (curr=(list)?list_get_first_const(list):(NULL); curr; curr=elem_get_next_const(curr))

#endif
#endif
