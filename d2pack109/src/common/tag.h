/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef JUST_NEED_TYPES
/* FIXME: could probably be in PROTOS... does any other header need these? */
#ifndef INCLUDED_TAG_TYPES
#define INCLUDED_TAG_TYPES

/* Software tags */
#define CLIENTTAG_BNCHATBOT "CHAT" /* CHAT bot */
#define CLIENTTAG_STARCRAFT "STAR" /* Starcraft (original) */
#define CLIENTTAG_BROODWARS "SEXP" /* Starcraft EXpansion Pack */
#define CLIENTTAG_SHAREWARE "SSHR" /* Starcraft Shareware */
#define CLIENTTAG_DIABLORTL "DRTL" /* Diablo ReTaiL */
#define CLIENTTAG_DIABLOSHR "DSHR" /* Diablo SHaReware */
#define CLIENTTAG_WARCIIBNE "W2BN" /* WarCraft II Battle.net Edition */
#define CLIENTTAG_DIABLO2DV "D2DV" /* Diablo II Diablo's Victory */
#define CLIENTTAG_STARJAPAN "JSTR" /* Starcraft (Japan) */
#define CLIENTTAG_DIABLO2ST "D2ST" /* Diablo II Stress Test */
#define CLIENTTAG_DIABLO2XP "D2XP" /* Diablo II Extension Pack */
/* FIXME: according to FSGS:
    SJPN==Starcraft (Japanese)
    SSJP==Starcraft (Japanese,Spawn)
*/
#define CLIENTTAG_WARCRAFT3 "WAR3" /* WarCraft III */
#define CLIENTTAG_WAR3XP    "W3XP" /* WarCraft III Expansion */

/* BNETD-specific software tags - we try to use lowercase to avoid collisions  */
#define CLIENTTAG_FREECRAFT "free" /* FreeCraft http://www.freecraft.com/ */

/* Architecture tags */
#define ARCHTAG_WINX86       "IX86" /* MS Windows on Intel x86 */
#define ARCHTAG_MACPPC       "PMAC" /* MacOS   on PowerPC   */
#define ARCHTAG_OSXPPC       "XMAC" /* MacOS X on PowerPC   */

/* Server tag */
#define BNETTAG "bnet" /* Battle.net */

/* Filetype tags (note these are "backwards") */
#define EXTENSIONTAG_PCX "xcp."
#define EXTENSIONTAG_SMK "kms."
#define EXTENSIONTAG_MNG "gnm."

#endif
#endif
