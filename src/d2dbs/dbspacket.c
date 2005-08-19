/*
 * Copyright (C) 2001		sousou	(liupeng.cs@263.net)
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
#include "setup.h"

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#include "compat/statmacros.h"
#include "compat/mkdir.h"
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
#include "compat/memcpy.h"
#include "compat/memmove.h"
#include "compat/strdup.h"
#include "compat/strsep.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
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
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include "compat/inet_ntoa.h"
#include "compat/psock.h"
#include "compat/access.h"
#include "compat/rename.h"
#include "dbserver.h"
#include "dbspacket.h"
#include "d2ladder.h"
#include "charlock.h"
#include "prefs.h"
#include "d2cs/xstring.h"
#include "d2cs/d2cs_d2gs_character.h"
#include "common/bn_type.h"
#include "common/list.h"
#include "common/eventlog.h"
#include "common/addr.h"
#include "common/xalloc.h"
#include "common/setup_after.h"

static unsigned int dbs_packet_savedata_charsave(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int datalen);
static unsigned int dbs_packet_savedata_charinfo(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int datalen);
static unsigned int dbs_packet_getdata_charsave(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int bufsize);
static unsigned int dbs_packet_getdata_charinfo(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int bufsize);
static unsigned int dbs_packet_echoreply(t_d2dbs_connection* conn);
static unsigned int dbs_packet_getdata(t_d2dbs_connection* conn);
static unsigned int dbs_packet_savedata(t_d2dbs_connection* conn);
static unsigned int dbs_packet_charlock(t_d2dbs_connection* conn);
static unsigned int dbs_packet_updateladder(t_d2dbs_connection* conn);
static int dbs_verify_ipaddr(char const * addrlist,t_d2dbs_connection * c);

static int dbs_packet_fix_charinfo(t_d2dbs_connection * conn,char * AccountName,char * CharName,char * charsave);
static void dbs_packet_set_charinfo_level(char * CharName,char * charinfo);

static unsigned int dbs_packet_savedata_charsave(t_d2dbs_connection* conn, char * AccountName,char * CharName,char * data,unsigned int datalen)
{
	char filename[MAX_PATH];
	char savefile[MAX_PATH];
	char bakfile[MAX_PATH];
	unsigned short curlen,readlen,leftlen,writelen;
	FILE * fd;
	
	strtolower(AccountName);
	strtolower(CharName);

	sprintf(filename,"%s/.%s.tmp",d2dbs_prefs_get_charsave_dir(),CharName);
	fd = fopen(filename, "wb");
	if (!fd) {
		eventlog(eventlog_level_error,__FUNCTION__,"open() failed : %s",filename);
		return 0;
	}
	curlen=0;
	leftlen=datalen;
	while(curlen<datalen) {
		if (leftlen>2000) writelen=2000;
		else writelen=leftlen;
		readlen=fwrite(data+curlen,1,writelen,fd);
		if (readlen<=0) {
			fclose(fd);
			eventlog(eventlog_level_error,__FUNCTION__,"write() failed error : %s",strerror(errno));
			return 0;
		}
		curlen+=readlen;
		leftlen-=readlen;
	}
	fclose(fd);

	sprintf(bakfile,"%s/%s",prefs_get_charsave_bak_dir(),CharName);
	sprintf(savefile,"%s/%s",d2dbs_prefs_get_charsave_dir(),CharName);
	if (p_rename(savefile, bakfile)==-1) {
		eventlog(eventlog_level_warn,__FUNCTION__,"error rename %s to %s", savefile, bakfile);
	}
	if (p_rename(filename, savefile)==-1) {
		eventlog(eventlog_level_error,__FUNCTION__,"error rename %s to %s", filename, savefile);
		return 0;
	}
	eventlog(eventlog_level_info,__FUNCTION__,"saved charsave %s(*%s) for gs %s(%d)", CharName, AccountName, conn->serverip, conn->serverid);
	return datalen;
}

static unsigned int dbs_packet_savedata_charinfo(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int datalen)
{
	char savefile[MAX_PATH];
	char bakfile[MAX_PATH];
	char filepath[MAX_PATH];
	char filename[MAX_PATH];
	FILE * fd;
	unsigned short curlen,readlen,leftlen,writelen;
	struct stat statbuf;
	
	strtolower(AccountName);
	strtolower(CharName);

	sprintf(filepath,"%s/%s",prefs_get_charinfo_bak_dir(),AccountName);
	if (stat(filepath,&statbuf)==-1) {
		p_mkdir(filepath,S_IRWXU|S_IRWXG|S_IRWXO );
		eventlog(eventlog_level_info,__FUNCTION__,"created charinfo directory: %s",filepath);
	}

	sprintf(filename,"%s/%s/.%s.tmp",d2dbs_prefs_get_charinfo_dir(),AccountName,CharName);
	fd = fopen(filename, "wb");
	if (!fd) {
		eventlog(eventlog_level_error,__FUNCTION__,"open() failed : %s",filename);
		return 0;
	}

	curlen=0;
	leftlen=datalen;
	while(curlen<datalen) {
		if (leftlen>2000) writelen=2000;
		else writelen=leftlen;
		readlen=fwrite(data+curlen,1,writelen,fd);
		if (readlen<=0) {
			fclose(fd);
			eventlog(eventlog_level_error,__FUNCTION__,"write() failed error : %s",strerror(errno));
			return 0;
		}
		curlen+=readlen;
		leftlen-=readlen;
	}
	fclose(fd);

	sprintf(bakfile,"%s/%s/%s",prefs_get_charinfo_bak_dir(),AccountName,CharName);
	sprintf(savefile,"%s/%s/%s",d2dbs_prefs_get_charinfo_dir(),AccountName,CharName);
	if (p_rename(savefile, bakfile)==-1) {
		eventlog(eventlog_level_info,__FUNCTION__,"error rename %s to %s", savefile, bakfile);
	}
	if (p_rename(filename, savefile)==-1) {
		eventlog(eventlog_level_error,__FUNCTION__,"error rename %s to %s", filename, savefile);
		return 0;
	}
	eventlog(eventlog_level_info,__FUNCTION__,"saved charinfo %s(*%s) for gs %s(%d)", CharName, AccountName, conn->serverip, conn->serverid);
	return datalen;
}

static unsigned int dbs_packet_getdata_charsave(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int bufsize)
{
	char filename[MAX_PATH];
        char filename_d2closed[MAX_PATH];
	FILE * fd;
	unsigned short curlen,readlen,leftlen,writelen;
	long filesize;
	
	strtolower(AccountName);
	strtolower(CharName);
	
	sprintf(filename,"%s/%s",d2dbs_prefs_get_charsave_dir(),CharName);
        sprintf(filename_d2closed,"%s/%s.d2s",d2dbs_prefs_get_charsave_dir(),CharName);
        if ((access(filename, F_OK) < 0) && (access(filename_d2closed, F_OK) == 0))
        {
                p_rename(filename_d2closed, filename);
        }
	fd = fopen(filename, "rb");
	if (!fd) {
		eventlog(eventlog_level_error,__FUNCTION__,"open() failed : %s",filename);
		return 0;
	}
	fseek(fd,0,SEEK_END);
	filesize=ftell(fd);
	rewind(fd);
	if (filesize==-1) {
		fclose(fd);
		eventlog(eventlog_level_error,__FUNCTION__,"lseek() failed");
		return 0;
	}
	if ((signed)bufsize < filesize) {
		fclose(fd);
		eventlog(eventlog_level_error,__FUNCTION__,"not enough buffer");
		return 0;
	}

	curlen=0;
	leftlen=filesize;
	while(curlen < filesize) {
		if (leftlen>2000) writelen=2000;
		else writelen=leftlen;
		readlen=fread(data+curlen,1,writelen,fd);
		if (readlen<=0) {
			fclose(fd);
			eventlog(eventlog_level_error,__FUNCTION__,"read() failed error : %s",strerror(errno));
			return 0;
		}
		leftlen-=readlen;
		curlen+=readlen;
	}
	fclose(fd);
	eventlog(eventlog_level_info,__FUNCTION__,"loaded charsave %s(*%s) for gs %s(%d)", CharName, AccountName, conn->serverip, conn->serverid);
	return filesize;
}

static unsigned int dbs_packet_getdata_charinfo(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int bufsize)
{
	char filename[MAX_PATH];
	FILE * fd;
	unsigned short curlen,readlen,leftlen,writelen;
	long filesize;
	
	strtolower(AccountName);
	strtolower(CharName);

	sprintf(filename,"%s/%s/%s",d2dbs_prefs_get_charinfo_dir(),AccountName,CharName);
	fd = fopen(filename, "rb");
	if (!fd) {
		eventlog(eventlog_level_error,__FUNCTION__,"open() failed : %s",filename);
		return 0;
	}
	fseek(fd,0,SEEK_END);
	filesize=ftell(fd);
	rewind(fd);
	if (filesize==-1) {
		fclose(fd);
		eventlog(eventlog_level_error,__FUNCTION__,"lseek() failed");
		return 0;
	}
	if ((signed)bufsize < filesize) {
		fclose(fd);
		eventlog(eventlog_level_error,__FUNCTION__,"not enough buffer");
		return 0;
	}

	curlen=0;
	leftlen=filesize;
	while(curlen < filesize) {
		if (leftlen>2000)
			writelen=2000;
		else
			writelen=leftlen;
		readlen=fread(data+curlen,1,writelen,fd);
	    if (readlen<=0)
		{
			fclose(fd);
			eventlog(eventlog_level_error,__FUNCTION__,"read() failed error : %s",strerror(errno));
			return 0;
		}
		leftlen-=readlen;
		curlen+=readlen;
	}
	fclose(fd);
	eventlog(eventlog_level_info,__FUNCTION__,"loaded charinfo %s(*%s) for gs %s(%d)", CharName, AccountName, conn->serverip, conn->serverid);
	return filesize;
}

static unsigned int dbs_packet_savedata(t_d2dbs_connection * conn)
{
	unsigned short writelen;
	unsigned short      datatype;
	unsigned short      datalen; 
	unsigned int        result; 
	char AccountName[MAX_NAME_LEN+16];
	char CharName[MAX_NAME_LEN+16];
	char RealmName[MAX_NAME_LEN+16];
	t_d2gs_d2dbs_save_data_request	* savecom; 
	t_d2dbs_d2gs_save_data_reply	* saveret; 
	char * readpos;
	unsigned char * writepos;
	
	readpos=conn->ReadBuf;
	savecom=(t_d2gs_d2dbs_save_data_request	*)readpos;
	datatype=bn_short_get(savecom->datatype);
	datalen=bn_short_get(savecom->datalen);

	readpos+=sizeof(*savecom);
	strncpy(AccountName,readpos,MAX_NAME_LEN);
	AccountName[MAX_NAME_LEN]=0;
	readpos+=strlen(AccountName)+1;
	strncpy(CharName,readpos,MAX_NAME_LEN);
	CharName[MAX_NAME_LEN]=0;
	readpos+=strlen(CharName)+1;
	strncpy(RealmName,readpos,MAX_NAME_LEN);
	RealmName[MAX_NAME_LEN]=0;
	readpos+=strlen(RealmName)+1;

	if (readpos+datalen!=conn->ReadBuf+bn_short_get(savecom->h.size)) {
		eventlog(eventlog_level_error,__FUNCTION__,"request packet size error");
		return -1;
	}

	if (datatype==D2GS_DATA_CHARSAVE) {
		if (dbs_packet_savedata_charsave(conn,AccountName,CharName,readpos,datalen)>0 &&
		    dbs_packet_fix_charinfo(conn,AccountName,CharName,readpos)) {
			result=D2DBS_SAVE_DATA_SUCCESS;
		} else {
			datalen=0;
			result=D2DBS_SAVE_DATA_FAILED    ;
		}
	} else if (datatype==D2GS_DATA_PORTRAIT) {
		/* if level is > 255 , sets level to 255 */
		dbs_packet_set_charinfo_level(CharName,readpos);
		if (dbs_packet_savedata_charinfo(conn,AccountName,CharName,readpos,datalen)>0) {
			result=D2DBS_SAVE_DATA_SUCCESS;
		} else {
			datalen=0;
			result=D2DBS_SAVE_DATA_FAILED;
		}
	} else {
		eventlog(eventlog_level_error,__FUNCTION__,"unknown data type %d",datatype);
		return -1;
	}
	writelen=sizeof(*saveret)+strlen(CharName)+1;
	if (writelen > kBufferSize-conn->nCharsInWriteBuffer) return 0;
	writepos=conn->WriteBuf+conn->nCharsInWriteBuffer;
	saveret=(t_d2dbs_d2gs_save_data_reply *)writepos;
	bn_short_set(&saveret->h.type, D2DBS_D2GS_SAVE_DATA_REPLY);
	bn_short_set(&saveret->h.size,writelen);
	bn_int_set(&saveret->h.seqno,bn_int_get(savecom->h.seqno));
	bn_short_set(&saveret->datatype,bn_short_get(savecom->datatype));
	bn_int_set(&saveret->result,result);
	writepos+=sizeof(*saveret);
	strncpy(writepos,CharName,MAX_NAME_LEN);
	conn->nCharsInWriteBuffer += writelen;
	return 1;
}

