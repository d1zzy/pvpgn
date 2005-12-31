/*
 * Copyright (C) 1998,1999  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
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
#include "file.h"

#include <cstring>
#include <cerrno>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "common/eventlog.h"
#include "common/xalloc.h"
#include "common/bnettime.h"
#include "common/packet.h"
#include "common/util.h"
#include "common/bn_type.h"

#include "prefs.h"
#include "connection.h"
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

static char const * file_get_info(char const * rawname, unsigned int * len, bn_long * modtime);

static char * file_find_default(const char *rawname)
{
    /* Add new default files here */
    const char * defaultfiles[]={"termsofservice-",".txt",
				 "newaccount-",".txt",
				 "chathelp-war3-",".txt",
				 "matchmaking-war3-",".dat",
				 "tos_",".txt",
				 "tos-unicode_", ".txt",
				 NULL,NULL};
    const char ** pattern, **extension;
    char *filename = NULL;

    for (pattern = defaultfiles, extension = defaultfiles + 1; *pattern; pattern+=2, extension+=2)
    	if (!std::strncmp(rawname, *pattern,std::strlen(*pattern))) {	/* Check if there is a default file available for this kind of file */
	    filename = (char*)xmalloc(std::strlen(prefs_get_filedir()) + 1 + std::strlen(*pattern) + 7 + std::strlen(*extension) + 1);

	    std::strcpy(filename, prefs_get_filedir());
	    std::strcat(filename, "/");
	    std::strcat(filename, *pattern);
	    std::strcat(filename, "default");
	    std::strcat(filename, *extension);

	    break;
	}

    return filename;
}

static char const * file_get_info(char const * rawname, unsigned int * len, bn_long * modtime)
{
    char *filename;
    t_bnettime   bt;
    struct stat sfile;

    if (!rawname) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL rawname");
	return NULL;
    }

    if (!len) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL len");
	return NULL;
    }

    if (!modtime) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL modtime");
	return NULL;
    }

    if (std::strchr(rawname,'/') || std::strchr(rawname,'\\')) {
	eventlog(eventlog_level_warn,__FUNCTION__,"got rawname containing '/' or '\\' \"%s\"",rawname);
	return NULL;
    }

    filename = buildpath(prefs_get_filedir(), rawname);

    if (stat(filename, &sfile)<0) { /* if it doesn't exist, try to replace with default file */
	xfree((void*)filename);
	filename = file_find_default(rawname);
	if (!filename) return NULL; /* no default version */

	if (stat(filename, &sfile)<0) { /* try again */
	    /* FIXME: check for lower-case version of filename */
	    xfree(filename);
	    return NULL;
	}
    }

    *len = (unsigned int)sfile.st_size;
    bt = time_to_bnettime(sfile.st_mtime,0);
    bnettime_to_bn_long(bt,modtime);

    return filename;
}


extern int file_to_mod_time(char const * rawname, bn_long * modtime)
{
    char const * filename;
    unsigned int len;

    if (!rawname)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL rawname");
	return -1;
    }
    if (!modtime)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL modtime");
	return -1;
    }

    if (!(filename = file_get_info(rawname, &len, modtime)))
	return -1;

    xfree((void *)filename); /* avoid warning */

    return 0;
}


/* Send a file.  If the file doesn't exist we still need to respond
 * to the file request.  This will set filelen to 0 and send the server
 * reply message and the client will be happy and not hang.
 */
extern int file_send(t_connection * c, char const * rawname, unsigned int adid, unsigned int etag, unsigned int startoffset, int need_header)
{
    char const * filename;
    t_packet *   rpacket;
    std::FILE *       fp;
    unsigned int filelen;
    int          nbytes;

    if (!c)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
	return -1;
    }
    if (!rawname)
    {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL rawname");
	return -1;
    }

    if (!(rpacket = packet_create(packet_class_file)))
    {
	eventlog(eventlog_level_error,__FUNCTION__,"could not create file packet");
	return -1;
    }
    packet_set_size(rpacket,sizeof(t_server_file_reply));
    packet_set_type(rpacket,SERVER_FILE_REPLY);

    if ((filename = file_get_info(rawname,&filelen,&rpacket->u.server_file_reply.timestamp)))
    {
	if (!(fp = std::fopen(filename,"rb")))
	{
	    /* FIXME: check for lower-case version of filename */
	    eventlog(eventlog_level_error,__FUNCTION__,"stat() succeeded yet could not open file \"%s\" for reading (std::fclose: %s)",filename,std::strerror(errno));
	    filelen = 0;
	}
	xfree((void *)filename); /* avoid warning */
    }
    else
    {
	fp = NULL;
	filelen = 0;
	bn_long_set_a_b(&rpacket->u.server_file_reply.timestamp,0,0);
    }

    if (fp)
    {
	if (startoffset<filelen) {
	    std::fseek(fp,startoffset,SEEK_SET);
	} else {
	    eventlog(eventlog_level_warn,__FUNCTION__,"[%d] startoffset is beyond end of file (%u>%u)",conn_get_socket(c),startoffset,filelen);
	    /* Keep the real filesize. Battle.net does it the same way ... */
	    std::fclose(fp);
	    fp = NULL;
	}
    }

    if (need_header)
    {
	/* send the header from the server with the rawname and length. */
	bn_int_set(&rpacket->u.server_file_reply.filelen,filelen);
	bn_int_set(&rpacket->u.server_file_reply.adid,adid);
	bn_int_set(&rpacket->u.server_file_reply.extensiontag,etag);
	/* rpacket->u.server_file_reply.timestamp is set above */
	packet_append_string(rpacket,rawname);
	conn_push_outqueue(c,rpacket);
    }
    packet_del_ref(rpacket);

    /* Now send the data. Since it may be longer than a packet; we use
     * the raw packet class.
     */
    if (!fp)
    {
	eventlog(eventlog_level_warn,__FUNCTION__,"[%d] sending no data for file \"%s\"",conn_get_socket(c),rawname);
	return -1;
    }

    eventlog(eventlog_level_info,__FUNCTION__,"[%d] sending file \"%s\" of length %d",conn_get_socket(c),rawname,filelen);
    for (;;)
    {
	if (!(rpacket = packet_create(packet_class_raw)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"could not create raw packet");
	    if (std::fclose(fp)<0)
		eventlog(eventlog_level_error,__FUNCTION__,"could not close file \"%s\" after reading (std::fclose: %s)",rawname,std::strerror(errno));
	    return -1;
	}
	if ((nbytes = std::fread(packet_get_raw_data_build(rpacket,0),1,MAX_PACKET_SIZE,fp))<(int)MAX_PACKET_SIZE)
	{
	    if (nbytes>0) /* send out last portion */
	    {
		packet_set_size(rpacket,nbytes);
		conn_push_outqueue(c,rpacket);
	    }
	    packet_del_ref(rpacket);
	    if (std::ferror(fp))
		eventlog(eventlog_level_error,__FUNCTION__,"read failed before EOF on file \"%s\" (std::fread: %s)",rawname,std::strerror(errno));
	    break;
	}
	packet_set_size(rpacket,nbytes);
	conn_push_outqueue(c,rpacket);
	packet_del_ref(rpacket);
    }

    if (std::fclose(fp)<0)
	eventlog(eventlog_level_error,__FUNCTION__,"could not close file \"%s\" after reading (std::fclose: %s)",rawname,std::strerror(errno));
    return 0;
}

}

}
