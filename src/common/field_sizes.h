/*
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
#ifndef INCLUDED_FIELD_SIZES_TYPES
#define INCLUDED_FIELD_SIZES_TYPES

#define MAX_PACKET_SIZE      3072
#define MAX_NORMAL_TYPE    0xffff
#define MAX_FILE_TYPE      0xffff
#define MAX_AUTH_TYPE        0xff
#define MAX_GAMES_LIST         20
#define MAX_MESSAGE_LEN       255
#define MAX_COMMAND_LEN        32
#define USER_NAME_MAX          16 /* including terminating NUL char */
#define USER_NAME_MIN           2
#define USER_PASS_MAX          12 /* max password length as allowed by clients */
#define UNCHECKED_NAME_STR     32 /* including terminating NUL char */
                                  /* (longer than USER_NAME_MAX and CHAR_NAME_LEN so that
                                   *  proper error packets can be returned) */
#define MAX_CLANNAME_LEN       64
#define CHANNEL_NAME_LEN       64
#define CHAR_NAME_LEN          16 /* including terminating NUL char */
#define REALM_NAME_LEN         32
#define GAME_NAME_LEN          32
#define GAME_PASS_LEN          32
#define GAME_INFO_LEN         256
#define MAP_NAME_LEN           64
#define BNETHASH_LEN           20 /* uint32*5, see bnethash.h */
#define MAX_EXEINFO_STR       256 /* including terminating NUL char */
#define MAX_OWNER_STR         128 /* including terminating NUL char */
#define MAX_CDKEY_STR         128 /* including terminating NUL char */
#define MAX_EMAIL_STR         128 /* including terminating NUL char */
#define MAX_WINHOST_STR       128 /* including terminating NUL char */
#define MAX_WINUSER_STR       128 /* including terminating NUL char */
#define MAX_LANG_STR           64 /* including terminating NUL char */
#define MAX_COUNTRYNAME_STR   128 /* including terminating NUL char */
#define MAX_FILENAME_STR     2048 /* including terminating NUL char */
#define MAX_GAMEREP_HEAD_STR 2048 /* including terminating NUL char */
#define MAX_GAMEREP_BODY_STR 8192 /* including terminating NUL char */
#define MAX_PLAYERINFO_STR   2048 /* including terminating NUL char */
#define MAX_COUNTRYCODE_STR    32 /* including terminating NUL char */
#define MAX_COUNTRY_STR        32 /* including terminating NUL char */
#define MAX_ATTRKEY_STR      1024 /* including terminating NUL char */
#define MAX_ATTRVAL_STR      4096 /* including terminating NUL char */
#define MAX_IRC_MESSAGE_LEN 512 /* including CRLF (according to RFC 2812) */
#define MAX_TOPIC_LEN         201 /* including terminating NUL char */
#define CLANSHORT_NAME_MAX          4
#define CLAN_NAME_MAX          25 /* including terminating NUL char */

#endif
