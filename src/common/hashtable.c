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
#define HASHTABLE_INTERNAL_ACCESS
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
#include "common/hashtable.h"
#include "common/setup_after.h"


static int nodata; /* if data points to this, then the entry was actually deleted */
 

static t_entry * hashtable_entry_export(t_internentry * entry, t_hashtable const * hashtable, unsigned int row);


static t_entry * hashtable_entry_export(t_internentry * entry, t_hashtable const * hashtable, unsigned int row)
{
    t_entry * temp;
    
    if (!entry)
    {
	eventlog(eventlog_level_error,"hashtable_entry_export","got NULL entry");
	return NULL;
    }
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_entry_export","got NULL hashtable");
	return NULL;
    }
    if (row>=hashtable->num_rows)
    {
	eventlog(eventlog_level_error,"hashtable_entry_export","got bad row %u (max %u)",row,hashtable->num_rows-1);
	return NULL;
    }
    
    if (!(temp = malloc(sizeof(t_entry))))
    {
	eventlog(eventlog_level_error,"hashtable_entry_export","could not allocate memory for temp");
	return NULL;
    }
    
    temp->row = row;
    temp->real = entry;
    temp->hashtable = hashtable;
    
    return temp;
}


extern t_hashtable * hashtable_create(unsigned int num_rows)
{
    t_hashtable * new;
    unsigned int  i;
    
    if (num_rows<1)
    {
	eventlog(eventlog_level_error,"hashtable_create","num_rows must be at least 1");
	return NULL;
    }
    
    if (!(new = malloc(sizeof(t_hashtable))))
    {
	eventlog(eventlog_level_error,"hashtable_create","could not allocate memory for new");
	return NULL;
    }
    
    if (!(new->rows = malloc(sizeof(t_internentry *)*num_rows)))
    {
	eventlog(eventlog_level_error,"hashtable_create","could not allocate memory for new->rows");
	free(new);
	return NULL;
    }
    
    new->num_rows = num_rows;
    new->len = 0;
    for (i=0; i<num_rows; i++)
	new->rows[i] = NULL;
    
    return new;
}


extern int hashtable_destroy(t_hashtable * hashtable)
{
    unsigned int i;
    
    if (!hashtable)
    {
        eventlog(eventlog_level_error,"hashtable_destroy","got NULL hashtable");
        return -1;
    }
    
    hashtable_purge(hashtable);
    for (i=0; i<hashtable->num_rows; i++)
	if (hashtable->rows[i])
	    eventlog(eventlog_level_error,"hashtable_destroy","got non-empty hashtable");
    
    free(hashtable->rows);
    free(hashtable);
    
    return 0;
}


extern int hashtable_purge(t_hashtable * hashtable)
{
    unsigned int      row;
    t_internentry *   curr;
    t_internentry *   head;
    t_internentry *   next;
    t_internentry * * change;
    
    if (!hashtable)
    {
        eventlog(eventlog_level_error,"hashtable_purge","got NULL hashtable");
        return -1;
    }
    
#ifdef HASHTABLE_DEBUG
    hashtable_check(hashtable);
#endif
    
    for (row=0; row<hashtable->num_rows; row++)
    {
	head = NULL;
	change = NULL;
	for (curr=hashtable->rows[row]; curr; curr=next)
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
		if (!head)
		    head = curr;
		change = &curr->next;
	    }
	}
	hashtable->rows[row] = head;
    }
    
    return 0;
}


extern int hashtable_check(t_hashtable const * hashtable)
{
    unsigned int          emptycnt;
    unsigned int          validcnt;
    unsigned int          row;
    unsigned int          temp;
    t_internentry const * tail;
    t_internentry const * curr;
    
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_check","got NULL hashtable");
	return -1;
    }
    if (hashtable->num_rows<1)
    {
        eventlog(eventlog_level_error,"hashtable_check","num_rows=%u",hashtable->num_rows);
	return -1;
    }
    if (!hashtable->rows)
    {
        eventlog(eventlog_level_error,"hashtable_check","hashtable->rows is NULL");
	return -1;
    }
    
    emptycnt=validcnt = 0;
    for (row=0; row<hashtable->num_rows; row++)
    {
	tail = NULL;
	for (curr=hashtable->rows[row]; curr; curr=curr->next)
	{
	    if (tail)
	    {
		if (curr==tail) /* tail is currently the previous node */
		{
		    eventlog(eventlog_level_error,"list_check","row %u is circular (curr==prev==%p)",row,curr);
		    return -1;
		}
		if (curr->next==tail)
		{
		    eventlog(eventlog_level_error,"list_check","row %u is circular (curr->next==prev==%p)",row,curr);
		    return -1;
		}
		for (temp=0; temp<hashtable->num_rows; temp++)
		    if (curr==hashtable->rows[temp])
		    {
			eventlog(eventlog_level_error,"list_check","row %u is circular (curr==rows[%u]==%p)",row,temp,curr);
			return -1;
		    }
	    }
	    if (curr->data==&nodata)
		emptycnt++;
	    else
		validcnt++;
	    tail = curr;
	}
    }
    
    if (emptycnt>10 && emptycnt>validcnt+5) /* arbitrary heuristic to detect missing list_purge() cal
ls */
	eventlog(eventlog_level_warn,"hashtable_check","emptycnt=%u but validcnt=%u",emptycnt,validcnt);
    if (hashtable->len!=validcnt)
    {
	eventlog(eventlog_level_error,"hashtable_check","hashtable->len=%u but validcnt=%u",hashtable->len,validcnt);
	return -1;
    }
    
    return 0;
}