static unsigned int dbs_packet_echoreply(t_d2dbs_connection * conn)
{
	conn->last_active=time(NULL);
	return 1;
}

static unsigned int dbs_packet_getdata(t_d2dbs_connection * conn)
{
	unsigned short	writelen;
	unsigned short	datatype;
	unsigned short	datalen; 
	unsigned int	result; 
	char		AccountName[MAX_NAME_LEN+16];
	char		CharName[MAX_NAME_LEN+16];
	char		RealmName[MAX_NAME_LEN+16];
	t_d2gs_d2dbs_get_data_request	* getcom; 
	t_d2dbs_d2gs_get_data_reply	* getret; 
	char		* readpos;
	char		* writepos;
	char		databuf[kBufferSize ];
	t_d2charinfo_file charinfo;
	unsigned short	charinfolen;
	unsigned int	gsid;

	readpos=conn->ReadBuf;
	getcom=(t_d2gs_d2dbs_get_data_request *)readpos;
	datatype=bn_short_get(getcom->datatype);
	
	readpos+=sizeof(*getcom);
	strncpy(AccountName,readpos,MAX_NAME_LEN);
	AccountName[MAX_NAME_LEN]=0;
	readpos+=strlen(AccountName)+1;
	strncpy(CharName,readpos,MAX_NAME_LEN);
	CharName[MAX_NAME_LEN]=0;
	readpos+=strlen(CharName)+1;
	strncpy(RealmName,readpos,MAX_NAME_LEN);
	RealmName[MAX_NAME_LEN]=0;
	readpos+=strlen(RealmName)+1;

	if (readpos != conn->ReadBuf+bn_short_get(getcom->h.size)) {
		eventlog(eventlog_level_error,__FUNCTION__,"request packet size error");
		return -1;
	}
	writepos=conn->WriteBuf+conn->nCharsInWriteBuffer;
	getret=(t_d2dbs_d2gs_get_data_reply *)writepos;
	datalen=0;
	if (datatype==D2GS_DATA_CHARSAVE) {
		if (cl_query_charlock_status(CharName,RealmName,&gsid)!=0) {
			eventlog(eventlog_level_warn,__FUNCTION__,"char %s(*%s)@%s is already locked on gs %u",CharName,AccountName,RealmName,gsid);
			result=D2DBS_GET_DATA_CHARLOCKED;
		} else if (cl_lock_char(CharName,RealmName,conn->serverid) != 0) {
			eventlog(eventlog_level_error,__FUNCTION__,"failed to lock char %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
			result=D2DBS_GET_DATA_CHARLOCKED;
		} else {
			eventlog(eventlog_level_info,__FUNCTION__,"lock char %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
			datalen=dbs_packet_getdata_charsave(conn,AccountName,CharName,databuf,kBufferSize );
			if (datalen>0) {
				result=D2DBS_GET_DATA_SUCCESS;
				charinfolen=dbs_packet_getdata_charinfo(conn,AccountName,CharName,(char *)&charinfo,sizeof(charinfo));
				if (charinfolen>0) {
					result=D2DBS_GET_DATA_SUCCESS;
				} else {
					result=D2DBS_GET_DATA_FAILED;
					if (cl_unlock_char(CharName,RealmName)!=0) {
						eventlog(eventlog_level_error,__FUNCTION__,"failed to unlock char %s(*%s)@%s for gs %s(%d)",CharName,\
							AccountName,RealmName,conn->serverip,conn->serverid);
					} else {
						eventlog(eventlog_level_info,__FUNCTION__,"unlock char %s(*%s)@%s for gs %s(%d)",CharName,\
							AccountName,RealmName,conn->serverip,conn->serverid);
					}
				}
			} else {
				datalen=0;
				result=D2DBS_GET_DATA_FAILED;
				if (cl_unlock_char(CharName,RealmName)!=0) {
					eventlog(eventlog_level_error,__FUNCTION__,"faled to unlock char %s(*%s)@%s for gs %s(%d)",CharName,\
						AccountName,RealmName,conn->serverip,conn->serverid);
				} else {
					eventlog(eventlog_level_info,__FUNCTION__,"unlock char %s(*%s)@%s for gs %s(%d)",CharName,\
						AccountName,RealmName,conn->serverip,conn->serverid);
				}
					
			}
		}
		if (result==D2DBS_GET_DATA_SUCCESS) {
			bn_int_set(&getret->charcreatetime,bn_int_get(charinfo.header.create_time));
			/* FIXME: this should be rewritten to support string formatted time */
			if (bn_int_get(charinfo.header.create_time)>=prefs_get_ladderinit_time()) {
				bn_int_set(&getret->allowladder,1);
			} else {
				bn_int_set(&getret->allowladder,0);
			}
		} else {
			bn_int_set(&getret->charcreatetime,0);
			bn_int_set(&getret->allowladder,0);
		}
	} else if (datatype==D2GS_DATA_PORTRAIT) {
		datalen=dbs_packet_getdata_charinfo(conn,AccountName,CharName,databuf,kBufferSize );
		if (datalen>0) result=D2DBS_GET_DATA_SUCCESS    ;
		else { 
			datalen=0;
			result=D2DBS_GET_DATA_FAILED    ;
		}
	} else {
		eventlog(eventlog_level_error,__FUNCTION__,"unknown data type %d",datatype);
		return -1;
	}
	writelen=datalen+sizeof(*getret)+strlen(CharName)+1;
	if (writelen > kBufferSize-conn->nCharsInWriteBuffer) return 0;
	bn_short_set(&getret->h.type,D2DBS_D2GS_GET_DATA_REPLY);
	bn_short_set(&getret->h.size,writelen);
	bn_int_set(&getret->h.seqno,bn_int_get(getcom->h.seqno));
	bn_short_set(&getret->datatype,bn_short_get(getcom->datatype));
	bn_int_set(&getret->result,result);
	bn_short_set(&getret->datalen,datalen);
	writepos+=sizeof(*getret);
	strncpy(writepos,CharName,MAX_NAME_LEN);
	writepos+=strlen(CharName)+1;
	if (datalen) memcpy(writepos,databuf,datalen);
	conn->nCharsInWriteBuffer += writelen;
	return 1;
}

static unsigned int dbs_packet_updateladder(t_d2dbs_connection * conn)
{
	char CharName[MAX_NAME_LEN+16];
	char RealmName[MAX_NAME_LEN+16];
	t_d2gs_d2dbs_update_ladder	* updateladder; 
	char * readpos;
	t_d2ladder_info			charladderinfo;

	readpos=conn->ReadBuf;
	updateladder=(t_d2gs_d2dbs_update_ladder *)readpos;
	
	readpos+=sizeof(*updateladder);
	strncpy(CharName,readpos,MAX_NAME_LEN);
	CharName[MAX_NAME_LEN]=0;
	readpos+=strlen(CharName)+1;
	strncpy(RealmName,readpos,MAX_NAME_LEN);
	RealmName[MAX_NAME_LEN]=0;
	readpos+=strlen(RealmName)+1;
	if (readpos != conn->ReadBuf+bn_short_get(updateladder->h.size)) {
		eventlog(eventlog_level_error,__FUNCTION__,"request packet size error");
		return -1;
	}

	strcpy(charladderinfo.charname,CharName);
	charladderinfo.experience=bn_int_get(updateladder->charexplow);
	charladderinfo.level=bn_int_get(updateladder->charlevel);
	charladderinfo.status=bn_short_get(updateladder->charstatus);
	charladderinfo.class=bn_short_get(updateladder->charclass);
	eventlog(eventlog_level_info,__FUNCTION__,"update ladder for %s@%s for gs %s(%d)",CharName,RealmName,conn->serverip,conn->serverid);
	d2ladder_update(&charladderinfo);
	return 1;
}

static unsigned int dbs_packet_charlock(t_d2dbs_connection * conn)
{
	char CharName[MAX_NAME_LEN+16];
	char AccountName[MAX_NAME_LEN+16];
	char RealmName[MAX_NAME_LEN+16];
	t_d2gs_d2dbs_char_lock * charlock; 
	char * readpos;

	readpos=conn->ReadBuf;
	charlock=(t_d2gs_d2dbs_char_lock*)readpos;
	
	readpos+=sizeof(*charlock);
	strncpy(AccountName,readpos,MAX_NAME_LEN);
	AccountName[MAX_NAME_LEN]=0;
	readpos+=strlen(AccountName)+1;
	strncpy(CharName,readpos,MAX_NAME_LEN);
	CharName[MAX_NAME_LEN]=0;
	readpos+=strlen(CharName)+1;
	strncpy(RealmName,readpos,MAX_NAME_LEN);
	RealmName[MAX_NAME_LEN]=0;
	readpos+=strlen(RealmName)+1;

	if (readpos != conn->ReadBuf+ bn_short_get(charlock->h.size)) {
		eventlog(eventlog_level_error,__FUNCTION__,"request packet size error");
		return -1;
	}

	if (bn_int_get(charlock->lockstatus)) {
		if (cl_lock_char(CharName,RealmName,conn->serverid)!=0) {
			eventlog(eventlog_level_error,__FUNCTION__,"failed to lock character %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
		} else {
			eventlog(eventlog_level_info,__FUNCTION__,"lock character %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
		}
	} else {
		if (cl_unlock_char(CharName,RealmName) != 0) {
			eventlog(eventlog_level_error,__FUNCTION__,"failed to unlock character %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
		} else {
			eventlog(eventlog_level_info,__FUNCTION__,"unlock character %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
		}
	}
	return 1;
}

/*
	return value:
	1  :  process one or more packet
	0  :  not get a whole packet,do nothing
	-1 :  error
*/
extern int dbs_packet_handle(t_d2dbs_connection* conn) 
{
	unsigned short		readlen,writelen;
	t_d2dbs_d2gs_header	* readhead;
	unsigned short		retval; 

	if (conn->stats==0) {
		if (conn->nCharsInReadBuffer<(signed)sizeof(t_d2gs_d2dbs_connect)) {
			return 0;
		}
		conn->stats=1;
		conn->type=conn->ReadBuf[0];

		if (conn->type==CONNECT_CLASS_D2GS_TO_D2DBS) {
			if (dbs_verify_ipaddr(d2dbs_prefs_get_d2gs_list(),conn)<0) {
				eventlog(eventlog_level_error,__FUNCTION__,"d2gs connection from unknown ip address");
				return -1;
			}
			readlen=1;
			writelen=0;
			eventlog(eventlog_level_info,__FUNCTION__,"set connection type for gs %s(%d) on socket %d", conn->serverip, conn->serverid, conn->sd);
			eventlog_step(prefs_get_logfile_gs(),eventlog_level_info,"set connection type for gs %s(%d) on socket %d", conn->serverip, conn->serverid, conn->sd);
		} else {
			eventlog(eventlog_level_error,__FUNCTION__,"unknown connection type");
			return -1;
		}
		conn->nCharsInReadBuffer -= readlen;
		memmove(conn->ReadBuf,conn->ReadBuf+readlen,conn->nCharsInReadBuffer);
	} else if (conn->stats==1) {
		if (conn->type==CONNECT_CLASS_D2GS_TO_D2DBS) {
			while (conn->nCharsInReadBuffer >= (signed)sizeof(*readhead)) {
				readhead=(t_d2dbs_d2gs_header *)conn->ReadBuf;
				readlen=bn_short_get(readhead->size);
				if (conn->nCharsInReadBuffer < readlen) break;
				switch(bn_short_get(readhead->type)) {
					case D2GS_D2DBS_SAVE_DATA_REQUEST:
						retval=dbs_packet_savedata(conn);
						break;
					case D2GS_D2DBS_GET_DATA_REQUEST:
						retval=dbs_packet_getdata(conn);
						break;
					case D2GS_D2DBS_UPDATE_LADDER:
						retval=dbs_packet_updateladder(conn);
						break;
					case D2GS_D2DBS_CHAR_LOCK:
						retval=dbs_packet_charlock(conn);
						break;
					case D2GS_D2DBS_ECHOREPLY:
						retval=dbs_packet_echoreply(conn);
						break;
					default:
						eventlog(eventlog_level_error,__FUNCTION__,"unknown request type %d",\
							bn_short_get(readhead->type));
						retval=-1;
				}
				if (retval!=1) return retval;
				conn->nCharsInReadBuffer -= readlen;
				memmove(conn->ReadBuf,conn->ReadBuf+readlen,conn->nCharsInReadBuffer);
			}
		} else {
			eventlog(eventlog_level_error,__FUNCTION__,"unknown connection type %d",conn->type);
			return -1;
		}
	} else {
		eventlog(eventlog_level_error,__FUNCTION__,"unknown connection stats");
		return -1;
	}
	return 1;
}

/* FIXME: we should save client ipaddr into c->ipaddr after accept */
static int dbs_verify_ipaddr(char const * addrlist,t_d2dbs_connection * c)
{
	struct	in_addr		in;
	char			* adlist;
	char const		* ipaddr;
	char			* s, * temp;
	t_elem			* elem;
	t_d2dbs_connection	* tempc;
	unsigned int		valid;
	unsigned int		resolveipaddr;

	in.s_addr=htonl(c->ipaddr);
	ipaddr=inet_ntoa(in);
	adlist = xstrdup(addrlist);
	temp=adlist;
	valid=0;
	while ((s=strsep(&temp, ","))) {
		host_lookup(s, &resolveipaddr);
		if(resolveipaddr == 0) continue;
				
		if (!strcmp(ipaddr, (const char *)addr_num_to_ip_str(resolveipaddr))) {
			valid=1;
			break;
		}
	}
	xfree(adlist);
	if (valid) {
		eventlog(eventlog_level_info,__FUNCTION__,"ip address %s is valid",ipaddr);
		LIST_TRAVERSE(dbs_server_connection_list,elem)
		{
			if (!(tempc=elem_get_data(elem))) continue;
			if (tempc !=c && tempc->ipaddr==c->ipaddr) {
				eventlog(eventlog_level_info,__FUNCTION__,"destroying previous connection %d",tempc->serverid);
				dbs_server_shutdown_connection(tempc);
				list_remove_elem(dbs_server_connection_list,&elem);
			}
		}
		c->verified = 1;
		return 0;
	} else {
		eventlog(eventlog_level_info,__FUNCTION__,"ip address %s is invalid",ipaddr);
	}
	return -1;
}

int dbs_check_timeout(void)
{
	t_elem				*elem;
	t_d2dbs_connection		*tempc;
	time_t				now;
	int				timeout;

	now=time(NULL);
	timeout=d2dbs_prefs_get_idletime();
	LIST_TRAVERSE(dbs_server_connection_list,elem)
	{
		if (!(tempc=elem_get_data(elem))) continue;
		if (now-tempc->last_active>timeout) {
			eventlog(eventlog_level_debug,__FUNCTION__,"connection %d timed out",tempc->serverid);
			dbs_server_shutdown_connection(tempc);
			list_remove_elem(dbs_server_connection_list,&elem);
			continue;
		}
	}
	return 0;
}

int dbs_keepalive(void)
{
	t_elem				*elem;
	t_d2dbs_connection		*tempc;
	t_d2dbs_d2gs_echorequest	*echoreq;
	unsigned short			writelen;
	unsigned char			*writepos;
	time_t				now;

	writelen = sizeof(t_d2dbs_d2gs_echorequest);
	now=time(NULL);
	LIST_TRAVERSE(dbs_server_connection_list,elem)
	{
		if (!(tempc=elem_get_data(elem))) continue;
		if (writelen > kBufferSize - tempc->nCharsInWriteBuffer) continue;
		writepos = tempc->WriteBuf + tempc->nCharsInWriteBuffer;
		echoreq  = (t_d2dbs_d2gs_echorequest*)writepos;
		bn_short_set(&echoreq->h.type, D2DBS_D2GS_ECHOREQUEST);
		bn_short_set(&echoreq->h.size, writelen);
		/* FIXME: sequence number not set */
		bn_int_set(&echoreq->h.seqno,  0);
		tempc->nCharsInWriteBuffer += writelen;
	}
	return 0;
}

/*************************************************************************************/
#define CHARINFO_SIZE			0xC0
#define CHARINFO_PORTRAIT_LEVEL_OFFSET	0x89
#define CHARINFO_PORTRAIT_STATUS_OFFSET	0x8A
#define CHARINFO_SUMMARY_LEVEL_OFFSET	0xB8
#define CHARINFO_SUMMARY_STATUS_OFFSET	0xB4
#define CHARINFO_PORTRAIT_GFX_OFFSET	0x72
#define CHARINFO_PORTRAIT_COLOR_OFFSET	0x7E

#define CHARSAVE_LEVEL_OFFSET		0x2B
#define CHARSAVE_STATUS_OFFSET		0x24
#define CHARSAVE_GFX_OFFSET		0x88
#define CHARSAVE_COLOR_OFFSET		0x98

#define charstatus_to_portstatus(status) ((((status & 0xFF00) << 1) | (status & 0x00FF)) | 0x8080)
#define portstatus_to_charstatus(status) (((status & 0x7F00) >> 1) | (status & 0x007F))

static void dbs_packet_set_charinfo_level(char * CharName,char * charinfo)
{
    if (prefs_get_difficulty_hack()) { /* difficulty hack enabled */
	unsigned int	level = bn_int_get(&charinfo[CHARINFO_SUMMARY_LEVEL_OFFSET]);
	unsigned int	plevel = bn_byte_get(&charinfo[CHARINFO_PORTRAIT_LEVEL_OFFSET]);
	
	/* levels 257 thru 355 */
	if (level != plevel) {
	    eventlog(eventlog_level_info,__FUNCTION__,"level mis-match for %s ( %u != %u ) setting to 255",CharName,level,plevel);
	    bn_byte_set((bn_byte *)&charinfo[CHARINFO_PORTRAIT_LEVEL_OFFSET],255);
	    bn_int_set((bn_int *)&charinfo[CHARINFO_SUMMARY_LEVEL_OFFSET],255);
	}
    }
}

static int dbs_packet_fix_charinfo(t_d2dbs_connection * conn,char * AccountName,char * CharName,char * charsave)
{
    if (prefs_get_difficulty_hack()) {
	unsigned char	charinfo[CHARINFO_SIZE];
	unsigned int	level = bn_byte_get(&charsave[CHARSAVE_LEVEL_OFFSET]);
	unsigned short	status = bn_short_get(&charsave[CHARSAVE_STATUS_OFFSET]);
	unsigned short	pstatus = charstatus_to_portstatus(status);
	int		i;
	
	/*
	 * charinfo is only updated from level 1 to 99 (d2gs issue)
	 * from 100 to 256 d2gs does not send it
	 * when value rolls over (level 256 = 0)
	 * and charactar reaches level 257 (rolled over to level 1)
	 * d2gs starts sending it agian until level 356 (rolled over to 100)
	 * is reached agian. etc. etc. etc.
	 */
	if (level == 0) /* level 256, 512, 768, etc */
	    level = 255;
	
	if (level < 100)
	    return 1; /* d2gs will send charinfo - level will be set to 255 at that time if needed */
	
	eventlog(eventlog_level_info,__FUNCTION__,"level %u > 99 for %s",level,CharName);
	
	if(!(dbs_packet_getdata_charinfo(conn,AccountName,CharName,charinfo,CHARINFO_SIZE))) {
	    eventlog(eventlog_level_error,__FUNCTION__,"unable to get charinfo for %s",CharName);
	    return 0;
	}
	
	/* if level in charinfo file is already set to 255,
	 * then is must have been set when d2gs sent the charinfo 
	 * and got a level mis-match (levels 257 - 355)
	 * or level is actually 255. In eather case we set to 255
	 * this should work for any level mod
	 */
	if (bn_byte_get(&charinfo[CHARINFO_PORTRAIT_LEVEL_OFFSET]) == 255)
	    level = 255;
	
	eventlog(eventlog_level_info,__FUNCTION__,"updating charinfo for %s -> level = %u , status = 0x%04X , pstatus = 0x%04X",CharName,level,status,pstatus);
	bn_byte_set((bn_byte *)&charinfo[CHARINFO_PORTRAIT_LEVEL_OFFSET],level);
	bn_int_set((bn_int *)&charinfo[CHARINFO_SUMMARY_LEVEL_OFFSET],level);
	bn_short_set((bn_short *)&charinfo[CHARINFO_PORTRAIT_STATUS_OFFSET],pstatus);
	bn_int_set((bn_int *)&charinfo[CHARINFO_SUMMARY_STATUS_OFFSET],status);
	
	for (i=0;i<11;i++) {
	    bn_byte_set((bn_byte *)&charinfo[CHARINFO_PORTRAIT_GFX_OFFSET+i],bn_byte_get(&charsave[CHARSAVE_GFX_OFFSET+i]));
	    bn_byte_set((bn_byte *)&charinfo[CHARINFO_PORTRAIT_COLOR_OFFSET+i],bn_byte_get(&charsave[CHARSAVE_GFX_OFFSET+i]));
	}
	
	if (!(dbs_packet_savedata_charinfo(conn,AccountName,CharName,charinfo,CHARINFO_SIZE))) {
	    eventlog(eventlog_level_error,__FUNCTION__,"unable to save charinfo for %s",CharName);
	    return 0;
	}
	
	return 1; /* charinfo updated */
    }
    
    return 1; /* difficulty hack not enabled */
}
