/*
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
#define TOPIC_INTERNAL_ACCESS
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
#include "compat/strrchr.h"
#include "compat/strdup.h"
#include "compat/strcasecmp.h"
#include "compat/pdir.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include "common/eventlog.h"
#include "common/list.h"
#include "common/field_sizes.h"
#include "common/xalloc.h"
#include "prefs.h"
#include "topic.h"
#include "common/setup_after.h"

static t_list * topiclist_head=NULL;

t_topic * get_topic(char const * channel_name)
{
  t_elem  * curr;
  t_topic * topic;

  if (topiclist_head)
  {
    LIST_TRAVERSE(topiclist_head,curr)
    {
      if (!(topic = (t_topic*)elem_get_data(curr)))
      {
        eventlog(eventlog_level_error,__FUNCTION__,"found NULL entry in list");
        continue;
      }
      if (strcasecmp(channel_name,topic->channel_name)==0) return topic;
    }
  }
  return NULL;
}

char * channel_get_topic(char const * channel_name)
{
  t_topic * topic;

  if (!(topic = get_topic(channel_name)))
    return NULL;
  else
    return topic->topic;
}

int topiclist_save(char const * topic_file)
{
  t_elem  * curr;
  t_topic * topic;
  FILE * fp;

  if (topiclist_head)
  {

    if ((fp = fopen(topic_file,"w"))==NULL) 
    {
      eventlog(eventlog_level_error, __FUNCTION__,"can't open topic file");
      return -1;
    }

    LIST_TRAVERSE(topiclist_head,curr)
    {
      if (!(topic = (t_topic*)elem_get_data(curr)))
      {
        eventlog(eventlog_level_error,__FUNCTION__,"found NULL entry in list");
        continue;
      }
      if (topic->save == DO_SAVE_TOPIC) 
        fprintf(fp,"\"%s\",\"%s\"\n",topic->channel_name,topic->topic);
    }

    fclose(fp);
  }

  return 0;
}

int topiclist_add_topic(char const * channel_name, char const * topic_text, int do_save)
{
  t_topic * topic;
  
  topic = (t_topic*)xmalloc(sizeof(t_topic));
  topic->channel_name = xstrdup(channel_name);
  topic->topic = xstrdup(topic_text);
  list_prepend_data(topiclist_head,topic);
  topic->save = do_save;
  return 0;
}

int channel_set_topic(char const * channel_name, char const * topic_text, int do_save)
{
  t_topic * topic;

  if (!(channel_name))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"got NULL channel_name");
    return -1;
  }

  if (!(topic_text))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"got NULL topic");
    return -1;
  }

  if ((topic = get_topic(channel_name)))
  {
    xfree((void *)topic->topic);
    topic->topic = xstrdup(topic_text);
  }
  else
  {
    topiclist_add_topic(channel_name, topic_text,do_save);
  }

  if (do_save == DO_SAVE_TOPIC)
  {
    if (topiclist_save(prefs_get_topicfile())<0)
    {
      eventlog(eventlog_level_error,__FUNCTION__,"error saving topic list");
      return -1;
    }
  }

  return 0;
}

int topiclist_load(char const * topicfile)
{
  FILE * fp;
  char channel_name[CHANNEL_NAME_LEN];
  char topic[MAX_TOPIC_LEN];

  // make sure to unload previous topiclist before loading again
  if (topiclist_head) topiclist_unload();

  if ((fp = fopen(topicfile,"r"))==NULL) 
  {
    eventlog(eventlog_level_error, __FUNCTION__,"can't open topic file");
    return -1;
  }
  
  topiclist_head = list_create();
  
  eventlog(eventlog_level_trace,__FUNCTION__,"start reading topic file");

  while (fscanf(fp,"\"%[^\"]\",\"%[^\"]\"\n",channel_name,topic)==2)
  {
    topiclist_add_topic(channel_name,topic,DO_SAVE_TOPIC);
    eventlog(eventlog_level_trace,__FUNCTION__,"channel: %s topic: \"%s\"",channel_name,topic);
  }

  eventlog(eventlog_level_trace,__FUNCTION__,"finished reading topic file");

  fclose(fp);
  return 0;
}

int topiclist_unload(void)
{
  t_elem  * curr;
  t_topic * topic;

  if (topiclist_head)
  {
    LIST_TRAVERSE(topiclist_head,curr)
    {
      if (!(topic = (t_topic*)elem_get_data(curr)))
      {
        eventlog(eventlog_level_error,__FUNCTION__,"found NULL entry in list");
        continue;
      }

      if (topic->channel_name) xfree((void *)topic->channel_name);
      if (topic->topic) xfree((void *)topic->topic);
      xfree((void *)topic);
      list_remove_elem(topiclist_head,&curr);
    }
    if (list_destroy(topiclist_head)<0)
      return -1;

    topiclist_head = NULL;
  }
  return 0;
}
