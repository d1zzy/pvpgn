/*
 * Copyright (C) 2000 Rob Crittenden (rcrit@greyoak.com)
 * Copyright (C) 2002 Gianluigi Tiesi (sherpya@netfarm.it)
 * Copyright (C) 2004 CreepLord (creeplord@pvpgn.org)
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
#define AUTOUPDATE_INTERNAL_ACCESS
#include "autoupdate.h"

#include <cerrno>
#include <cstring>

#include "compat/strtoul.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/util.h"
#include "common/proginfo.h"
#include "common/tag.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

static t_list * autoupdate_head=NULL;
static FILE * fp = NULL;


/*
 * Open the autoupdate configuration file, create a linked list of the
 * clienttag and the update file for it.  The format of the file is:
 * archtag<tab>clienttag<tab>versiontag<tab>update file
 *
 * Comments begin with # and are ignored.
 *
 * The server assumes that the update file is in the "files" directory
 * so do not include "/" in the filename - it won't be sent
 * (because it is a security risk).
 */

extern int autoupdate_load(char const * filename)
{
    unsigned int   line;
    unsigned int   pos;
    char *         buff;
    char *         temp;
    char const *   archtag;
    char const *   clienttag;
    char const *   mpqfile;
    char const *   versiontag;
    t_autoupdate * entry;

    if (!filename) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL filename");
	return -1;
    }

    if (!(fp = fopen(filename,"r"))) {
	eventlog(eventlog_level_error, __FUNCTION__, "could not open file \"%s\" for reading (fopen: %s)", filename, std::strerror(errno));
	return -1;
    }

    autoupdate_head = list_create();

    for (line=1; (buff = file_get_line(fp)); line++) {
	for (pos=0; buff[pos]=='\t' || buff[pos]==' '; pos++);

	if (buff[pos]=='\0' || buff[pos]=='#') {
	    continue;
	}

	if ((temp = std::strrchr(buff,'#'))) {
	    unsigned int len;
	    unsigned int endpos;

	    *temp = '\0';
	    len = std::strlen(buff)+1;
	    for (endpos=len-1;  buff[endpos]=='\t' || buff[endpos]==' '; endpos--);
	    buff[endpos+1] = '\0';
	}

	/* FIXME: use next_token instead of std::strtok */
	if (!(archtag = std::strtok(buff, " \t"))) { /* std::strtok modifies the string it is passed */
	    eventlog(eventlog_level_error,__FUNCTION__,"missing archtag on line %u of file \"%s\"",line,filename);
	    continue;
	}
	if (!(clienttag = std::strtok(NULL," \t"))) {
	    eventlog(eventlog_level_error,__FUNCTION__,"missing clienttag on line %u of file \"%s\"",line,filename);
	    continue;
	}
        if (!(versiontag = std::strtok(NULL, " \t"))) {
	    eventlog(eventlog_level_error,__FUNCTION__,"missing versiontag on line %u of file \"%s\"",line,filename);
	    continue;
	}
	if (!(mpqfile = std::strtok(NULL," \t"))) {
	    eventlog(eventlog_level_error,__FUNCTION__,"missing mpqfile on line %u of file \"%s\"",line,filename);
	    continue;
	}

	entry = (t_autoupdate*)xmalloc(sizeof(t_autoupdate));

	if (!tag_check_arch((entry->archtag = tag_str_to_uint(archtag)))) {
	    eventlog(eventlog_level_error,__FUNCTION__,"got unknown archtag");
	    xfree(entry);
	    continue;
	}
	if (!tag_check_client((entry->clienttag = tag_str_to_uint(clienttag)))) {
	    eventlog(eventlog_level_error,__FUNCTION__,"got unknown clienttag");
	    xfree(entry);
	    continue;
	}
	entry->versiontag = xstrdup(versiontag);
	entry->mpqfile = xstrdup(mpqfile);

	eventlog(eventlog_level_debug,__FUNCTION__,"update '%s' version '%s' with file %s",clienttag,versiontag,mpqfile);

	list_append_data(autoupdate_head,entry);
    }
    file_get_line(NULL); // clear file_get_line buffer
    fclose(fp);
    return 0;
}

/*
 * Free up all of the elements in the linked list
 */

extern int autoupdate_unload(void)
{
    if (autoupdate_head) {
	t_elem *       curr;
	t_autoupdate * entry;
	LIST_TRAVERSE(autoupdate_head,curr)
	{
	    if (!(entry = (t_autoupdate*)elem_get_data(curr)))
		eventlog(eventlog_level_error,__FUNCTION__,"found NULL entry in list");
	    else {
		xfree((void *)entry->versiontag);	/* avoid warning */
		xfree((void *)entry->mpqfile);		/* avoid warning */
		xfree(entry);
	    }
	    list_remove_elem(autoupdate_head,&curr);
	}

	if (list_destroy(autoupdate_head)<0) return -1;
	autoupdate_head = NULL;
    }
    return 0;
}

/*
 *  Check to see if an update exists for the clients version
 *  return file name if there is one
 *  retrun NULL if no update exists
 */

extern char * autoupdate_check(t_tag archtag, t_tag clienttag, t_tag gamelang, char const * versiontag)
{
    if (autoupdate_head) {
	t_elem const * curr;
	t_autoupdate * entry;
	char * temp;

	LIST_TRAVERSE_CONST(autoupdate_head,curr)
	{
	    if (!(entry = (t_autoupdate*)elem_get_data(curr))) {
		eventlog(eventlog_level_error,__FUNCTION__,"found NULL entry in list");
		continue;
	    }

	    if (entry->archtag != archtag)
		continue;
	    if (entry->clienttag != clienttag)
		continue;
	    if (std::strcmp(entry->versiontag, versiontag) != 0)
		continue;

	    /* if we have a gamelang then add it to the mpq file name */
	    if ((gamelang) && // so far only WAR3 uses gamelang specific MPQs!
	        ((clienttag == CLIENTTAG_WARCRAFT3_UINT) || (clienttag == CLIENTTAG_WAR3XP_UINT))){
		char gltag[5];
		char * tempmpq;
		char * extention;

		tag_uint_to_str(gltag,gamelang);
		tempmpq = xstrdup(entry->mpqfile);

		temp = (char*)xmalloc(std::strlen(tempmpq)+6);

		extention = std::strrchr(tempmpq,'.');
		*extention = '\0';
		extention++;

		sprintf(temp, "%s_%s.%s", tempmpq, gltag, extention);

		xfree((void *)tempmpq);
		return temp;
	    }
	    temp = xstrdup(entry->mpqfile);
	    return temp;
	}
    }
    return NULL;
}

}

}
