/*
 * Copyright (C) 2003	Aaron
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
#define BINARY_LADDER_INTERNAL_ACCESS
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
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "errno.h"
#include "compat/strerror.h"
#include "account.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "ladder_binary.h"
#include "ladder.h"
#include "prefs.h"
#include "common/setup_after.h"

static void dispose_filename(const char * filename)
{
    if (filename) xfree((void*)filename);
}

static const char * binary_ladder_type_to_filename(t_binary_ladder_types type)
{
  switch (type) {
    case WAR3_SOLO: return "WAR3_SOLO";
    case WAR3_TEAM: return "WAR3_TEAM";
    case WAR3_FFA : return "WAR3_FFA";
    case WAR3_AT  : return "WAR3_AT";
    case W3XP_SOLO: return "W3XP_SOLO";
    case W3XP_TEAM: return "W3XP_TEAM";
    case W3XP_FFA : return "W3XP_FFA";
    case W3XP_AT  : return "W3XP_AT";
    case STAR_AR  : return "STAR_AR";
    case STAR_AW  : return "STAR_AW";
    case STAR_AG  : return "STAR_AG";
    case STAR_CR  : return "STAR_CR";
    case STAR_CW  : return "STAR_CW";
    case STAR_CG  : return "STAR_CG";
    case SEXP_AR  : return "SEXP_AR";
    case SEXP_AW  : return "SEXP_AW";
    case SEXP_AG  : return "SEXP_AG";
    case SEXP_CR  : return "SEXP_CR";
    case SEXP_CW  : return "SEXP_CW";
    case SEXP_CG  : return "SEXP_CG";
    case W2BN_AR  : return "W2BN_AR";
    case W2BN_AW  : return "W2BN_AW";
    case W2BN_AG  : return "W2BN_AG";
    case W2BN_CR  : return "W2BN_CR";
    case W2BN_CW  : return "W2BN_CW";
    case W2BN_CG  : return "W2BN_CG";
    case W2BN_ARI : return "W2BN_ARI";
    case W2BN_AWI : return "W2BN_AWI";
    case W2BN_AGI : return "W2BN_AGI";
    case W2BN_CRI : return "W2BN_CRI";
    case W2BN_CWI : return "W2BN_CWI";
    case W2BN_CGI : return "W2BN_CGI";

    default:
      eventlog(eventlog_level_error,__FUNCTION__,"got invalid binary ladder type");
      return NULL;
  }
}

extern int binary_ladder_save(t_binary_ladder_types type, unsigned int paracount, t_cb_get_from_ladder _cb_get_from_ladder)
{ int results[10];
  int rank = 1;
  const char * ladder_name;
  const char * filename;
  int checksum, count;
  FILE * fp;

  if ((!(ladder_name = binary_ladder_type_to_filename(type))) || 
      (!(filename = create_filename(prefs_get_ladderdir(),ladder_name,"_LADDER"))))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"NULL filename -  aborting");
    return -1;  
  }

  if (!(fp = fopen(filename,"wb")))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"could not open file \"%s\" for writing (fopen: %s)",filename,pstrerror(errno));
    dispose_filename(filename);
    return -1;
  }

  results[0] = magick;
  fwrite(results,sizeof(int),1,fp); //write the magick int as header

  checksum = 0;

  while ((*_cb_get_from_ladder)(type,rank,results)!=-1)
  {
    fwrite(results,sizeof(int),paracount,fp);
    for (count=0;count<paracount;count++) checksum+=results[count];
    rank++;
  }

  //calculate a checksum over saved data

  results[0] = checksum;
  fwrite(results,sizeof(int),1,fp); // add checksum at the end

  fclose(fp);
  dispose_filename(filename);
  return 0;
}

extern t_binary_ladder_load_result binary_ladder_load(t_binary_ladder_types type, unsigned int paracount, t_cb_add_to_ladder _cb_add_to_ladder)
{ int values[10];
  const char * ladder_name;
  const char * filename;
  int checksum, count;
  FILE * fp;

  //TODO: load from file and if this fails return binary_ladder_load_failed
  //      then make sure ladder gets loaded somehow else (form accounts)
  //      compare checksum - and if it differs return load_invalid 
  //      then make sure ladder gets flushed and then loaded from accounts
  //      on success don't load from accounts

  if ((!(ladder_name = binary_ladder_type_to_filename(type))) || 
      (!(filename = create_filename(prefs_get_ladderdir(),ladder_name,"_LADDER"))))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"NULL filename -  aborting");
    return load_failed;  
  }

  if (!(fp = fopen(filename,"rb")))
  {
    eventlog(eventlog_level_info,__FUNCTION__,"could not open ladder file \"%s\" - maybe ladder still empty",filename,pstrerror(errno));
    dispose_filename(filename);
    return load_failed;
  }

  if ((fread(values,sizeof(int),1,fp)!=1) ||  (values[0]!=magick))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"ladder file not starting with the magick int");
    dispose_filename(filename);
    fclose(fp);
    return load_failed;
  }

  checksum = 0;

  while (fread(values,sizeof(int),paracount,fp)==paracount)
  {
    (*_cb_add_to_ladder)(type,values);
    for (count=0;count<paracount;count++) checksum+=values[count];
  }

  fread(values,sizeof(int),1,fp); 
  if (feof(fp)==0)
  {
    eventlog(eventlog_level_error,__FUNCTION__,"got data past end.. fall back to old loading mode");
    return illegal_checksum;

  }
  if (values[0]!=checksum)
  {
    eventlog(eventlog_level_error,__FUNCTION__,"ladder file has invalid checksum... fall back to old loading mode");
    return illegal_checksum;
  }

  fclose(fp);
  eventlog(eventlog_level_info,__FUNCTION__,"successfully loaded %s",filename);
  dispose_filename(filename);
  return load_success;

}
