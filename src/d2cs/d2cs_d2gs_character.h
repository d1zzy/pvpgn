/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
 * Copyright (C) 2001		faster	(lqx@cic.tsinghua.edu.cn)
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
#ifndef INCLUDED_D2CS_D2GS_CHARACTER_H
#define INCLUDED_D2CS_D2GS_CHARACTER_H

#ifdef D2CS
# include "common/bn_type.h"
#endif

#ifdef D2GS
# include "bn_types.h"
#pragma pack(push, character_h, 1)
#endif 

#define D2CHARINFO_MAGICWORD			0x12345678
#define D2CHARINFO_VERSION			0x00010000
#define D2CHARINFO_PORTRAIT_PADSIZE		30

typedef struct
{
	bn_int		magicword;	/* static for check */
	bn_int		version;	/* charinfo file version */
	bn_int		create_time;	/* character creation time */
	bn_int		last_time;	/* character last access time */
	bn_int		checksum;
	bn_int		reserved[7];
	unsigned char	charname[MAX_CHARNAME_LEN];
	unsigned char	account[MAX_ACCTNAME_LEN];
	unsigned char	realmname[MAX_REALMNAME_LEN];
} t_d2charinfo_header;

typedef struct
{
	bn_int		experience;
	bn_int		charstatus;
	bn_int		charlevel;
	bn_int		charclass;
} t_d2charinfo_summary;

typedef struct
{
        bn_short        header;	/* 0x84 0x80 */
        bn_byte         gfx[11];
        bn_byte         class;
        bn_byte         color[11];
        bn_byte         level;
        bn_byte         status;
        bn_byte         u1[3];
        bn_byte         u2[3];
        bn_byte         end;	/* 0x00 */
} t_d2charinfo_portrait;

typedef struct
{
	t_d2charinfo_header		header;
	t_d2charinfo_portrait		portrait;
        bn_byte         		pad[D2CHARINFO_PORTRAIT_PADSIZE];
	t_d2charinfo_summary		summary;
} t_d2charinfo_file;

#ifdef D2GS
#pragma pack(pop, character_h)
#endif

#endif
