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
#define WATCH_INTERNAL_ACCESS
#include "common/setup_before.h"
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
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#endif
#include "compat/strcasecmp.h"
#include "common/field_sizes.h"
#include "common/list.h"
#include "account.h"
#include "connection.h"
#include "common/eventlog.h"
#include "message.h"
#include "watch.h"
#include "friends.h"
#include "common/tag.h"
#include "common/xalloc.h"
#include "prefs.h"
#include "common/setup_after.h"


namespace pvpgn
{

static t_list * watchlist_head=NULL;


/* who == NULL means anybody */
extern int watchlist_add_events(t_connection * owner, t_account * who, t_clienttag clienttag, unsigned events)
{
    t_elem const * curr;
    t_watch_pair * pair;

    if (!owner)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL owner");
	return -1;
    }

    LIST_TRAVERSE_CONST(watchlist_head,curr)
    {
	pair = (t_watch_pair*)elem_get_data(curr);
	if (!pair) /* should not happen */
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"watchlist contains NULL item");
	    return -1;
	}
	if (pair->owner==owner && pair->who==who && ((clienttag == pair->clienttag)))
	{
	    pair->what |= events;
	    return 0;
	}
    }

    pair = (t_watch_pair*)xmalloc(sizeof(t_watch_pair));
    pair->owner = owner;
    pair->who   = who;
    pair->what  = events;
    pair->clienttag = clienttag;

    list_prepend_data(watchlist_head,pair);

    return 0;
}


/* who == NULL means anybody */
extern int watchlist_del_events(t_connection * owner, t_account * who, t_clienttag clienttag, unsigned events)
{
    t_elem *       curr;
    t_watch_pair * pair;

    if (!owner)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL owner");
	return -1;
    }

    LIST_TRAVERSE(watchlist_head,curr)
    {
	pair = (t_watch_pair*)elem_get_data(curr);
	if (!pair) /* should not happen */
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"watchlist contains NULL item");
	    return -1;
	}
	if (pair->owner==owner && pair->who==who && ((!clienttag) || (clienttag == pair->clienttag)))
	{
	    pair->what &= ~events;
	    if (pair->what==0)
	    {
		if (list_remove_elem(watchlist_head,&curr)<0)
		{
		    eventlog(eventlog_level_error,__FUNCTION__,"could not remove item");
		    pair->owner = NULL;
		}
		else
		    xfree(pair);
	    }

	    return 0;
	}
    }

    return -1; /* not found */
}


/* this differs from del_events because it doesn't return an error if nothing was found */
extern int watchlist_del_all_events(t_connection * owner)
{
    t_elem *       curr;
    t_watch_pair * pair;

    if (!owner)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL owner");
	return -1;
    }

    LIST_TRAVERSE(watchlist_head,curr)
    {
	pair = (t_watch_pair*)elem_get_data(curr);
	if (!pair) /* should not happen */
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"watchlist contains NULL item");
	    return -1;
	}
	if (pair->owner==owner)
	{
	    if (list_remove_elem(watchlist_head,&curr)<0)
	    {
		eventlog(eventlog_level_error,__FUNCTION__,"could not remove item");
		pair->owner = NULL;
	    }
	    else
	      { xfree(pair); }
	}
    }

    return 0;
}

/* this differs from del_events because it doesn't return an error if nothing was found */
extern int watchlist_del_by_account(t_account * who)
{
    t_elem *       curr;
    t_watch_pair * pair;

    if (!who)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL account");
	return -1;
    }

    LIST_TRAVERSE(watchlist_head,curr)
    {
	pair = (t_watch_pair*)elem_get_data(curr);
	if (!pair) /* should not happen */
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"watchlist contains NULL item");
	    return -1;
	}
	if (pair->who==who)
	{
	    if (list_remove_elem(watchlist_head,&curr)<0)
	    {
		eventlog(eventlog_level_error,__FUNCTION__,"could not remove item");
		pair->owner = NULL;
	    }
	    else
	      { xfree(pair); }
	}
    }

    return 0;
}

