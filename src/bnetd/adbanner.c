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
#define ADBANNER_INTERNAL_ACCESS
#include "common/setup_before.h"
#include <stdio.h>
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
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#include "compat/strrchr.h"
#include "compat/strdup.h"
#include "compat/strcasecmp.h"
#include <errno.h>
#include "compat/strerror.h"
#include "common/tag.h"
#include "common/bn_type.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/util.h"
#include "common/xalloc.h"
#include "connection.h"
#include "adbanner.h"
#include "common/setup_after.h"


static t_list * adbannerlist_init_head=NULL;
static t_list * adbannerlist_start_head=NULL;
static t_list * adbannerlist_norm_head=NULL;
static unsigned int adbannerlist_init_count=0;
static unsigned int adbannerlist_start_count=0;
static unsigned int adbannerlist_norm_count=0;


static t_adbanner * adbanner_create(unsigned int id, unsigned int next_id, unsigned int delay, bn_int tag, char const * filename, char const * link, char const * client);
static int adbanner_destroy(t_adbanner const * ad);
static int adbannerlist_insert(t_list * head, unsigned int * count, char const * filename, unsigned int delay, char const * link, unsigned int next_id, char const * client);
static t_adbanner * adbannerlist_find_adbanner_by_id(t_list const * head, unsigned int id, t_clienttag clienttag);
static t_adbanner * adbannerlist_get_random(t_list const * head, t_clienttag client);


static t_adbanner * adbanner_create(unsigned int id, unsigned int next_id, unsigned int delay, bn_int tag, char const * filename, char const * link, char const * client)
{
    t_adbanner * ad;
    
    if (!filename)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL filename");
	return NULL;
    }
    if (!link)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL link");
	return NULL;
    }
    if (!client)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL client");
	return NULL;
    }
    
    ad = xmalloc(sizeof(t_adbanner));
    
    ad->id           = id;
    ad->extensiontag = bn_int_get(tag);
    ad->delay        = delay;
    ad->next         = next_id;
    ad->filename = xstrdup(filename);
    ad->link = xstrdup(link);

    if (strcasecmp(client,"NULL")==0)
    	ad->client = 0;
    else
	ad->client = clienttag_str_to_uint(client);

    /* I'm aware that this statement looks stupid */
    if (ad->client && (!tag_check_client(ad->client)))
    {
    	eventlog(eventlog_level_error,__FUNCTION__,"banner with invalid clienttag \"%s\"encountered",client);
	xfree((void *)ad->link);
	xfree((void *)ad->filename); /* avoid warning */
	xfree(ad);
	return NULL;
    }


    eventlog(eventlog_level_debug,__FUNCTION__,"created ad id=0x%08x filename=\"%s\" extensiontag=0x%04x delay=%u link=\"%s\" next_id=0x%08x client=\"%s\"",ad->id,ad->filename,ad->extensiontag,ad->delay,ad->link,ad->next,ad->client?client:"");
    return ad;
}


static int adbanner_destroy(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ad");
	return -1;
    }
    
    xfree((void *)ad->filename); /* avoid warning */
    xfree((void *)ad->link); /* avoid warning */
    xfree((void *)ad); /* avoid warning */
    
    return 0;
}

extern t_adbanner * adbanner_get(t_connection const * c, unsigned int id)
{
  t_adbanner * banner;
  t_clienttag ctag = conn_get_clienttag(c);

  banner = adbannerlist_find_adbanner_by_id(adbannerlist_init_head,id,ctag);
  if (!banner) banner = adbannerlist_find_adbanner_by_id(adbannerlist_start_head,id,ctag);
  if (!banner) banner = adbannerlist_find_adbanner_by_id(adbannerlist_norm_head,id,ctag);

  return banner;
}

extern t_adbanner * adbanner_pick(t_connection const * c, unsigned int prev_id)
{
    t_adbanner const * prev;
    unsigned int       next_id;
    t_clienttag ctag;
    
    if (!c)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return NULL;
    }

    ctag = conn_get_clienttag(c);

    /* eventlog(eventlog_level_debug,__FUNCTION__,"prev_id=%u init_count=%u start_count=%u norm_count=%u",prev_id,adbannerlist_init_count,adbannerlist_start_count,adbannerlist_norm_count); */
    /* if this is the first ad, randomly choose an init sequence (if there is one) */
    if (prev_id==0 && adbannerlist_init_count>0)
        return adbannerlist_get_random(adbannerlist_init_head,ctag);
