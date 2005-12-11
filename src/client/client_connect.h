/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_CLIENT_CONNECT_PROTOS
#define INCLUDED_CLIENT_CONNECT_PROTOS

#define JUST_NEED_TYPES
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#undef JUST_NEED_TYPES


// Diablo 1.09b
#define CLIENT_VERSIONID_DRTL   0x0000002a
#define CLIENT_GAMEVERSION_DRTL 0x01000902
#define CLIENT_EXEINFO_DRTL     "Diablo.exe 05/18/01 23:10:57 767760"
#define CLIENT_CHECKSUM_DRTL    0xac337ef9

//plain Starcraft 1.11b
#define CLIENT_VERSIONID_STAR   0x000000c9
#define CLIENT_GAMEVERSION_STAR 0x01010103
#define CLIENT_EXEINFO_STAR     "starcraft.exe 05/26/04 00:46:00 1048576"
#define CLIENT_CHECKSUM_STAR    0xe6380bff


#define CLIENT_VERSIONID_SSHR   0x000000a5 /* FIXME: wrong? */
#define CLIENT_GAMEVERSION_SSHR 0x0100080a /* FIXME: wrong? */
#define CLIENT_EXEINFO_SSHR     "starcraft.exe 03/08/99 22:41:50 1042432" /* FIXME: wrong */
#define CLIENT_CHECKSUM_SSHR    0x12345678

//Broodwar 1.11b
#define CLIENT_VERSIONID_SEXP   0x000000c9
#define CLIENT_GAMEVERSION_SEXP 0x01010103
#define CLIENT_EXEINFO_SEXP     "starcraft.exe 05/26/04 00:46:00 1048576"
#define CLIENT_CHECKSUM_SEXP    0xe6380bff

//Warcraft 2 Battle.Net Edition 2.02b
#define CLIENT_VERSIONID_W2BN   0x0000004f
#define CLIENT_GAMEVERSION_W2BN 0x02000201
#define CLIENT_EXEINFO_W2BN     "Warcraft II BNE.exe 05/21/01 21:52:22 712704"
#define CLIENT_CHECKSUM_W2BN    0xf144a97e

//Diablo 2 1.10
#define CLIENT_VERSIONID_D2DV   0x0000000a
#define CLIENT_GAMEVERSION_D2DV 0x01000a00
#define CLIENT_EXEINFO_D2DV     "Game.exe 10/13/03 08:34:46 1194761"
#define CLIENT_CHECKSUM_D2DV    0x3132dda6

//Diable 2 Expansion 1.10
#define CLIENT_VERSIONID_D2XP   0x0000000a
#define CLIENT_GAMEVERSION_D2XP 0x01000a00
#define CLIENT_EXEINFO_D2XP     "Game.exe 10/13/03 08:35:30 1198857"
#define CLIENT_CHECKSUM_D2XP    0x210126ec

//Warcraft 3 1.16 and Warcraft 3 Frozen Throne 1.16
#define CLIENT_VERSIONID_WAR3   0x00000010
#define CLIENT_GAMEVERSION_WAR3 0x01001026
#define CLIENT_EXEINFO_WAR3     "war3.exe 06/28/04 23:37:02 1568211"
#define CLIENT_CHECKSUM_WAR3    0xcc882269

#define CLIENT_COUNTRYINFO_109_PROTOCOL            0x00000000
#define CLIENT_COUNTRYINFO_109_VERSIONID_D2DV      0x00000009
#define CLIENT_COUNTRYINFO_109_GAMELANG            "enUS"
#define CLIENT_COUNTRYINFO_109_LOCALIP             0x00000000
#define CLIENT_COUNTRYINFO_109_LANGID_USENGLISH    0x00000409
#define CLIENT_COUNTRYINFO_109_LANGSTR_USENGLISH   "ENU"
#define CLIENT_COUNTRYINFO_109_COUNTRYNAME_USA     "United States"

namespace pvpgn
{

namespace client
{

extern int client_connect(char const * progname, char const * servname, unsigned short servport, char const * cdowner, char const * cdkey, char const * clienttag, struct sockaddr_in * saddr, unsigned int * sessionkey, unsigned int * sessionnum, char const * archtag, char const * gamelang);

}

}
#endif
#endif