extern unsigned int hashtable_get_length(t_hashtable const * hashtable)
{
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_get_length","got NULL hashtable");
	return 0;
    }
    
    return hashtable->len;
}


extern int hashtable_insert_data(t_hashtable * hashtable, void * data, unsigned int hash)
{
    unsigned int    row;
    t_internentry * entry;
    
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_insert_data","got NULL hashtable");
	return -1;
    }
    
    if (!(entry = malloc(sizeof(t_internentry))))
    {
	eventlog(eventlog_level_error,"hashtable_insert_data","could not allocate memory for entry");
	return -1;
    }
    entry->data = data;
    
    row = hash%hashtable->num_rows;
    entry->next = hashtable->rows[row];
    hashtable->rows[row] = entry;
    hashtable->len++;
    
    return 0;
}


extern t_entry * hashtable_get_entry_by_data(t_hashtable const * hashtable, void const * data, unsigned int hash)
{
    unsigned int    row;
    t_internentry * curr;
    
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_get_entry_by_data","got NULL hashtable");
	return NULL;
    }
    
    row = hash%hashtable->num_rows;
    for (curr=hashtable->rows[row]; curr; curr=curr->next)
	if (curr->data==data)
	    return hashtable_entry_export(curr,hashtable,row);
    
    return NULL;
}


extern t_entry const * hashtable_get_entry_by_data_const(t_hashtable const * hashtable, void const * data, unsigned int hash)
{
    unsigned int    row;
    t_internentry * curr;
    
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_get_entry_by_data_const","got NULL hashtable");
	return NULL;
    }
    
    row = hash%hashtable->num_rows;
    for (curr=hashtable->rows[row]; curr; curr=curr->next)
	if (curr->data==data)
	    return hashtable_entry_export(curr,hashtable,row);
    
    return NULL;
}


extern int hashtable_remove_entry(t_hashtable * hashtable, t_entry * entry)
{
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_remove_entry","got NULL hashtable");
	return -1;
    }
    if (!entry)
    {
	eventlog(eventlog_level_error,"hashtable_remove_entry","got NULL entry");
	return -1;
    }
    if (!entry->real)
    {
	eventlog(eventlog_level_error,"hashtable_remove_entry","entry has NULL real pointer");
	return -1;
    }
    if (entry->real->data==&nodata)
    {
	eventlog(eventlog_level_error,"hashtable_remove_entry","got deleted entry");
	return -1;
    }
    
    entry->real->data = &nodata;
    hashtable->len--;
    
    return 0;
}


extern int hashtable_remove_data(t_hashtable * hashtable, void const * data, unsigned int hash)
{
    t_entry * entry;
    int       retval;
    
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_remove_data","got NULL hashtable");
	return -1;
    }
    
    if (!(entry = hashtable_get_entry_by_data(hashtable,data,hash)))
	return -1;
    
    retval = hashtable_remove_entry(hashtable,entry);
    
    hashtable_entry_release(entry);
    
    return retval;
}


extern void * entry_get_data(t_entry const * entry)
{
    if (!entry)
    {
	eventlog(eventlog_level_error,"entry_get_data","got NULL entry");
	return NULL;
    }
    if (!entry->real)
    {
	eventlog(eventlog_level_error,"entry_get_data","entry has NULL real pointer");
	return NULL;
    }
    if (entry->real->data==&nodata)
    {
	eventlog(eventlog_level_error,"entry_get_data","got deleted entry");
	return NULL;
    }
    
    return entry->real->data;
}


extern void * hashtable_get_data_by_pos(t_hashtable const * hashtable, unsigned int pos)
{
    t_internentry const * curr;
    unsigned int          row;
    unsigned int          len;
    
    if (!hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_get_data_by_pos","got NULL hashtable");
	return NULL;
    }
    
    len = 0;
    row = 0;
    curr = NULL;
    for (;;)
    {
	if (!curr)
	{
	    if (row>=hashtable->num_rows)
		break;
	    curr = hashtable->rows[row++];
	    continue;
	}
	if (curr->data!=&nodata && len++==pos)
	    return curr->data;
	curr = curr->next;
    }
    
    eventlog(eventlog_level_error,"hashtable_get_data_by_pos","requested position %u but len=%u",pos,len);
    return NULL;
}


