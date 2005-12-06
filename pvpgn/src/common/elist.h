/*
 * Copyright (C) 2004  Dizzy 
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
#ifndef INCLUDED_ELIST_TYPES
#define INCLUDED_ELIST_TYPES

typedef struct elist_struct {
    struct elist_struct *next, *prev;
} t_elist;

/* hlist are single linked elists */
typedef struct hlist_struct {
    struct hlist_struct *next;
} t_hlist;

#endif /* INCLUDED_ELIST_TYPES */

#ifndef INCLUDED_ELIST_PROTOS
#define INCLUDED_ELIST_PROTOS

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#endif

/* access to it's members */
#define elist_next(ptr) ((ptr)->next)
#define elist_prev(ptr) ((ptr)->prev)

#define __elist_init(elist,val) { (elist)->next = (elist)->prev = (val); }
#define elist_init(elist) __elist_init(elist,elist)
#define DECLARE_ELIST_INIT(var) \
    t_elist var = { &var, &var };

/* link an new node just after "where" */
static inline void elist_add(t_elist *where, t_elist *what)
{
    what->next = where->next;
    where->next->prev = what;
    what->prev = where;
    where->next = what;
}

/* link a new node just before "where" (usefull in creating queues) */
static inline void elist_add_tail(t_elist *where, t_elist *what)
{
    what->prev = where->prev;
    where->prev->next = what;
    what->next = where;
    where->prev = what;
}

/* unlink "what" from it's list */
static inline void elist_del(t_elist *what)
{
    what->next->prev = what->prev;
    what->prev->next = what->next;
}

/* finds out the container address by computing it from the list node 
 * address substracting the offset inside the container of the list node 
 * member */
#define elist_entry(ptr,type,member) ((type*)(((char*)ptr)-offsetof(type,member)))

/* DONT remove while traversing with this ! */
#define elist_for_each(pos,head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define elist_for_each_rev(pos,head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

/* safe for removals while traversing */
#define elist_for_each_safe(pos,head,save) \
    for (pos = (head)->next, save = pos->next; pos != (head); \
			pos = save, save = pos->next)

#define elist_for_each_safe_rev(pos,head,save) \
    for (pos = (head)->prev, save = pos->prev; pos != (head); \
			pos = save, save = pos->prev)

#define elist_empty(ptr) ((ptr)->next == (ptr))

#define hlist_next(ptr) elist_next(ptr)

#define __hlist_init(hlist,val) { (hlist)->next = (val); }
#define hlist_init(hlist) __hlist_init(hlist,hlist)
#define DECLARE_HLIST_INIT(var) \
    t_hlist var = { &var };

/* link an new node just after "where" */
static inline void hlist_add(t_hlist *where, t_hlist *what)
{
    what->next = where->next;
    where->next = what;
}

/* unlink "what" from it's list, prev->next = what */
static inline void hlist_del(t_hlist *what, t_hlist *prev)
{
    prev->next = what->next;
}

/* move "what" back in the list with one position, never NULL
 * prev: the previous element in the list, never NULL
 * prev2: the previous element of "prev", if NULL means "what" is first
 */
static inline void hlist_promote(t_hlist *what, t_hlist *prev, t_hlist *prev2)
{
    if (prev2) {
	prev->next = what->next;
	prev2->next = what;
	what->next = prev;
    }
}

/* finds out the container address by computing it from the list node 
 * address substracting the offset inside the container of the list node 
 * member */
#define hlist_entry(ptr,type,member) elist_entry(ptr,type,member)

/* DONT remove while traversing with this ! */
#define hlist_for_each(pos,head) elist_for_each(pos,head)

/* safe for removals while traversing */
#define hlist_for_each_safe(pos,head,save) elist_for_each_safe(pos,head,save)

#define hlist_empty(ptr) elist_empty(ptr)

#endif /* INCLUDED_ELIST_PROTOS */
