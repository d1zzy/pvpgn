/*
 * Copyright (C) 2004	Aaron
 * Copyright (C) 2004	CreepLord (creeplord@pvpgn.org)
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
#include <ctype.h>
#include "errno.h"
#include "compat/strerror.h"
#include "common/eventlog.h"
#include "common/tag.h"
#include "common/setup_after.h"

/* fixme: have all functions call tag_str_to_uint() */
extern t_clienttag clienttag_str_to_uint(char const * clienttag)
{
	if (!clienttag)
	{
		eventlog(eventlog_level_error,__FUNCTION__,"got NULL clienttag");
		return CLIENTTAG_UNKNOWN_UINT;
	}

	return tag_str_to_uint(clienttag);
}

/* fixme: have all fuctions call tag_uint_to_str() */
extern char const * clienttag_uint_to_str(t_clienttag clienttag)
{
	switch (clienttag)
	{
	    case CLIENTTAG_BNCHATBOT_UINT:
		return CLIENTTAG_BNCHATBOT;
	    case CLIENTTAG_STARCRAFT_UINT:
		return CLIENTTAG_STARCRAFT;
	    case CLIENTTAG_BROODWARS_UINT:
		return CLIENTTAG_BROODWARS;
	    case CLIENTTAG_SHAREWARE_UINT:
		return CLIENTTAG_SHAREWARE;
	    case CLIENTTAG_DIABLORTL_UINT:
		return CLIENTTAG_DIABLORTL;
	    case CLIENTTAG_DIABLOSHR_UINT:
	    	return CLIENTTAG_DIABLOSHR;
	    case CLIENTTAG_WARCIIBNE_UINT:
	    	return CLIENTTAG_WARCIIBNE;
	    case CLIENTTAG_DIABLO2DV_UINT:
	    	return CLIENTTAG_DIABLO2DV;
	    case CLIENTTAG_STARJAPAN_UINT:
	    	return CLIENTTAG_STARJAPAN;
	    case CLIENTTAG_DIABLO2ST_UINT:
	    	return CLIENTTAG_DIABLO2ST;
	    case CLIENTTAG_DIABLO2XP_UINT:
	    	return CLIENTTAG_DIABLO2XP;
	    case CLIENTTAG_WARCRAFT3_UINT:
	    	return CLIENTTAG_WARCRAFT3;
	    case CLIENTTAG_WAR3XP_UINT:
	    	return CLIENTTAG_WAR3XP;
	    default:
		return CLIENTTAG_UNKNOWN;
	}
}

/*****/
/* make all letters in string upper case - used in command.c*/
extern t_tag tag_case_str_to_uint(char const * tag_str)
{
    unsigned int i, len;
    char temp_str[5];
    
    len = strlen(tag_str);
    if (len != 4) 
	eventlog(eventlog_level_warn,__FUNCTION__,"got unusual sized clienttag '%s'",tag_str);

    for (i=0; i<len && i < 4; i++)
	if (islower((int)tag_str[i]))
	    temp_str[i] = toupper((int)tag_str[i]);
	else
	    temp_str[i] = tag_str[i];
	    
    temp_str[4] = '\0';
    
    return tag_str_to_uint(temp_str);
}

extern t_tag tag_str_to_uint(char const * tag_str)
{
    t_tag	tag_uint;
    
    if (!tag_str) {
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL tag");
	return 0; /* unknown */
    }
    
    tag_uint  = tag_str[0]<<24;
    tag_uint |= tag_str[1]<<16;
    tag_uint |= tag_str[2]<< 8;
    tag_uint |= tag_str[3]    ;
    
    return tag_uint;
}
/* tag_uint_to_str()
 *
 * from calling function:
 *
 *    char tag_str[5]; // define first, then send into fuction
 *    tag_uint_to_str(tag_str, tag_uint); // returns pointer to tag_str
 *
 * Nothing to malloc, nothing to free
 */
