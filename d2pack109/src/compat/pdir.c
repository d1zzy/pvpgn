/*
 * Copyright (C) 2001  Dizzy (dizzy@roedu.net)
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
#define PDIR_INTERNAL_ACCESS
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
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strdup.h"
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
# define dirent direct
#endif
#ifdef WIN32
# include <io.h> /* for _findfirst(), _findnext(), etc */
#endif
#include <errno.h>
#include "compat/strerror.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "pdir.h"
#include "common/setup_after.h"


extern t_pdir * p_opendir(const char * path) {
#ifdef WIN32
   char npath[_MAX_PATH];
#endif
   t_pdir * pdir;
   
   if (path==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL path");
      return NULL;
   }
/*   while(path[strlen(path)]=='/') path[strlen(path)]='\0'; */
   /* win32 can use slash in addition to backslash  */
   pdir=xmalloc(sizeof(t_pdir));

#ifdef WIN32
   if (strlen(path)+1+3+1>_MAX_PATH) {
      eventlog(eventlog_level_error,__FUNCTION__,"WIN32: path too long");
      xfree(pdir);
      return NULL;
   }
   strcpy(npath, path);
   strcat(npath, "/*.*");
   pdir->path=xstrdup(npath);

   pdir->status = 0;
   memset(&pdir->fileinfo, 0, sizeof(pdir->fileinfo)); /* no need for compat because WIN32 always has memset() */
   pdir->lFindHandle = _findfirst(npath, &pdir->fileinfo);
   if (pdir->lFindHandle < 0) {
      eventlog(eventlog_level_error,__FUNCTION__,"WIN32: unable to open directory \"%s\" for reading (_findfirst: %s)",npath,strerror(errno));
      xfree((void *)pdir->path); /* avoid warning */
      xfree(pdir);
      return NULL;
   }

#else /* POSIX style */

   pdir->path=xstrdup(path);
   if ((pdir->dir=opendir(path))==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"POSIX: unable to open directory \"%s\" for reading (opendir: %s)",path,strerror(errno));
      xfree((void *)pdir->path); /* avoid warning */
      xfree(pdir);
      return NULL;
   }
   
#endif /* WIN32-POSIX */
   
   eventlog(eventlog_level_debug,__FUNCTION__,"successfully opened dir: %s",path);
   return pdir;
}


extern int p_rewinddir(t_pdir * pdir) {
   if (pdir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL pdir");
      return -1;
   }
   if (pdir->path==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got pdir with NULL path");
      return -1;
   }
#ifdef WIN32
   /* i dont have any win32 around so i dont know if io.h has any rewinddir equivalent */
   /* FIXME: for the time being ill just close and reopen it */
   if (pdir->status!=-1)
   {
       if (pdir->lFindHandle<0) {
	   eventlog(eventlog_level_error,__FUNCTION__,"WIN32: got negative lFindHandle");
	   return -1;
       }
       _findclose(pdir->lFindHandle);
   }
   pdir->status = 0;
   memset(&pdir->fileinfo, 0, sizeof(pdir->fileinfo)); /* no need for compat because WIN32 always has memset() */
   pdir->lFindHandle = _findfirst(pdir->path, &pdir->fileinfo);
   if (pdir->lFindHandle < 0) {
      eventlog(eventlog_level_error,__FUNCTION__,"WIN32: unable to open directory \"%s\" for reading (_findfirst: %s)",pdir->path,strerror(errno));
      pdir->status = -1;
      return -1;
   }
#else /* POSIX */
   if (pdir->dir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"POSIX: got pdir with NULL dir");
      return -1;
   }
   rewinddir(pdir->dir);
#endif
   return 0;
}


extern char const * p_readdir(t_pdir * pdir) {
   if (pdir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL pdir");
      return NULL;
   }
   if (pdir->path==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got pdir with NULL path");
      return NULL;
   }
#ifdef WIN32
   switch (pdir->status)
   {
   default:
   case -1: /* couldn't rewind */
       eventlog(eventlog_level_error,__FUNCTION__,"got pdir with status -1");
       return NULL;
   case 0: /* freshly opened */
       pdir->status = 1;
       return pdir->fileinfo.name;
   case 1: /* reading */
       if (_findnext(pdir->lFindHandle, &pdir->fileinfo)<0)
       {
	   pdir->status = 2;
	   return NULL;
       }
       else
	   return pdir->fileinfo.name;
       break;
   case 2: /* EOF */
       return NULL;
   }
#else /* POSIX */
   {
	struct dirent * dentry;
	
	if (pdir->dir==NULL) {
	    eventlog(eventlog_level_error,__FUNCTION__,"POSIX: got pdir with NULL dir");
	    return NULL;
	}
	if ((dentry=readdir(pdir->dir))==NULL)
	    return NULL;
	return dentry->d_name;
   }
#endif /* WIN32-POSIX */
}


extern int p_closedir(t_pdir * pdir) {
   int ret;
   
   if (pdir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL pdir");
      return -1;
   }
   if (pdir->path==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got pdir with NULL path");
      return -1;
   }
   
#ifdef WIN32
   if (pdir->status!=-1)
   {
       if (pdir->lFindHandle<0) {
	   eventlog(eventlog_level_info,__FUNCTION__,"WIN32: got NULL findhandle");
	   return -1;
       }
       _findclose(pdir->lFindHandle); /* FIXME: what does _findclose() return on error? */
   }
   ret = 0;
#else /* POSIX */
   if (pdir->dir==NULL) {
      eventlog(eventlog_level_info,__FUNCTION__,"POSIX: got NULL dir");
      return -1;
   }
# ifdef CLOSEDIR_VOID
    closedir(pdir->dir);
    ret = 0;
# else
    ret = closedir(pdir->dir);
# endif
#endif /* WIN32-POSIX */
   
   xfree((void *)pdir->path); /* avoid warning */
   xfree(pdir);
   return ret;
}
