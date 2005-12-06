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
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <errno.h>
#include "output.h"
#include "prefs.h"
#include "connection.h"
#include "game.h"
#include "ladder.h"
#include "server.h"
#include "channel.h"
#include "account.h"
#include "common/util.h"
#include "common/bnettime.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/proginfo.h"
#include "compat/strerror.h"
#include "common/xalloc.h"
#include "common/tag.h"

char * status_filename;

int output_standard_writer(FILE * fp);

/* 
 * Initialisation Output *
 */

extern void output_init(void)
{
    eventlog(eventlog_level_info,__FUNCTION__,"initializing output file");

    if (prefs_get_XML_status_output())
	status_filename = create_filename(prefs_get_outputdir(),"server",".xml"); // WarCraft III
    else
	status_filename = create_filename(prefs_get_outputdir(),"server",".dat"); // WarCraft III

    return;
} 

/* 
 * Write Functions *
 */

static int _glist_cb_xml(t_game *game, void *data)
{
    char clienttag_str[5];

    fprintf((FILE*)data,"\t\t<game><name>%s</name><clienttag>%s</clienttag></game>\n",game_get_name(game),tag_uint_to_str(clienttag_str,game_get_clienttag(game)));

    return 0;
}

static int _glist_cb_simple(t_game *game, void *data)
{
    static int number;
    char clienttag_str[5];

    if (!data) { 
	number = 1;
	return 0;
    }

    fprintf((FILE*)data,"game%d=%s,%s\n",number,tag_uint_to_str(clienttag_str,game_get_clienttag(game)),game_get_name(game));
    number++;

    return 0;
}

int output_standard_writer(FILE * fp)
{
    t_elem const	*curr;
    t_connection	*conn;
    t_channel const	*channel;
    char const		*channel_name;
    int			number;
    char		clienttag_str[5];
    
    if (prefs_get_XML_status_output())
    {
	fprintf(fp,"<?xml version=\"1.0\"?>\n<status>\n");
        fprintf(fp,"\t\t<Version>%s</Version>\n",PVPGN_VERSION);
	fprintf(fp,"\t\t<Uptime>%s</Uptime>\n",seconds_to_timestr(server_get_uptime()));
	fprintf(fp,"\t\t<Users>\n");
	fprintf(fp,"\t\t<Number>%d</Number>\n",connlist_login_get_length());

	LIST_TRAVERSE_CONST(connlist(),curr)
	{
	    conn = (t_connection*)elem_get_data(curr);
	    if (conn_get_account(conn))
		fprintf(fp,"\t\t<user><name>%s</name><clienttag>%s</clienttag><version>%s</version></user>\n",conn_get_username(conn),tag_uint_to_str(clienttag_str,conn_get_clienttag(conn)),conn_get_clientver(conn));
        }

	fprintf(fp,"\t\t</Users>\n");
	fprintf(fp,"\t\t<Games>\n");
	fprintf(fp,"\t\t<Number>%d</Number>\n",gamelist_get_length());
	
	gamelist_traverse(_glist_cb_xml,fp);

	fprintf(fp,"\t\t</Games>\n");
	fprintf(fp,"\t\t<Channels>\n");
	fprintf(fp,"\t\t<Number>%d</Number>\n",channellist_get_length());

	LIST_TRAVERSE_CONST(channellist(),curr)
	{
    	    channel = (t_channel*)elem_get_data(curr);
	    channel_name = channel_get_name(channel);
	    fprintf(fp,"\t\t<channel>%s</channel>\n",channel_name);
	}
	
	fprintf(fp,"\t\t</Channels>\n");
	fprintf(fp,"</status>\n");
	return 0;
    }
    else
    {
	fprintf(fp,"[STATUS]\nVersion=%s\nUptime=%s\nGames=%d\nUsers=%d\nChannels=%d\nUserAccounts=%d\n",PVPGN_VERSION,seconds_to_timestr(server_get_uptime()),gamelist_get_length(),connlist_login_get_length(),channellist_get_length(),accountlist_get_length()); // Status
	fprintf(fp,"[CHANNELS]\n");
	number=1;
	LIST_TRAVERSE_CONST(channellist(),curr)
	{
    	    channel = (t_channel*)elem_get_data(curr);
	    channel_name = channel_get_name(channel);
	    fprintf(fp,"channel%d=%s\n",number,channel_name);
	    number++;
	}

	fprintf(fp,"[GAMES]\n");
	_glist_cb_simple(NULL,NULL);	/* init number */
	gamelist_traverse(_glist_cb_simple,fp);

	fprintf(fp,"[USERS]\n");
	number=1;
	LIST_TRAVERSE_CONST(connlist(),curr)
	{
    	    conn = (t_connection*)elem_get_data(curr);
    	    if (conn_get_account(conn))
	    {
		fprintf(fp,"user%d=%s,%s\n",number,tag_uint_to_str(clienttag_str,conn_get_clienttag(conn)),conn_get_username(conn));
		number++;
	    }
	}
	
	return 0;
    }
}

extern int output_write_to_file(void)
{
    FILE * fp;
  
    if (!status_filename)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL filename");
	return -1;
    }
    
    if (!(fp = fopen(status_filename,"w")))
    {
        eventlog(eventlog_level_error,__FUNCTION__,"could not open file \"%s\" for writing (fopen: %s)",status_filename,pstrerror(errno)); 
        return -1;
    }
    
    output_standard_writer(fp);
    fclose(fp);
    return 0;
}

extern void output_dispose_filename(void)
{
  if (status_filename) xfree(status_filename);
}