static int handle_event_whisper(t_account *account, char const *gamename, t_clienttag clienttag, unsigned event)
{
    t_elem const * curr;
    t_watch_pair * pair;
    char msg[512];
    int cnt = 0;
    char const *myusername;
    t_list * flist;
    t_connection * dest_c, * my_c;
    t_friend * fr;
    char const * game_title;

    if (!account)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL account");
	return -1;
    }

    if (!(myusername = account_get_name(account)))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL account name");
	return -1;
    }

    my_c = account_get_conn(account);

    game_title = clienttag_get_title(clienttag);

    /* mutual friends handling */
    flist = account_get_friends(account);
    if(flist)
    {
        if (event == watch_event_joingame) {
    	    if (gamename)
	        sprintf(msg,"Your friend %s has entered a %s game named \"%s\".",myusername,game_title,gamename);
    	    else
	        sprintf(msg,"Your friend %s has entered a %s game",myusername,game_title);
    	}
        if (event == watch_event_leavegame)sprintf(msg,"Your friend %s has left a %s game.",myusername,game_title);
        if (event == watch_event_login)    sprintf(msg,"Your friend %s has entered %s.",myusername,prefs_get_servername());
        if (event == watch_event_logout)   sprintf(msg,"Your friend %s has left %s.",myusername,prefs_get_servername());
        LIST_TRAVERSE(flist,curr)
        {
            if (!(fr = (t_friend*)elem_get_data(curr)))
            {
                eventlog(eventlog_level_error,__FUNCTION__,"found NULL entry in list");
                continue;
            }

            dest_c = connlist_find_connection_by_account(fr->friendacc);

            if (dest_c==NULL) /* If friend is offline, go on to next */
	        continue;
            else {
    		cnt++;	/* keep track of successful whispers */
	    	if(friend_get_mutual(fr))
        	    message_send_text(dest_c,message_type_whisper,NULL,msg);
	    }
    	}
    }
    if (cnt) eventlog(eventlog_level_info,__FUNCTION__,"notified %d friends about %s",cnt,myusername);

    /* watchlist handling */

    if (event == watch_event_joingame)
    {
	if (gamename)
    	    sprintf(msg,"Watched user %s has entered a %s game named \"%s\".",myusername,game_title,gamename);
	else
	    sprintf(msg,"Watched user %s has entered a %s game",myusername,game_title);
    }

    if (event == watch_event_leavegame)sprintf(msg,"Watched user %s has left a %s game.",myusername,game_title);
    if (event == watch_event_login)    sprintf(msg,"Watched user %s has entered %s.",myusername,prefs_get_servername());
    if (event == watch_event_logout)   sprintf(msg,"Watched user %s has left %s",myusername,prefs_get_servername());

    LIST_TRAVERSE_CONST(watchlist_head,curr)
    {
      pair = (t_watch_pair*)elem_get_data(curr);
      if (!pair) /* should not happen */
	{
	  eventlog(eventlog_level_error,__FUNCTION__,"watchlist contains NULL item");
	  return -1;
	}
	if (pair->owner && (!pair->who || pair->who==account) && ((!pair->clienttag) || (clienttag == pair->clienttag)) && (pair->what&event))
	{
	    message_send_text(pair->owner,message_type_whisper,NULL,msg);
	}
    }

    return 0;
}

extern int watchlist_notify_event(t_account * who, char const * gamename, t_clienttag clienttag, t_watch_event event)
{

    switch (event)
    {
    case watch_event_login:
    case watch_event_logout:
    case watch_event_joingame:
    case watch_event_leavegame:
      handle_event_whisper(who,gamename,clienttag,event);
      break;
    default:
      eventlog(eventlog_level_error,__FUNCTION__,"got unknown event %u",(unsigned int)event);
      return -1;
    }
    return 0;
}


extern int watchlist_create(void)
{
    watchlist_head = list_create();
    return 0;
}


extern int watchlist_destroy(void)
{
    t_elem *       curr;
    t_watch_pair * pair;

    if (watchlist_head)
    {
	LIST_TRAVERSE(watchlist_head,curr)
	{
	    pair = (t_watch_pair*)elem_get_data(curr);
	    if (!pair) /* should not happen */
	    {
		eventlog(eventlog_level_error,__FUNCTION__,"watchlist contains NULL item");
		continue;
	    }

	    if (list_remove_elem(watchlist_head,&curr)<0)
        	eventlog(eventlog_level_error,__FUNCTION__,"could not remove item from list");
	    xfree(pair);
	}

	if (list_destroy(watchlist_head)<0)
	    return -1;
	watchlist_head = NULL;
    }

    return 0;
}

}