//        return list_get_data_by_pos(adbannerlist_init_head,((unsigned int)rand())%adbannerlist_init_count);
    /* eventlog(eventlog_level_debug,__FUNCTION__,"not sending init banner"); */
    
    /* find the previous adbanner */
    if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_init_head,prev_id,ctag)))
	next_id = prev->next;
    else if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_start_head,prev_id,ctag)))
	next_id = prev->next;
    else if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_norm_head,prev_id,ctag)))
	next_id = prev->next;
    else
	next_id = 0;
    
    /* return its next ad if there is one */
    if (next_id)
    {
	t_adbanner * curr;
	
	if ((curr = adbannerlist_find_adbanner_by_id(adbannerlist_init_head,next_id,ctag)))
	    return curr;
	if ((curr = adbannerlist_find_adbanner_by_id(adbannerlist_start_head,next_id,ctag)))
	    return curr;
	if ((curr = adbannerlist_find_adbanner_by_id(adbannerlist_norm_head,next_id,ctag)))
	    return curr;
	
	eventlog(eventlog_level_error,__FUNCTION__,"could not locate next requested ad with id 0x%06x",next_id);
    }
    /* eventlog(eventlog_level_debug,__FUNCTION__,"not sending next banner"); */
    
    /* otherwise choose another starting point randomly */
    if (adbannerlist_start_count>0)
	return adbannerlist_get_random(adbannerlist_start_head,ctag);

    /* eventlog(eventlog_level_debug,__FUNCTION__,"not sending start banner... nothing to return"); */
    return NULL; /* nothing else to return */
}


extern unsigned int adbanner_get_id(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ad");
	return 0;
    }
    return ad->id;
}


extern unsigned int adbanner_get_extensiontag(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ad");
	return 0;
    }
    return ad->extensiontag;
}


extern char const * adbanner_get_filename(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ad");
	return NULL;
    }
    return ad->filename;
}


extern char const * adbanner_get_link(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ad");
	return NULL;
    }
    return ad->link;
}


extern t_clienttag adbanner_get_client(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL ad");
	return CLIENTTAG_UNKNOWN_UINT;
    }
    return ad->client;
}


static t_adbanner * adbannerlist_find_adbanner_by_id(t_list const * head, unsigned int id, t_clienttag clienttag)
{
    t_elem const * curr;
    t_adbanner *   temp;
    
    if (!head)
	return NULL;
    
    LIST_TRAVERSE_CONST(head,curr)
    {
        if (!(temp = elem_get_data(curr)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"found NULL adbanner in list");
	    continue;
	}
	if (temp->id==id && (temp->client == 0 || temp->client == clienttag))
	    return temp;
    }
    
    return NULL;
}

/*
 * Dizzy: maybe we should use a temporary list, right now we parse the list for
 * 2 times. It should not matter for servers without more than 20 ads :)
*/
static t_adbanner * adbannerlist_get_random(t_list const * head, t_clienttag client)
{
    t_elem const * curr;
    t_adbanner *   temp;
    unsigned int ccount, ocount, pos;

    if (!head)
	return NULL;

    ocount = 0; ccount = 0;
    LIST_TRAVERSE_CONST(head,curr)
    {
        if (!(temp = elem_get_data(curr)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"found NULL adbanner in list");
	    continue;
	}
	if ((adbanner_get_client(temp) == client))
	    ccount++;
	else if ((adbanner_get_client(temp) == 0))
	    ocount++;
    }
    if (ccount) {
	pos = ((unsigned int)rand())%ccount;
	ccount = 0;
	LIST_TRAVERSE_CONST(head,curr)
	{
    	    if (!(temp = elem_get_data(curr))) continue;
	    if ((adbanner_get_client(temp) == client))
		if (ccount++ == pos) return temp;
	}
	eventlog(eventlog_level_error,__FUNCTION__,"found client ads but couldnt locate random chosed!");
    } else if (ocount) {
	pos = ((unsigned int)rand())%ocount;
	ocount = 0;
	LIST_TRAVERSE_CONST(head,curr)
	{
    	    if (!(temp = elem_get_data(curr))) continue;
	    if ((adbanner_get_client(temp) == 0))
		if (ocount++ == pos) return temp; 
	}
	eventlog(eventlog_level_error,__FUNCTION__,"couldnt locate random chosed!");
    }

    return NULL;
}


static int adbannerlist_insert(t_list * head, unsigned int * count, char const * filename, unsigned int delay, char const * link, unsigned int next_id, char const * client)
{
    t_adbanner * ad;
    unsigned int id;
    char *       ext;
    bn_int       bntag;
    
    assert(head != NULL);
    assert(count != NULL);
    assert(filename != NULL);
    assert(link != NULL);

    if (strlen(filename)<7)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got bad ad filename \"%s\"",filename);
	return -1;
    }
    
    ext = xmalloc(strlen(filename));
    
    if (sscanf(filename,"%*c%*c%x.%s",&id,ext)!=2)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got bad ad filename \"%s\"",filename);
	xfree(ext);
	return -1;
    }
    
    if (strcasecmp(ext,"pcx")==0)
        bn_int_tag_set(&bntag,EXTENSIONTAG_PCX);
    else if (strcasecmp(ext,"mng")==0)
	bn_int_tag_set(&bntag,EXTENSIONTAG_MNG);
    else if (strcasecmp(ext,"smk")==0)
        bn_int_tag_set(&bntag,EXTENSIONTAG_SMK);
    else
    {
	eventlog(eventlog_level_error,__FUNCTION__,"unknown extension on filename \"%s\"",filename);
	xfree(ext);
	return -1;
    }
    xfree(ext);
    
    if (!(ad = adbanner_create(id,next_id,delay,bntag,filename,link,client)))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create ad");
	return -1;
    }
    
    list_prepend_data(head,ad);

    (*count)++;
    
    return 0;
}


