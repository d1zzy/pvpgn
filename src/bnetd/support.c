/*
 * Copyright (C) 2004 Lector
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
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "compat/access.h"
#include "compat/strerror.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "support.h"
#include "prefs.h"
#include "common/util.h"
#include "common/setup_after.h"

extern int support_check_files(char const * supportfile)
{

  FILE *fp;
  char *buff;
  unsigned int line;
  int filedirlen;
  char * namebuff;

  if (!(supportfile))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"got NULL supportfile");
    return -1;
  }

  if (!(fp = fopen(supportfile,"r")))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"could not open file \"%s\" for reading (fopen: %s)",supportfile,pstrerror(errno));
    eventlog(eventlog_level_error,__FUNCTION__,"can't guarantee that everything will run smooth");
    return 0;
  }

  filedirlen = strlen(prefs_get_filedir());

  for (line=1; (buff = file_get_line(fp)); line++)
  {
    if (buff[0]=='#' || buff[0]=='\0')
    {
      continue;
    }
    
    namebuff = (char*)xmalloc(filedirlen + 1 + strlen(buff) + 1);
    sprintf(namebuff,"%s/%s",prefs_get_filedir(),buff);

    if (access(namebuff, F_OK) < 0)
    {
      eventlog(eventlog_level_fatal,__FUNCTION__,"necessary file \"%s\" missing",namebuff);
      xfree((void *)namebuff);
      fclose(fp);
      return -1;
    }

    xfree((void *)namebuff);
  }

  file_get_line(NULL); // clear file_get_line buffer
  fclose(fp);

  return 0;
}
