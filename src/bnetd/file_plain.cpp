/*
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 2000,2001  Marco Ziech (mmz@gmx.net)
 * Copyright (C) 2002,2003,2004 Dizzy 
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
#include "compat/strchr.h"
#include "compat/strdup.h"
#include "compat/strcasecmp.h"
#include "compat/strncasecmp.h"
#include <ctype.h>
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#include "compat/char_bit.h"
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
#include <errno.h>
#include "compat/strerror.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#else
# ifdef WIN32
#  include <io.h>
# endif
#endif
#include "compat/pdir.h"
#include "common/eventlog.h"
#include "prefs.h"
#include "common/util.h"
#include "common/field_sizes.h"
#include "common/bnethash.h"
#define CLAN_INTERNAL_ACCESS
#include "common/introtate.h"
#include "account.h"
#include "common/hashtable.h"
#include "storage.h"
#include "storage_file.h"
#include "common/list.h"
#include "common/xalloc.h"
#include "connection.h"
#include "watch.h"
#include "clan.h"
#undef CLAN_INTERNAL_ACCESS
#include "common/elist.h"
#include "attr.h"
#include "common/tag.h"
#include "common/setup_after.h"

/* plain file storage API functions */

static t_attr * plain_read_attr(const char *filename, const char *key);
static int plain_read_attrs(const char *filename, t_read_attr_func cb, void *data);
static int plain_write_attrs(const char *filename, const t_hlist *attributes);

/* file_engine struct populated with the functions above */

t_file_engine file_plain = {
    plain_read_attr,
    plain_read_attrs,
    plain_write_attrs
};


static int plain_write_attrs(const char *filename, const t_hlist *attributes)
{
    FILE       *  accountfile;
    t_hlist    *  curr;
    t_attr     *  attr;
    char const *  key;
    char const *  val;

    if (!(accountfile = fopen(filename,"w"))) {
	eventlog(eventlog_level_error, __FUNCTION__, "unable to open file \"%s\" for writing (fopen: %s)",filename,pstrerror(errno));
	return -1;
    }

    hlist_for_each(curr, attributes) {
	attr = hlist_entry(curr, t_attr, link);

	if (attr_get_key(attr))
	    key = escape_chars(attr_get_key(attr),strlen(attr_get_key(attr)));
	else {
	    eventlog(eventlog_level_error, __FUNCTION__, "attribute with NULL key in list");
	    key = NULL;
	}

	if (attr_get_val(attr))
	    val = escape_chars(attr_get_val(attr),strlen(attr_get_val(attr)));
	else {
	    eventlog(eventlog_level_error, __FUNCTION__, "attribute with NULL val in list");
	    val = NULL;
	}

	if (key && val) {
	    if (strncmp("BNET\\CharacterDefault\\", key, 20) == 0) {
		eventlog(eventlog_level_debug, __FUNCTION__, "skipping attribute key=\"%s\"",attr->key);
	    } else {
		eventlog(eventlog_level_debug, __FUNCTION__, "saving attribute key=\"%s\" val=\"%s\"",attr->key,attr->val);
		fprintf(accountfile,"\"%s\"=\"%s\"\n",key,val);
	    }
	} else eventlog(eventlog_level_error, __FUNCTION__,"could not save attribute key=\"%s\"",attr->key);

	if (key) xfree((void *)key); /* avoid warning */
	if (val) xfree((void *)val); /* avoid warning */

	attr_clear_dirty(attr);
    }

    if (fclose(accountfile)<0) {
	eventlog(eventlog_level_error, __FUNCTION__, "could not close account file \"%s\" after writing (fclose: %s)",filename,pstrerror(errno));
	return -1;
    }

    return 0;
}

static int plain_read_attrs(const char *filename, t_read_attr_func cb, void *data)
{
    FILE *       accountfile;
    unsigned int line;
    char const * buff;
    unsigned int len;
    char *       esckey;
    char *       escval;
    char * key;
    char * val;
    
    if (!(accountfile = fopen(filename,"r"))) {
	eventlog(eventlog_level_error, __FUNCTION__,"could not open account file \"%s\" for reading (fopen: %s)", filename, pstrerror(errno));
	return -1;
    }

    for (line=1; (buff=file_get_line(accountfile)); line++) {
	if (buff[0]=='#' || buff[0]=='\0') {
	    continue;
	}

	if (strlen(buff)<6) /* "?"="" */ {
	    eventlog(eventlog_level_error, __FUNCTION__, "malformed line %d of account file \"%s\"", line, filename);
	    continue;
	}

	len = strlen(buff)-5+1; /* - ""="" + NUL */
	esckey = (char*)xmalloc(len);
	escval = (char*)xmalloc(len);

	if (sscanf(buff,"\"%[^\"]\" = \"%[^\"]\"",esckey,escval)!=2) {
	    if (sscanf(buff,"\"%[^\"]\" = \"\"",esckey)!=1) /* hack for an empty value field */ {
		eventlog(eventlog_level_error, __FUNCTION__,"malformed entry on line %d of account file \"%s\"", line, filename);
		xfree(escval);
		xfree(esckey);
		continue;
	    }
	    escval[0] = '\0';
	}
	
	key = unescape_chars(esckey);
	val = unescape_chars(escval);

/* eventlog(eventlog_level_debug,__FUNCTION__,"strlen(esckey)=%u (%c), len=%u",strlen(esckey),esckey[0],len);*/
	xfree(esckey);
	xfree(escval);
	
	if (cb(key,val,data))
	    eventlog(eventlog_level_error, __FUNCTION__, "got error from callback (key: '%s' val:'%s')", key, val);

	if (key) xfree((void *)key); /* avoid warning */
	if (val) xfree((void *)val); /* avoid warning */
    }

    file_get_line(NULL); // clear file_get_line buffer

    if (fclose(accountfile)<0) 
	eventlog(eventlog_level_error, __FUNCTION__, "could not close account file \"%s\" after reading (fclose: %s)", filename, pstrerror(errno));

    return 0;
}

static t_attr * plain_read_attr(const char *filename, const char *key)
{
    /* flat file storage doesnt know to read selective attributes */
    return NULL;
}
