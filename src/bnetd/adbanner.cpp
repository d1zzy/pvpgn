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
#include "common/setup_before.h"

#include <stdexcept>

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


namespace pvpgn
{

static t_list * adbannerlist_init_head=NULL;
static t_list * adbannerlist_start_head=NULL;
static t_list * adbannerlist_norm_head=NULL;
static unsigned int adbannerlist_init_count=0;
static unsigned int adbannerlist_start_count=0;
static unsigned int adbannerlist_norm_count=0;


static int adbannerlist_insert(t_list * head, unsigned int * count, char const * filename, unsigned int delay, char const * link, unsigned int next_id, char const * client);
static AdBanner * adbannerlist_find_adbanner_by_id(t_list const * head, unsigned int id, t_clienttag clienttag);
static AdBanner * adbannerlist_get_random(t_list const * head, t_clienttag client);

AdBanner::AdBanner(unsigned id_, bn_int extag, unsigned delay_, unsigned next_, const std::string& fname, const std::string& link_, const char* clientstr)
:id(id_), extensiontag(bn_int_get(extag)), delay(delay_), next(next_), filename(fname), link(link_)
{
	if (!clientstr)
		throw std::runtime_error("Got bad client");

	if (strcasecmp(clientstr,"NULL")==0)
		client = 0;
	else
		client = clienttag_str_to_uint(clientstr);

	/* I'm aware that this statement looks stupid */
	if (client && (!tag_check_client(client)))
		throw std::runtime_error("banner with invalid clienttag \"" + std::string(clientstr) + "\"encountered");

	eventlog(eventlog_level_debug,__FUNCTION__,"created ad id=0x%08x filename=\"%s\" extensiontag=0x%04x delay=%u link=\"%s\" next_id=0x%08x client=\"%s\"",id, filename.c_str(), extensiontag, delay, link.c_str(), next, clientstr ? clientstr : "");
}


AdBanner::~AdBanner() throw()
{
}

const AdBanner*
adbannerlist_find(t_clienttag ctag, unsigned id)
{
	AdBanner* banner;

	banner = adbannerlist_find_adbanner_by_id(adbannerlist_init_head,id,ctag);
	if (!banner) banner = adbannerlist_find_adbanner_by_id(adbannerlist_start_head,id,ctag);
	if (!banner) banner = adbannerlist_find_adbanner_by_id(adbannerlist_norm_head,id,ctag);

	return banner;
}

const AdBanner*
adbannerlist_pick(t_clienttag ctag, unsigned int prev_id)
{
    /* eventlog(eventlog_level_debug,__FUNCTION__,"prev_id=%u init_count=%u start_count=%u norm_count=%u",prev_id,adbannerlist_init_count,adbannerlist_start_count,adbannerlist_norm_count); */
    /* if this is the first ad, randomly choose an init sequence (if there is one) */
	if (prev_id==0 && adbannerlist_init_count>0)
		return adbannerlist_get_random(adbannerlist_init_head,ctag);
//        return list_get_data_by_pos(adbannerlist_init_head,((unsigned int)rand())%adbannerlist_init_count);
    /* eventlog(eventlog_level_debug,__FUNCTION__,"not sending init banner"); */

	AdBanner const * prev;
	unsigned int       next_id;

    /* find the previous adbanner */
	if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_init_head,prev_id,ctag)))
		next_id = prev->getNextId();
	else if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_start_head,prev_id,ctag)))
		next_id = prev->getNextId();
	else if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_norm_head,prev_id,ctag)))
		next_id = prev->getNextId();
	else
		next_id = 0;

	/* return its next ad if there is one */
	if (next_id)
	{
		AdBanner * curr;

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

unsigned
AdBanner::getId() const
{
	return id;
}

unsigned
AdBanner::getNextId() const
{
	return next;
}

unsigned
AdBanner::getExtensionTag() const
{
	return extensiontag;
}


char const *
AdBanner::getFilename() const
{
	return filename.c_str();
}


char const *
AdBanner::getLink() const
{
	return link.c_str();
}


t_clienttag
AdBanner::getClient() const
{
	return client;
}


static AdBanner *
adbannerlist_find_adbanner_by_id(t_list const * head, unsigned int id, t_clienttag clienttag)
{
	if (!head)
		return NULL;

	t_elem const * curr;
	AdBanner *   temp;

	LIST_TRAVERSE_CONST(head,curr)
	{
		if (!(temp = (AdBanner*)elem_get_data(curr)))
		{
			eventlog(eventlog_level_error,__FUNCTION__,"found NULL adbanner in list");
			continue;
		}
		if (temp->getId() == id && (temp->getClient() == 0 || temp->getClient() == clienttag))
			return temp;
	}

	return NULL;
}

/*
 * Dizzy: maybe we should use a temporary list, right now we parse the list for
 * 2 times. It should not matter for servers without more than 20 ads :)
*/
static AdBanner *
adbannerlist_get_random(t_list const * head, t_clienttag client)
{
	if (!head)
		return NULL;

	t_elem const * curr;
	AdBanner *   temp;
	unsigned ccount, ocount, pos;

	ocount = 0; ccount = 0;
	LIST_TRAVERSE_CONST(head,curr)
	{
		if (!(temp = (AdBanner*)elem_get_data(curr)))
		{
			eventlog(eventlog_level_error,__FUNCTION__,"found NULL adbanner in list");
			continue;
		}
		if ((temp->getClient() == client))
			ccount++;
		else if ((temp->getClient() == 0))
			ocount++;
	}

	if (ccount) {
		pos = ((unsigned int)rand())%ccount;
		ccount = 0;
		LIST_TRAVERSE_CONST(head,curr)
		{
			if (!(temp = (AdBanner*)elem_get_data(curr))) continue;
			if ((temp->getClient() == client))
				if (ccount++ == pos) return temp;
		}
		eventlog(eventlog_level_error,__FUNCTION__,"found client ads but couldnt locate random chosed!");
	} else if (ocount) {
		pos = ((unsigned int)rand())%ocount;
		ocount = 0;
		LIST_TRAVERSE_CONST(head,curr)
		{
			if (!(temp = (AdBanner*)elem_get_data(curr))) continue;
			if ((temp->getClient() == 0))
				if (ocount++ == pos) return temp;
		}
		eventlog(eventlog_level_error,__FUNCTION__,"couldnt locate random chosed!");
	}

	return NULL;
}


static int adbannerlist_insert(t_list * head, unsigned int * count, char const * filename, unsigned int delay, char const * link, unsigned int next_id, char const * client)
{
	assert(head != NULL);
	assert(count != NULL);
	assert(filename != NULL);
	assert(link != NULL);

	if (strlen(filename)<7)
	{
		eventlog(eventlog_level_error,__FUNCTION__,"got bad ad filename \"%s\"",filename);
		return -1;
	}

	char *       ext = (char*)xmalloc(strlen(filename));
	unsigned int id;
	if (sscanf(filename,"%*c%*c%x.%s",&id,ext)!=2)
	{
		eventlog(eventlog_level_error,__FUNCTION__,"got bad ad filename \"%s\"",filename);
		xfree(ext);
		return -1;
	}

	bn_int       bntag;
	if (strcasecmp(ext,"pcx")==0)
		bn_int_tag_set(&bntag,EXTENSIONTAG_PCX);
	else if (strcasecmp(ext,"mng")==0)
		bn_int_tag_set(&bntag,EXTENSIONTAG_MNG);
	else if (strcasecmp(ext,"smk")==0)
		bn_int_tag_set(&bntag,EXTENSIONTAG_SMK);
	else {
		eventlog(eventlog_level_error,__FUNCTION__,"unknown extension on filename \"%s\"",filename);
		xfree(ext);
		return -1;
	}
	xfree(ext);

	list_prepend_data(head, new AdBanner(id, bntag, delay, next_id, filename, link, client));

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

        name = (char*)xmalloc(len);
        when = (char*)xmalloc(len);
        link = (char*)xmalloc(len);
        client =(char*) xmalloc(len);

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


static void destroy_adlist(t_list* list)
{
	t_elem *     curr;
	AdBanner * ad;

	LIST_TRAVERSE(list, curr)
	{
		if (!(ad = (AdBanner*)elem_get_data(curr)))
			eventlog(eventlog_level_error,__FUNCTION__,"found NULL adbanner in init list");
		else delete ad;
		list_remove_elem(list, &curr);
	}
	list_destroy(list);
}

extern int adbannerlist_destroy(void)
{
	if (adbannerlist_init_head)
	{
		destroy_adlist(adbannerlist_init_head);
		adbannerlist_init_head = NULL;
		adbannerlist_init_count = 0;
	}

	if (adbannerlist_start_head)
	{
		destroy_adlist(adbannerlist_start_head);
		adbannerlist_start_head = NULL;
		adbannerlist_start_count = 0;
	}

	if (adbannerlist_norm_head)
	{
		destroy_adlist(adbannerlist_norm_head);
		adbannerlist_norm_head = NULL;
		adbannerlist_norm_count = 0;
	}

	return 0;
}

}
