/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_TAG_TYPES
#define INCLUDED_TAG_TYPES

#ifdef JUST_NEED_TYPES
#include "compat/uint.h"
#else
#define JUST_NEED_TYPES
#include "compat/uint.h"
#undef JUST_NEED_TYPES
#endif

typedef t_uint32	t_tag;
typedef t_tag		t_archtag;
typedef t_tag		t_clienttag;
typedef t_tag		t_gamelang;

#endif

/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_TAG_PROTOS
#define INCLUDED_TAG_PROTOS

/* Software tags */
#define CLIENTTAG_BNCHATBOT 		"CHAT" /* CHAT bot */
#define CLIENTTAG_BNCHATBOT_UINT 	0x43484154
#define CLIENTTAG_STARCRAFT 		"STAR" /* Starcraft (original) */
#define CLIENTTAG_STARCRAFT_UINT 	0x53544152
#define CLIENTTAG_BROODWARS 		"SEXP" /* Starcraft EXpansion Pack */
#define CLIENTTAG_BROODWARS_UINT 	0x53455850
#define CLIENTTAG_SHAREWARE 		"SSHR" /* Starcraft Shareware */
#define CLIENTTAG_SHAREWARE_UINT	0x53534852
#define CLIENTTAG_DIABLORTL 		"DRTL" /* Diablo ReTaiL */
#define CLIENTTAG_DIABLORTL_UINT	0x4452544C
#define CLIENTTAG_DIABLOSHR 		"DSHR" /* Diablo SHaReware */
#define CLIENTTAG_DIABLOSHR_UINT	0x44534852
#define CLIENTTAG_WARCIIBNE 		"W2BN" /* WarCraft II Battle.net Edition */
#define CLIENTTAG_WARCIIBNE_UINT	0x5732424E
#define CLIENTTAG_DIABLO2DV 		"D2DV" /* Diablo II Diablo's Victory */
#define CLIENTTAG_DIABLO2DV_UINT	0x44324456
#define CLIENTTAG_STARJAPAN 		"JSTR" /* Starcraft (Japan) */
#define CLIENTTAG_STARJAPAN_UINT	0x4A535452
#define CLIENTTAG_DIABLO2ST 		"D2ST" /* Diablo II Stress Test */
#define CLIENTTAG_DIABLO2ST_UINT	0x44325354
#define CLIENTTAG_DIABLO2XP 		"D2XP" /* Diablo II Extension Pack */
#define CLIENTTAG_DIABLO2XP_UINT	0x44325850
/* FIXME: according to FSGS:
    SJPN==Starcraft (Japanese)
    SSJP==Starcraft (Japanese,Spawn)
*/
#define CLIENTTAG_WARCRAFT3 		"WAR3" /* WarCraft III */
#define CLIENTTAG_WARCRAFT3_UINT	0x57415233
#define CLIENTTAG_WAR3XP    		"W3XP" /* WarCraft III Expansion */
#define CLIENTTAG_WAR3XP_UINT		0x57335850

/* BNETD-specific software tags - we try to use lowercase to avoid collisions  */
#define CLIENTTAG_FREECRAFT "free" /* FreeCraft http://www.freecraft.com/ */

#define CLIENTTAG_UNKNOWN		"UNKN"
#define CLIENTTAG_UNKNOWN_UINT		0x554E4B4E

/* Architecture tags */
#define ARCHTAG_WINX86       "IX86" /* MS Windows on Intel x86 */
#define ARCHTAG_MACPPC       "PMAC" /* MacOS   on PowerPC   */
#define ARCHTAG_OSXPPC       "XMAC" /* MacOS X on PowerPC   */

#define ARCHTAG_WINX86_UINT  0x49583836		/* IX86 */
#define ARCHTAG_MACPPC_UINT  0x504D4143		/* PMAC */
#define ARCHTAG_OSXPPC_UINT  0x584D4143		/* XMAC */

/* game languages */
#define GAMELANG_ENGLISH_UINT	0x656E5553	/* enUS */
#define GAMELANG_GERMAN_UINT	0x64654445	/* deDE */
#define GAMELANG_CZECH_UINT	0x6373435A	/* csCZ */
#define GAMELANG_SPANISH_UINT	0x65734553	/* esES */
#define GAMELANG_FRENCH_UINT	0x66724652	/* frFR */
#define GAMELANG_ITALIAN_UINT	0x69744954	/* itIT */
#define GAMELANG_JAPANESE_UINT	0x6A614A41	/* jaJA */
#define GAMELANG_KOREAN_UINT	0x6B6F4B52	/* koKR */
#define GAMELANG_POLISH_UINT	0x706C504C	/* plPL */
#define GAMELANG_RUSSIAN_UINT	0x72755255	/* ruRU */
#define GAMELANG_CHINESE_S_UINT	0x7A68434E	/* zhCN */
#define GAMELANG_CHINESE_T_UINT	0x7A685457	/* zhTW */

#define TAG_UNKNOWN_UINT	0x554E4B4E	/* UNKN */
#define TAG_UNKNOWN		"UNKN"

/* Server tag */
#define BNETTAG "bnet" /* Battle.net */

/* Filetype tags (note these are "backwards") */
#define EXTENSIONTAG_PCX "xcp."
#define EXTENSIONTAG_SMK "kms."
#define EXTENSIONTAG_MNG "gnm."

extern t_clienttag clienttag_str_to_uint(char const * clienttag);
extern char const * clienttag_uint_to_str(t_clienttag clienttag);
extern char const * clienttag_get_title(t_clienttag clienttag);

extern t_tag	tag_str_to_uint(char const * tag_str);
extern t_tag	tag_case_str_to_uint(char const * tag_str);
extern char *	tag_uint_to_str(char * tag_str, t_tag tag_uint);
extern int	tag_check_arch(t_tag tag_uint);
extern int	tag_check_client(t_tag tag_uint);
extern int	tag_check_gamelang(t_tag tag_uint);

#endif
#endif