#ifdef HASHTABLE_DEBUG
extern t_entry * hashtable_get_first_real(t_hashtable const * hashtable, char const * fn, unsigned int ln)
#else
extern t_entry * hashtable_get_first(t_hashtable const * hashtable)
#endif
{
    unsigned int    row;
    t_internentry * curr;
    
    if (!hashtable)
    {
#ifdef HASHTABLE_DEBUG
	eventlog(eventlog_level_error,"hashtable_get_first","got NULL hashtable from %s:%u",fn,ln);
#else
	eventlog(eventlog_level_error,"hashtable_get_first","got NULL hashtable");
#endif
	return NULL;
    }
    
    for (row=0; row<hashtable->num_rows; row++)
	for (curr=hashtable->rows[row]; curr; curr=curr->next)
	    if (curr->data!=&nodata)
		return hashtable_entry_export(curr,hashtable,row);
    
    return NULL;
}


extern t_entry * entry_get_next(t_entry * entry)
{
    t_hashtable const * hashtable;
    unsigned int        row;
    t_internentry *     curr;
    
    if (!entry)
    {
	eventlog(eventlog_level_error,"entry_get_next","got NULL entry");
	return NULL;
    }
    
    hashtable = entry->hashtable;
    
    for (curr=entry->real->next; curr; curr=curr->next)
	if (curr->data!=&nodata)
	{
	    entry->real = curr;
	    return entry;
	}
    
    for (row=entry->row+1; row<hashtable->num_rows; row++)
	for (curr=hashtable->rows[row]; curr; curr=curr->next)
	    if (curr->data!=&nodata)
	    {
		entry->real = curr;
		entry->row = row;
		return entry;
	    }
    
    hashtable_entry_release(entry);
    return NULL;
}


#ifdef HASHTABLE_DEBUG
extern t_entry * hashtable_get_first_matching_real(t_hashtable const * hashtable, unsigned int hash, char const * fn, unsigned int ln)
#else
extern t_entry * hashtable_get_first_matching(t_hashtable const * hashtable, unsigned int hash)
#endif
{
    unsigned int    row;
    t_internentry * curr;
    
    if (!hashtable)
    {
#ifdef HASHTABLE_DEBUG
	eventlog(eventlog_level_error,"hashtable_get_first_matching","got NULL hashtable from %s:%u",fn,ln);
#else
	eventlog(eventlog_level_error,"hashtable_get_first_matching","got NULL hashtable");
#endif
	return NULL;
    }
    
    row = hash%hashtable->num_rows;
    for (curr=hashtable->rows[row]; curr; curr=curr->next)
	if (curr->data!=&nodata)
	    return hashtable_entry_export(curr,hashtable,row);
    
    return NULL;
}


extern t_entry * entry_get_next_matching(t_entry * entry)
{
    t_internentry * curr;
    
    if (!entry)
    {
	eventlog(eventlog_level_error,"entry_get_next_matching","got NULL entry");
	return NULL;
    }
    
    for (curr=entry->real->next; curr; curr=curr->next)
	if (curr->data!=&nodata)
	{
	    entry->real = curr;
	    return entry;
	}
    
    hashtable_entry_release(entry);
    return NULL;
}


extern int hashtable_entry_release(t_entry * entry)
{
    if (!entry)
    {
	eventlog(eventlog_level_error,"hashtable_entry_release","got NULL entry");
	return -1;
    }
    if (!entry->hashtable)
    {
	eventlog(eventlog_level_error,"hashtable_entry_release","got entry with NULL hashtable");
	return -1;
    }
    if (!entry->real)
    {
	eventlog(eventlog_level_error,"hashtable_entry_release","got entry with NULL real pointer");
	return -1;
    }
    if (entry->row>=entry->hashtable->num_rows)
    {
	eventlog(eventlog_level_error,"hashtable_entry_release","entry has bad row %u (max %u)",entry->row,entry->hashtable->num_rows-1);
	return -1;
    }
    
#ifdef HASHTABLE_DEBUG
    hashtable_check(entry->hashtable);
#endif
    free(entry);
    return 0;
}


extern int hashtable_stats(t_hashtable * hashtable)
{
    unsigned int      row;
    t_internentry *   curr;
    unsigned int      rcount, max, min;
    
    if (!hashtable)
    {
        eventlog(eventlog_level_error, __FUNCTION__,"got NULL hashtable");
        return -1;
    }
    
#ifdef HASHTABLE_DEBUG
    hashtable_check(hashtable);
#endif

    max = 0;
    min = hashtable->len;
    for (row=0; row<hashtable->num_rows; row++)
    {
	rcount = 0;
	for (curr=hashtable->rows[row]; curr; curr=curr->next)
	    if (curr->data!=&nodata) rcount++;
	if (rcount > max) max = rcount;
	if (rcount < min) min = rcount;
    }

    eventlog(eventlog_level_info, __FUNCTION__, "hashsize: %u min: %u max: %u avg: %.2f diff: %.2f%c", hashtable->num_rows, min, max, (float)(min + max)/2, (float)(max - min)/max*100, '%');
    return 0;
}