extern int adbannerlist_create(char const * filename)
{
    FILE *          fp;
    unsigned int    line;
    unsigned int    pos;
    unsigned int    len;
    char *          buff;
    char *          name;
    char *          when;
    char *          link;
    char *	    client;
    char *          temp;
    unsigned int    delay;
    unsigned int    next_id;
    
    if (!filename)
    {
        eventlog(eventlog_level_error,__FUNCTION__,"got NULL filename");
        return -1;
    }
    
    adbannerlist_init_head = list_create();
    adbannerlist_start_head = list_create();
    adbannerlist_norm_head = list_create();
    
    if (!(fp = fopen(filename,"r")))
    {
        eventlog(eventlog_level_error,__FUNCTION__,"could not open adbanner file \"%s\" for reading (fopen: %s)",filename,pstrerror(errno));
	list_destroy(adbannerlist_norm_head);
	list_destroy(adbannerlist_start_head);
	list_destroy(adbannerlist_init_head);
	adbannerlist_init_head=adbannerlist_start_head=adbannerlist_norm_head = NULL;
        return -1;
    }
    
    for (line=1; (buff = file_get_line(fp)); line++)
    {
        for (pos=0; buff[pos]=='\t' || buff[pos]==' '; pos++);
        if (buff[pos]=='\0' || buff[pos]=='#')
        {
            continue;
        }
        if ((temp = strrchr(buff,'#')))
        {
	    unsigned int endpos;
	    
            *temp = '\0';
	    len = strlen(buff)+1;
            for (endpos=len-1;  buff[endpos]=='\t' || buff[endpos]==' '; endpos--);
            buff[endpos+1] = '\0';
        }
        len = strlen(buff)+1;

        name = xmalloc(len);
        when = xmalloc(len);
        link = xmalloc(len);
        client = xmalloc(len);
	
	if (sscanf(buff," \"%[^\"]\" %[a-z] %u \"%[^\"]\" %x \"%[^\"]\"",name,when,&delay,link,&next_id,client)!=6)
	    {
		eventlog(eventlog_level_error,__FUNCTION__,"malformed line %u in file \"%s\"",line,filename);
		xfree(client);
		xfree(link);
		xfree(name);
         	xfree(when);
		continue;
	    }
	
	if (strcmp(when,"init")==0)
	    adbannerlist_insert(adbannerlist_init_head,&adbannerlist_init_count,name,delay,link,next_id,client);
	else if (strcmp(when,"start")==0)
	    adbannerlist_insert(adbannerlist_start_head,&adbannerlist_start_count,name,delay,link,next_id,client);
	else if (strcmp(when,"norm")==0)
	    adbannerlist_insert(adbannerlist_norm_head,&adbannerlist_norm_count,name,delay,link,next_id,client);
	else
	    eventlog(eventlog_level_error,__FUNCTION__,"when field has unknown value on line %u in file \"%s\"",line,filename);
	
	xfree(client);
	xfree(link);
	xfree(name);
        xfree(when);
    }
    
    file_get_line(NULL); // clear file_get_line buffer
    if (fclose(fp)<0)
	eventlog(eventlog_level_error,__FUNCTION__,"could not close adbanner file \"%s\" after reading (fclose: %s)",filename,pstrerror(errno));
    return 0;
}


extern int adbannerlist_destroy(void)
{
    t_elem *     curr;
    t_adbanner * ad;
    
    if (adbannerlist_init_head)
    {
	LIST_TRAVERSE(adbannerlist_init_head,curr)
	{
	    if (!(ad = elem_get_data(curr)))
		eventlog(eventlog_level_error,__FUNCTION__,"found NULL adbanner in init list");
	    else
		adbanner_destroy(ad);
	    list_remove_elem(adbannerlist_init_head,&curr);
	}
	list_destroy(adbannerlist_init_head);
	adbannerlist_init_head = NULL;
	adbannerlist_init_count = 0;
    }
    
    if (adbannerlist_start_head)
    {
	LIST_TRAVERSE(adbannerlist_start_head,curr)
	{
	    if (!(ad = elem_get_data(curr)))
		eventlog(eventlog_level_error,__FUNCTION__,"found NULL adbanner in start list");
	    else
		adbanner_destroy(ad);
	    list_remove_elem(adbannerlist_start_head,&curr);
	}
	list_destroy(adbannerlist_start_head);
	adbannerlist_start_head = NULL;
	adbannerlist_start_count = 0;
    }
    
    if (adbannerlist_norm_head)
    {
	LIST_TRAVERSE(adbannerlist_norm_head,curr)
	{
	    if (!(ad = elem_get_data(curr)))
		eventlog(eventlog_level_error,__FUNCTION__,"found NULL adbanner in norm list");
	    else
		adbanner_destroy(ad);
	    list_remove_elem(adbannerlist_norm_head,&curr);
	}
	list_destroy(adbannerlist_norm_head);
	adbannerlist_norm_head = NULL;
	adbannerlist_norm_count = 0;
    }
    
    return 0;
}