extern char * tag_uint_to_str(char * tag_str, t_tag tag_uint)
{
    if (!tag_uint) /* return "UNKN" if tag_uint = 0 */
	return TAG_UNKNOWN;
    
    tag_str[0] = ((unsigned char)(tag_uint>>24)     );
    tag_str[1] = ((unsigned char)(tag_uint>>16)&0xff);
    tag_str[2] = ((unsigned char)(tag_uint>> 8)&0xff);
    tag_str[3] = ((unsigned char)(tag_uint    )&0xff);
    tag_str[4] = '\0';
    return tag_str;
}

extern int tag_check_arch(t_tag tag_uint)
{
    switch (tag_uint)
    {
	case ARCHTAG_WINX86_UINT:
	case ARCHTAG_MACPPC_UINT:
	case ARCHTAG_OSXPPC_UINT:
	    return 1;
	default:
	    return 0;
    }
}

extern int tag_check_client(t_tag tag_uint)
{
    switch (tag_uint)
    {
	case CLIENTTAG_BNCHATBOT_UINT:
	case CLIENTTAG_STARCRAFT_UINT:
	case CLIENTTAG_BROODWARS_UINT:
	case CLIENTTAG_SHAREWARE_UINT:
	case CLIENTTAG_DIABLORTL_UINT:
	case CLIENTTAG_DIABLOSHR_UINT:
	case CLIENTTAG_WARCIIBNE_UINT:
	case CLIENTTAG_DIABLO2DV_UINT:
	case CLIENTTAG_STARJAPAN_UINT:
	case CLIENTTAG_DIABLO2ST_UINT:
	case CLIENTTAG_DIABLO2XP_UINT:
	case CLIENTTAG_WARCRAFT3_UINT:
	case CLIENTTAG_WAR3XP_UINT:
	    return 1;
	default:
	    return 0;
    }
}

extern int tag_check_gamelang(t_tag gamelang)
{
    switch (gamelang)
    {
	case GAMELANG_ENGLISH_UINT:	/* enUS */
	case GAMELANG_GERMAN_UINT:	/* deDE */
	case GAMELANG_CZECH_UINT:	/* csCZ */
	case GAMELANG_SPANISH_UINT:	/* esES */
	case GAMELANG_FRENCH_UINT:	/* frFR */
	case GAMELANG_ITALIAN_UINT:	/* itIT */
	case GAMELANG_JAPANESE_UINT:	/* jaJA */
	case GAMELANG_KOREAN_UINT:	/* koKR */
	case GAMELANG_POLISH_UINT:	/* plPL */
	case GAMELANG_RUSSIAN_UINT:	/* ruRU */
	case GAMELANG_CHINESE_S_UINT:	/* zhCN */
	case GAMELANG_CHINESE_T_UINT:	/* zhTW */
	    return 1;
	default:
	    return 0;
    }
}

extern char const * clienttag_get_title(t_clienttag clienttag)
{
   switch (clienttag)
   {
      case CLIENTTAG_WAR3XP_UINT:
        return "Warcraft III Frozen Throne";
      case CLIENTTAG_WARCRAFT3_UINT:
        return "Warcraft III";
      case CLIENTTAG_DIABLO2XP_UINT:
        return "Diablo II Lord of Destruction";
      case CLIENTTAG_DIABLO2DV_UINT:
        return "Diablo II";
      case CLIENTTAG_STARJAPAN_UINT:
        return "Starcraft (Japan)";
      case CLIENTTAG_WARCIIBNE_UINT:
        return "Warcraft II";
      case CLIENTTAG_DIABLOSHR_UINT:
        return "Diablo I (Shareware)";
      case CLIENTTAG_DIABLORTL_UINT:
        return "Diablo I";
      case CLIENTTAG_SHAREWARE_UINT:
        return "Starcraft (Shareware)";
      case CLIENTTAG_BROODWARS_UINT:
        return "Starcraft: BroodWars";
      case CLIENTTAG_STARCRAFT_UINT:
        return "Starcraft";
      case CLIENTTAG_BNCHATBOT_UINT:
        return "Chat";
      default:
        return "Unknown";
   }
}

