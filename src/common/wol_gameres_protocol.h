/*
 * Copyright (C) 2008  Pelish (pelish@gmail.com)
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
#ifndef INCLUDED_WOL_GAMERES_PROTOCOL_TYPES
#define INCLUDED_WOL_GAMERES_PROTOCOL_TYPES

#ifdef JUST_NEED_TYPES
# include "common/bn_type.h"
# include "common/tag.h"
#else
# define JUST_NEED_TYPES
# include "common/bn_type.h"
# include "common/tag.h"
# undef JUST_NEED_TYPES
#endif

/*
 * The "wol gamreres" protocol is used for sending Game reports to server
 * for ladders support.
 */


namespace pvpgn
{

/******************************************************/
typedef struct
{
    bn_short size;
    bn_short unknown1;
} PACKED_ATTR() t_wolgameres_header;
/******************************************************/

/******************************************************/
typedef struct
{
    t_wolgameres_header h;
} PACKED_ATTR() t_wolgameres_generic;
/******************************************************/

/* PELISH: not used now...
typedef struct
{
	bn_short type;
	bn_short length;
//	data;
} PACKED_ATTR() t_client_wolgameres_generic;
*/
/* Game tags */

const t_tag CLIENT_SERN_UINT = 0x53455223;
const t_tag CLIENT_IDNO_UINT = 0x49444E4F;
const t_tag CLIENT_GSKU_UINT = 0x47534B55;
const t_tag CLIENT_TRNY_UINT = 0x54524E59;
const t_tag CLIENT_OOSY_UINT = 0x4F4F5359;
const t_tag CLIENT_FINI_UINT = 0x46494E49;
const t_tag CLIENT_DURA_UINT = 0x44555241;
const t_tag CLIENT_CRED_UINT = 0x43524544;
const t_tag CLIENT_SHRT_UINT = 0x53485254;
const t_tag CLIENT_SUPR_UINT = 0x53555052;
const t_tag CLIENT_MODE_UINT = 0x4D4F4445;
const t_tag CLIENT_BAMR_UINT = 0x42414D52;
const t_tag CLIENT_CRAT_UINT = 0x43524154;
const t_tag CLIENT_AIPL_UINT = 0x4149504C;
const t_tag CLIENT_UNIT_UINT = 0x554E4954;
const t_tag CLIENT_SCEN_UINT = 0x5343454E;
const t_tag CLIENT_PNGS_UINT = 0x504E4753;
const t_tag CLIENT_PNGR_UINT = 0x504E4752;
const t_tag CLIENT_PLRS_UINT = 0x504C5253;
const t_tag CLIENT_SPID_UINT = 0x53504944;
const t_tag CLIENT_TIME_UINT = 0x54494D45;
const t_tag CLIENT_AFPS_UINT = 0x41465053;
const t_tag CLIENT_PROC_UINT = 0x50524F43;
const t_tag CLIENT_MEMO_UINT = 0x4D454D4F;
const t_tag CLIENT_VIDM_UINT = 0x5649444D;
const t_tag CLIENT_SPED_UINT = 0x53504544;
const t_tag CLIENT_VERS_UINT = 0x56455253;
const t_tag CLIENT_DATE_UINT = 0x44415445;
const t_tag CLIENT_BASE_UINT = 0x42415345;
const t_tag CLIENT_TIBR_UINT = 0x54494252;
const t_tag CLIENT_SHAD_UINT = 0x53484144;
const t_tag CLIENT_FLAG_UINT = 0x464C4147;
const t_tag CLIENT_TECH_UINT = 0x54454348;

/* Player tags */
const t_tag CLIENT_NAM0_UINT = 0x4E414D30;
const t_tag CLIENT_NAM1_UINT = 0x4E414D31;
const t_tag CLIENT_NAM2_UINT = 0x4E414D32;
const t_tag CLIENT_NAM3_UINT = 0x4E414D33;
const t_tag CLIENT_NAM4_UINT = 0x4E414D34;
const t_tag CLIENT_NAM5_UINT = 0x4E414D35;
const t_tag CLIENT_NAM6_UINT = 0x4E414D36;
const t_tag CLIENT_NAM7_UINT = 0x4E414D37;

const t_tag CLIENT_CMP0_UINT = 0x434D5030;
const t_tag CLIENT_CMP1_UINT = 0x434D5031;
const t_tag CLIENT_CMP2_UINT = 0x434D5032;
const t_tag CLIENT_CMP3_UINT = 0x434D5033;
const t_tag CLIENT_CMP4_UINT = 0x434D5034;
const t_tag CLIENT_CMP5_UINT = 0x434D5035;
const t_tag CLIENT_CMP6_UINT = 0x434D5036;
const t_tag CLIENT_CMP7_UINT = 0x434D5037;

}

#endif  /* INCLUDED_WOL_GAMERES_PROTOCOL_TYPES */

