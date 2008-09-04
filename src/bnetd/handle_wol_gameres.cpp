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

#include "common/setup_before.h"
#include "handle_wol_gameres.h"

#include <cstring>
#include <cctype>
#include <cstdlib>
//#include <sstream>
//#include <cstring>
//nclude <cctype>
//nclude <ctime>

#include "common/wol_gameres_protocol.h"
#include "common/packet.h"
#include "common/eventlog.h"
#include "common/tag.h"
#include "common/bn_type.h"

#include "connection.h"
//#include "prefs.h"
//#include "server.h"
#ifdef WIN32_GUI
#include <win32/winmain.h>
#endif
#include "common/setup_after.h"

namespace pvpgn
{

namespace bnetd
{

/* handlers prototypes */
static int _client_sern(wol_gameres_type type, int size, void const * data);
static int _client_idno(wol_gameres_type type, int size, void const * data);
static int _client_gsku(wol_gameres_type type, int size, void const * data);
static int _client_trny(wol_gameres_type type, int size, void const * data);
static int _client_oosy(wol_gameres_type type, int size, void const * data);
static int _client_fini(wol_gameres_type type, int size, void const * data);
static int _client_dura(wol_gameres_type type, int size, void const * data);
static int _client_cred(wol_gameres_type type, int size, void const * data);
static int _client_shrt(wol_gameres_type type, int size, void const * data);
static int _client_supr(wol_gameres_type type, int size, void const * data);
static int _client_mode(wol_gameres_type type, int size, void const * data);
static int _client_bamr(wol_gameres_type type, int size, void const * data);
static int _client_crat(wol_gameres_type type, int size, void const * data);
static int _client_aipl(wol_gameres_type type, int size, void const * data);
static int _client_unit(wol_gameres_type type, int size, void const * data);
static int _client_scen(wol_gameres_type type, int size, void const * data);
static int _client_pngs(wol_gameres_type type, int size, void const * data);
static int _client_pngr(wol_gameres_type type, int size, void const * data);
static int _client_plrs(wol_gameres_type type, int size, void const * data);
static int _client_spid(wol_gameres_type type, int size, void const * data);
static int _client_time(wol_gameres_type type, int size, void const * data);
static int _client_afps(wol_gameres_type type, int size, void const * data);
static int _client_proc(wol_gameres_type type, int size, void const * data);
static int _client_memo(wol_gameres_type type, int size, void const * data);
static int _client_vidm(wol_gameres_type type, int size, void const * data);
static int _client_sped(wol_gameres_type type, int size, void const * data);
static int _client_vers(wol_gameres_type type, int size, void const * data);
static int _client_date(wol_gameres_type type, int size, void const * data);
static int _client_base(wol_gameres_type type, int size, void const * data);
static int _client_tibr(wol_gameres_type type, int size, void const * data);
static int _client_shad(wol_gameres_type type, int size, void const * data);
static int _client_flag(wol_gameres_type type, int size, void const * data);
static int _client_tech(wol_gameres_type type, int size, void const * data);
static int _client_acco(wol_gameres_type type, int size, void const * data);
static int _client_brok(wol_gameres_type type, int size, void const * data);
static int _client_etim(wol_gameres_type type, int size, void const * data);
static int _client_pspd(wol_gameres_type type, int size, void const * data);
static int _client_smem(wol_gameres_type type, int size, void const * data);
static int _client_svid(wol_gameres_type type, int size, void const * data);
static int _client_snam(wol_gameres_type type, int size, void const * data);
static int _client_gmap(wol_gameres_type type, int size, void const * data);
static int _client_dsvr(wol_gameres_type type, int size, void const * data);

/* Renegade Player related functions */
static int _client_pnam(wol_gameres_type type, int size, void const * data);
static int _client_ploc(wol_gameres_type type, int size, void const * data);
static int _client_team(wol_gameres_type type, int size, void const * data);
static int _client_pscr(wol_gameres_type type, int size, void const * data);
static int _client_ppts(wol_gameres_type type, int size, void const * data);
static int _client_ptim(wol_gameres_type type, int size, void const * data);
static int _client_phlt(wol_gameres_type type, int size, void const * data);
static int _client_pkil(wol_gameres_type type, int size, void const * data);
static int _client_ekil(wol_gameres_type type, int size, void const * data);
static int _client_akil(wol_gameres_type type, int size, void const * data);
static int _client_shot(wol_gameres_type type, int size, void const * data);
static int _client_hedf(wol_gameres_type type, int size, void const * data);
static int _client_torf(wol_gameres_type type, int size, void const * data);
static int _client_armf(wol_gameres_type type, int size, void const * data);
static int _client_legf(wol_gameres_type type, int size, void const * data);
static int _client_crtf(wol_gameres_type type, int size, void const * data);
static int _client_pups(wol_gameres_type type, int size, void const * data);
static int _client_vkil(wol_gameres_type type, int size, void const * data);
static int _client_vtim(wol_gameres_type type, int size, void const * data);
static int _client_nkfv(wol_gameres_type type, int size, void const * data);
static int _client_squi(wol_gameres_type type, int size, void const * data);
static int _client_pcrd(wol_gameres_type type, int size, void const * data);
static int _client_bkil(wol_gameres_type type, int size, void const * data);
static int _client_hedr(wol_gameres_type type, int size, void const * data);
static int _client_torr(wol_gameres_type type, int size, void const * data);
static int _client_armr(wol_gameres_type type, int size, void const * data);
static int _client_legr(wol_gameres_type type, int size, void const * data);
static int _client_crtr(wol_gameres_type type, int size, void const * data);
static int _client_flgc(wol_gameres_type type, int size, void const * data);

/* Player related functions */
static int _client_nam0(wol_gameres_type type, int size, void const * data);
static int _client_nam1(wol_gameres_type type, int size, void const * data);
static int _client_nam2(wol_gameres_type type, int size, void const * data);
static int _client_nam3(wol_gameres_type type, int size, void const * data);
static int _client_nam4(wol_gameres_type type, int size, void const * data);
static int _client_nam5(wol_gameres_type type, int size, void const * data);
static int _client_nam6(wol_gameres_type type, int size, void const * data);
static int _client_nam7(wol_gameres_type type, int size, void const * data);
static int _client_cmp0(wol_gameres_type type, int size, void const * data);
static int _client_cmp1(wol_gameres_type type, int size, void const * data);
static int _client_cmp2(wol_gameres_type type, int size, void const * data);
static int _client_cmp3(wol_gameres_type type, int size, void const * data);
static int _client_cmp4(wol_gameres_type type, int size, void const * data);
static int _client_cmp5(wol_gameres_type type, int size, void const * data);
static int _client_cmp6(wol_gameres_type type, int size, void const * data);
static int _client_cmp7(wol_gameres_type type, int size, void const * data);

typedef int (* t_wol_gamerestag)(wol_gameres_type type, int size, void const * data);

typedef struct {
    t_tag               wol_gamerestag_uint;
    t_wol_gamerestag    wol_gamerestag_handler;
} t_wol_gamerestag_table_row;

/* handler table */
static const t_wol_gamerestag_table_row wol_gamreres_htable[] = {
    {CLIENT_SERN_UINT, _client_sern},
    {CLIENT_IDNO_UINT, _client_idno},
    {CLIENT_GSKU_UINT, _client_gsku},
    {CLIENT_TRNY_UINT, _client_trny},
    {CLIENT_OOSY_UINT, _client_oosy},
    {CLIENT_FINI_UINT, _client_fini},
    {CLIENT_DURA_UINT, _client_dura},
    {CLIENT_CRED_UINT, _client_cred},
    {CLIENT_SHRT_UINT, _client_shrt},
    {CLIENT_SUPR_UINT, _client_supr},
    {CLIENT_MODE_UINT, _client_mode},
    {CLIENT_BAMR_UINT, _client_bamr},
    {CLIENT_CRAT_UINT, _client_crat},
    {CLIENT_AIPL_UINT, _client_aipl},
    {CLIENT_UNIT_UINT, _client_unit},
    {CLIENT_SCEN_UINT, _client_scen},
    {CLIENT_PNGS_UINT, _client_pngs},
    {CLIENT_PNGR_UINT, _client_pngr},
    {CLIENT_PLRS_UINT, _client_plrs},
    {CLIENT_SPID_UINT, _client_spid},
    {CLIENT_TIME_UINT, _client_time},
    {CLIENT_AFPS_UINT, _client_afps},
    {CLIENT_PROC_UINT, _client_proc},
    {CLIENT_MEMO_UINT, _client_memo},
    {CLIENT_VIDM_UINT, _client_vidm},
    {CLIENT_SPED_UINT, _client_sped},
    {CLIENT_VERS_UINT, _client_vers},
    {CLIENT_DATE_UINT, _client_date},
    {CLIENT_BASE_UINT, _client_base},
    {CLIENT_TIBR_UINT, _client_tibr},
    {CLIENT_SHAD_UINT, _client_shad},
    {CLIENT_FLAG_UINT, _client_flag},
    {CLIENT_TECH_UINT, _client_tech},
    {CLIENT_BROK_UINT, _client_brok},
    {CLIENT_ACCO_UINT, _client_acco},
    {CLIENT_ETIM_UINT, _client_etim},
    {CLIENT_PSPD_UINT, _client_pspd},
    {CLIENT_SMEM_UINT, _client_smem},
    {CLIENT_SVID_UINT, _client_svid},
    {CLIENT_SNAM_UINT, _client_snam},
    {CLIENT_GMAP_UINT, _client_gmap},
    {CLIENT_DSVR_UINT, _client_dsvr},
    
    /* Renegade Player related tags */
    {CLIENT_PNAM_UINT, _client_pnam},
    {CLIENT_PLOC_UINT, _client_ploc},
    {CLIENT_TEAM_UINT, _client_team},
    {CLIENT_PSCR_UINT, _client_pscr},
    {CLIENT_PPTS_UINT, _client_ppts},
    {CLIENT_PTIM_UINT, _client_ptim},
    {CLIENT_PHLT_UINT, _client_phlt},
    {CLIENT_PKIL_UINT, _client_pkil},
    {CLIENT_EKIL_UINT, _client_ekil},
    {CLIENT_AKIL_UINT, _client_akil},
    {CLIENT_SHOT_UINT, _client_shot},
    {CLIENT_HEDF_UINT, _client_hedf},
    {CLIENT_TORF_UINT, _client_torf},
    {CLIENT_ARMF_UINT, _client_armf},
    {CLIENT_LEGF_UINT, _client_legf},
    {CLIENT_CRTF_UINT, _client_crtf},
    {CLIENT_PUPS_UINT, _client_pups},
    {CLIENT_VKIL_UINT, _client_vkil},
    {CLIENT_VTIM_UINT, _client_vtim},
    {CLIENT_NKFV_UINT, _client_nkfv},
    {CLIENT_SQUI_UINT, _client_squi},
    {CLIENT_PCRD_UINT, _client_pcrd},
    {CLIENT_BKIL_UINT, _client_bkil},
    {CLIENT_HEDR_UINT, _client_hedr},
    {CLIENT_TORR_UINT, _client_torr},
    {CLIENT_ARMR_UINT, _client_armr},
    {CLIENT_LEGR_UINT, _client_legr},
    {CLIENT_CRTR_UINT, _client_crtr},
    {CLIENT_FLGC_UINT, _client_flgc},

    /* Player related tags */
    {CLIENT_NAM0_UINT, _client_nam0},
    {CLIENT_NAM1_UINT, _client_nam1},
    {CLIENT_NAM2_UINT, _client_nam2},
    {CLIENT_NAM3_UINT, _client_nam3},
    {CLIENT_NAM4_UINT, _client_nam4},
    {CLIENT_NAM5_UINT, _client_nam5},
    {CLIENT_NAM6_UINT, _client_nam6},
    {CLIENT_NAM7_UINT, _client_nam7},
    {CLIENT_CMP0_UINT, _client_cmp0},
    {CLIENT_CMP1_UINT, _client_cmp1},
    {CLIENT_CMP2_UINT, _client_cmp2},
    {CLIENT_CMP3_UINT, _client_cmp3},
    {CLIENT_CMP4_UINT, _client_cmp4},
    {CLIENT_CMP5_UINT, _client_cmp5},
    {CLIENT_CMP6_UINT, _client_cmp6},
    {CLIENT_CMP7_UINT, _client_cmp7},

    {1, NULL}
};

static int handle_wolgameres_tag(t_tag gamerestag, wol_gameres_type type, int size, void const * data)
{
  t_wol_gamerestag_table_row const *p;

  for (p = wol_gamreres_htable; p->wol_gamerestag_uint != 1; p++) {
    if (gamerestag == p->wol_gamerestag_uint) {
	  if (p->wol_gamerestag_handler != NULL)
		  return ((p->wol_gamerestag_handler)(type,size,data));
	}
  }
  return -1;
}

static wol_gameres_type wol_gameres_type_from_int(int type)
{
    switch (type) {
        case 1:
            return wol_gameres_type_byte;
        case 2:
            return wol_gameres_type_bool;
        case 5:
            return wol_gameres_type_time;
        case 6:
            return wol_gameres_type_int;
        case 7:
            return wol_gameres_type_string;
        default:
            WARN1("type %d is not defined",type);
            return wol_gameres_type_unknown;
    }
}

extern int handle_wol_gameres_packet(t_connection * c, t_packet const * const packet)
{
    unsigned offset;
    t_tag wgtag;
    char wgtag_str[5];
    unsigned datatype;
    unsigned datalen;
    void const * data;
    wol_gameres_type type;

    DEBUG2("[%d] got WOL Gameres packet length %u", conn_get_socket(c), packet_get_size(packet));
    offset = sizeof(t_wolgameres_header);
    
    if ((unsigned int) bn_int_nget(*((bn_int *) packet_get_data_const(packet, offset, 4))) == 0)
        offset += 4; /* Just trying to get RNGD working */

    while (offset < packet_get_size(packet)) {
        wgtag = (unsigned int) bn_int_nget(*((bn_int *) packet_get_data_const(packet, offset, 4)));
        offset += 4;

        datatype = (unsigned int) bn_short_nget(*((bn_short *) packet_get_data_const(packet, offset, 2)));
        offset += 2;

        datalen = (unsigned int) bn_short_nget(*((bn_short *) packet_get_data_const(packet, offset, 2)));
        offset += 2;

        data = packet_get_data_const(packet, offset, 1);
        type = wol_gameres_type_from_int(datatype);

        if (handle_wolgameres_tag(wgtag, type, datalen, data) != 0) {
            char ch_data[255];
            tag_uint_to_str(wgtag_str, wgtag);
            if ((type == wol_gameres_type_bool) || (type == wol_gameres_type_byte)) {
                snprintf (ch_data, sizeof(ch_data),"%u", (unsigned int) bn_byte_get(*((bn_byte *)data)));
            }
            else if ((type == wol_gameres_type_int) || (type == wol_gameres_type_time)){
                snprintf (ch_data, sizeof(ch_data),"%u", (unsigned int) bn_int_nget(*((bn_int *)data)));
            }
            else if (type == wol_gameres_type_string){
                snprintf (ch_data, sizeof(ch_data),"%s", data);
            }
            else
                snprintf (ch_data, sizeof(ch_data),"unknown");
            eventlog(eventlog_level_warn, __FUNCTION__, "[%d] got unknown WOL Gameres tag: %s, data type %u, data lent %u, data %s",conn_get_socket(c), wgtag_str, datatype, datalen, ch_data);

        }

        datalen = 4 * ((datalen + 3) / 4);
//        offset = 4 * ((offset + 3) / 4);
        offset += datalen;
    }

    return 0;
}

/**
 * WOL Gamreres tag handlers
 */

static int _client_sern(wol_gameres_type type, int size, void const * data)
{
    /* SER# */
    return 0;
}

static int _client_idno(wol_gameres_type type, int size, void const * data)
{
    int gameidnumber;

    if (type == wol_gameres_type_int) {
        gameidnumber = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game ID Number: %u", gameidnumber);
    }
    else
        WARN1("got unknown gameres type %u for IDNO", type);

    return 0;
}

static int _client_gsku(wol_gameres_type type, int size, void const * data)
{
    int sku;

    if (type == wol_gameres_type_int) {
        sku = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Got WOL game resolution of %s", clienttag_get_title(tag_sku_to_uint(sku)));
    }
    else
        WARN1("got unknown gameres type %u for GSKU", type);

    return 0;
}

static int _client_trny(wol_gameres_type type, int size, void const * data)
{
    int tournament;

    switch (type) {
        case wol_gameres_type_int:
            tournament = (unsigned int) bn_int_nget(*((bn_int *)data));
            break;
        case wol_gameres_type_string:
            if (std::strcmp("TY  ", (const char *) data) == 0) /* for RNGD */
                tournament = 1;
            else if (std::strcmp("TN  ", (const char *) data) == 0) /* for RNGD */
                tournament = 0; 
            else {
                WARN1("got unknown string for TRNY: %s",data);
                tournament = 0;
            }
            break;
        default:
            WARN1("got unknown gameres type %u for TRNY", type);
            break;
    }

    if (tournament)
        DEBUG0("game was tournament");
    else
        DEBUG0("game was not tournament");

    return 0;
}

static int _client_oosy(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_fini(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_dura(wol_gameres_type type, int size, void const * data)
{
    unsigned int duration;

    /* Game dureation in seconds??? */
    // DURA : Game Duration
    // DURA 00 06 00 04 [00 00 01 F4]
    // DURA 00 06 00 04 [00 00 00 06]

    if (type == wol_gameres_type_int) {
        duration = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game duration %u seconds", duration);
    }
    else
        WARN1("got unknown gameres type %u for DURA", type);


    return 0;
}

static int _client_cred(wol_gameres_type type, int size, void const * data)
{
    unsigned int credits;

    if (type == wol_gameres_type_int) {
        credits = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game was start with %u credits", credits);
    }
    else
        WARN1("got unknown gameres type %u for CRED", type);

    return 0;
}

static int _client_shrt(wol_gameres_type type, int size, void const * data)
{
    bool shortgame;

    if (type == wol_gameres_type_bool) {
        shortgame = (bool) bn_byte_get(*((bn_byte *)data));
    }
    else if (type == wol_gameres_type_int) {
        shortgame = (bool) bn_int_nget(*((bn_int *)data));
    }
    else {
        WARN1("got unknown gameres type %u for SHRT", type);
        return 0;
    }

    if (shortgame)
        DEBUG0("Game has shortgame ON");
    else
        DEBUG0("Game has shortgame OFF");

    return 0;
}

static int _client_supr(wol_gameres_type type, int size, void const * data)
{
    bool superweapons;

    if (type == wol_gameres_type_bool) {
        superweapons = (bool) bn_byte_get(*((bn_byte *)data));
    }
    else if (type == wol_gameres_type_int) {
        superweapons = (bool) bn_int_nget(*((bn_int *)data));
    }
    else {
        WARN1("got unknown gameres type %u for SUPR", type);
        return 0;
    }

    if (superweapons)
        DEBUG0("Game has superweapons ON");
    else
        DEBUG0("Game has superweapons OFF");
    
    return 0;
}

static int _client_mode(wol_gameres_type type, int size, void const * data)
{
    /* This tag sends RNGD only */
      
    switch (type) {
        case wol_gameres_type_string:
            DEBUG1("Game mode: %s", data); /* For RNGD */
            break;
        default:
            WARN1("got unknown gameres type %u for MODE", type);
            break;
    }

    return 0;
}

static int _client_bamr(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_crat(wol_gameres_type type, int size, void const * data)
{
    bool crates;

    if (type == wol_gameres_type_bool) {
        crates = (bool) bn_byte_get(*((bn_byte *)data));
    }
    else if (type == wol_gameres_type_int) {
        crates = (bool) bn_int_nget(*((bn_int *)data));
    }
    else {
        WARN1("got unknown gameres type %u for CRAT", type);
        return 0;
    }

    if (crates)
        DEBUG0("Game has crates ON");
    else
        DEBUG0("Game has crates OFF");

    return 0;
}

static int _client_aipl(wol_gameres_type type, int size, void const * data)
{
    unsigned int aicount;

    if (type == wol_gameres_type_int) {
        aicount = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game has %u AI players", aicount);
    }
    else {
        WARN1("got unknown gameres type %u for AIPL", type);
        return 0;
    }

    return 0;
}

static int _client_unit(wol_gameres_type type, int size, void const * data)
{
    int units;

    if (type == wol_gameres_type_int) {
        units = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game was start with %u units", units);
    }
    else
        WARN1("got unknown gameres type %u for UNIT", type);

    return 0;
}

static int _client_scen(wol_gameres_type type, int size, void const * data)
{
    if (type == wol_gameres_type_string) {
        DEBUG1("Secnario %s",data);
    }
    else
        WARN1("got unknown gameres type %u for SCEN", type);

    return 0;
}

static int _client_pngs(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_pngr(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_plrs(wol_gameres_type type, int size, void const * data)
{
    unsigned int playercount;

    if (type == wol_gameres_type_int) {
        playercount = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game was played by %u players", playercount);
    }
    else
        WARN1("got unknown gameres type %u for PLRS", type);

    return 0;
}

static int _client_spid(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_time(wol_gameres_type type, int size, void const * data)
{
    /* PELISH: Time when was game started. This time is WOL STARTG packet time +-2 sec */
    std::time_t time;

    switch (type) {
        case wol_gameres_type_string:
            DEBUG1("Game was start at %s ", data);
            break;
        case wol_gameres_type_time:
            time = (std::time_t) bn_int_nget(*((bn_int *)data));
            DEBUG1("Game was start at %s ", ctime(&time));
            break;
        default:
            WARN1("got unknown gameres type %u for TIME", type);
            break;
    }
    return 0;
}

static int _client_afps(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_proc(wol_gameres_type type, int size, void const * data)
{
    switch (type) {
        case wol_gameres_type_string:
            DEBUG1("Procesor %s", data);
            break;
        default:
            WARN1("got unknown gameres type %u for SCEN", type);
            break;
    }
    return 0;
}

static int _client_memo(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_vidm(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_sped(wol_gameres_type type, int size, void const * data)
{
    unsigned int speed;

    //SPED 00 01 00 01 [00 00 00 00]

    if (type == wol_gameres_type_byte) {
        speed = (unsigned int) bn_byte_get(*((bn_byte *)data));
    }
    else if (type == wol_gameres_type_int) {
        speed = (unsigned int) bn_int_nget(*((bn_int *)data));
    }
    else {
        WARN1("got unknown gameres type %u for SPED", type);
        return 0;
    }

    DEBUG1("Game speed %u", speed);
    return 0;
}

static int _client_vers(wol_gameres_type type, int size, void const * data)
{
    unsigned int version;

    if (type == wol_gameres_type_string) {
        DEBUG1("Version of client %s",data);
    }
    else if (type == wol_gameres_type_int) {
        version = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Version of client %u",version);
    }
    else
        WARN1("got unknown gameres type %u for VERS", type);

    return 0;
}

static int _client_date(wol_gameres_type type, int size, void const * data)
{
    if (type == wol_gameres_type_string) {
        DEBUG1("Date %s",data);
    }
    else
        WARN1("got unknown gameres type %u for DATE", type);
    return 0;
}

static int _client_base(wol_gameres_type type, int size, void const * data)
{
    bool bases;

    //WOLv1 BASE 00 07 00 03 ON

    if (type == wol_gameres_type_bool) {
        bases = (bool) bn_byte_get(*((bn_byte *)data));
        if (bases)
            DEBUG0("Game has bases ON");
        else
            DEBUG0("Game has bases OFF");
    }
    else if (type == wol_gameres_type_string) {
        DEBUG1("Game has bases %s", data);
    }
    else
        WARN1("got unknown gameres type %u for BASE", type);

    return 0;
}

static int _client_tibr(wol_gameres_type type, int size, void const * data)
{
    bool tiberium;

    if (type == wol_gameres_type_bool) {
        tiberium = (bool) bn_byte_get(*((bn_byte *)data));
        if (tiberium)
            DEBUG0("Game has tiberium ON");
        else
            DEBUG0("Game has tiberium OFF");
    }
    else if (type == wol_gameres_type_string) {
        DEBUG1("Game has tiberium %s", data);
    }
    else
        WARN1("got unknown gameres type %u for TIBR", type);

    return 0;
}

static int _client_shad(wol_gameres_type type, int size, void const * data)
{
    //SHAD 00 02 00 01 [00 FF FF FF]
    //SHAD 00 02 00 01 [00 00 00 00]
    
    bool shadows;

    if (type == wol_gameres_type_bool) {
        shadows = (bool) bn_byte_get(*((bn_byte *)data));
        if (shadows)
            DEBUG0("Game has shadows ON");
        else
            DEBUG0("Game has shadows OFF");
    }
    else
        WARN1("got unknown gameres type %u for SHAD", type);

    return 0;
}

static int _client_flag(wol_gameres_type type, int size, void const * data)
{
    bool captureflag;

    if (type == wol_gameres_type_bool) {
        captureflag = (bool) bn_byte_get(*((bn_byte *)data));
        if (captureflag)
            DEBUG0("Game has capture the flag ON");
        else
            DEBUG0("Game has capture the flag OFF");
    }
    else
        WARN1("got unknown gameres type %u for FLAG", type);

    return 0;
}

static int _client_tech(wol_gameres_type type, int size, void const * data)
{
    unsigned int techlevel;

    if (type == wol_gameres_type_int) {
        techlevel = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game has techlevel %u", techlevel);
    }
    else
        WARN1("got unknown gameres type %u for TECH", type);

    return 0;
}

static int _client_brok(wol_gameres_type type, int size, void const * data)
{
    unsigned int brok;

    /* This tag sends YURI */
    //BROK 00 06 00 04 00 00 00 02

    if (type == wol_gameres_type_int) {
        brok = (unsigned int) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game has BROK %u", brok);
    }
    else
        WARN1("got unknown gameres type %u for BROK", type);

    return 0;
}

static int _client_acco(wol_gameres_type type, int size, void const * data)
{
    unsigned int acco;

    /* This tag sends YURI */
    //ACCO 00 06 00 04 00 00 00 00

    switch (type) {
        case wol_gameres_type_int:
            acco = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Game has ACCO %u", acco);
            break;
        default:
            WARN1("got unknown gameres type %u for ACCO", type);
            break;
    }

    return 0;
}

static int _client_etim(wol_gameres_type type, int size, void const * data)
{
    /* This tag sends RNGD only */

    /* Elapsed time??? this tag got unknown data type 0x0014 */

    return 0;
}

static int _client_pspd(wol_gameres_type type, int size, void const * data)
{
    /* This tag sends RNGD only */
    unsigned int procesorspeed;

    switch (type) {
        case wol_gameres_type_int:
            procesorspeed = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Server have processor speed: %u MHz", procesorspeed);
            break;
        default:
            WARN1("got unknown gameres type %u for PSPD", type);
            break;
    }
    return 0;
}

static int _client_smem(wol_gameres_type type, int size, void const * data)
{
    /* This tag sends RNGD only */
    unsigned int servermemory;

    switch (type) {
        case wol_gameres_type_int:
            servermemory = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Server have %u memory", servermemory);
            break;
        default:
            WARN1("got unknown gameres type %u for SMEM", type);
            break;
    }
    return 0;
}

static int _client_svid(wol_gameres_type type, int size, void const * data)
{
    /* This tag sends RNGD only */

    /* Server ID ??? this tag got unknown data type 0x0014 */

    return 0;
}

static int _client_snam(wol_gameres_type type, int size, void const * data)
{
    /* This tag sends RNGD only */

    switch (type) {
        case wol_gameres_type_string:
            DEBUG1("Server name: %s", data);
            break;
        default:
            WARN1("got unknown gameres type %u for SNAM", type);
            break;
    }
    return 0;
}

static int _client_gmap(wol_gameres_type type, int size, void const * data)
{
    /* This tag sends RNGD only */
      
    switch (type) {
        case wol_gameres_type_string:
            DEBUG1("Game map: %s", data);
            break;
        default:
            WARN1("got unknown gameres type %u for GMAP", type);
            break;
    }
    return 0;
}

static int _client_dsvr(wol_gameres_type type, int size, void const * data)
{
    /* This tag sends RNGD only */
    unsigned int dsvr;

    switch (type) {
        case wol_gameres_type_byte:
            dsvr = (unsigned int) bn_byte_get(*((bn_byte *)data));
            DEBUG1("DSVR: %u", dsvr);
            break;
        default:
            WARN1("got unknown gameres type %u for SCEN", type);
            break;
    }
    return 0;
}


/* Renegade player tag handlers */
static int _client_pnam(wol_gameres_type type, int size, void const * data)
{
    switch (type) {
        case wol_gameres_type_string:
            DEBUG1("Player name: %s", data);
            break;
        default:
            WARN1("got unknown gameres type %u for PNAM", type);
            break;
    }
    return 0;
}

static int _client_ploc(wol_gameres_type type, int size, void const * data)
{
    unsigned int playerloc;

    switch (type) {
        case wol_gameres_type_int:
            playerloc = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Player locale: %u", playerloc);
            break;
        default:
            WARN1("got unknown gameres type %u for PLOC", type);
            break;
    }
    return 0;
}

static int _client_team(wol_gameres_type type, int size, void const * data)
{
    unsigned int team;

    switch (type) {
        case wol_gameres_type_int:
            team = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Team: %u", team);
            break;
        default:
            WARN1("got unknown gameres type %u for TEAM", type);
            break;
    }
    return 0;
}

static int _client_pscr(wol_gameres_type type, int size, void const * data)
{
    unsigned int score;

    switch (type) {
        case wol_gameres_type_int:
            score = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Players score: %u", score);
            break;
        default:
            WARN1("got unknown gameres type %u for PSCR", type);
            break;
    }
    return 0;
}

static int _client_ppts(wol_gameres_type type, int size, void const * data)
{
    unsigned int points;

    switch (type) {
        case wol_gameres_type_time: /* PELISH: THIS is not a BUG! This is Renegade developers bug */
            points = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Points %u", points);
            break;
        default:
            WARN1("got unknown gameres type %u for PPTS", type);
            break;
    }
    return 0;
}

static int _client_ptim(wol_gameres_type type, int size, void const * data)
{
    unsigned int playertime;

    switch (type) {
        case wol_gameres_type_int:
            playertime = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Player game time %u", playertime);
            break;
        default:
            WARN1("got unknown gameres type %u for PTIM", type);
            break;
    }
    return 0;
}

static int _client_phlt(wol_gameres_type type, int size, void const * data)
{
    unsigned int health;

    switch (type) {
        case wol_gameres_type_int:
            health = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Health: %u", health);
            break;
        default:
            WARN1("got unknown gameres type %u for PHLT", type);
            break;
    }
    return 0;
}

static int _client_pkil(wol_gameres_type type, int size, void const * data)
{
    unsigned int kills;

    switch (type) {
        case wol_gameres_type_int:
            kills = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Kills: %u", kills);
            break;
        default:
            WARN1("got unknown gameres type %u for PKIL", type);
            break;
    }
    return 0;
}

static int _client_ekil(wol_gameres_type type, int size, void const * data)
{
    unsigned int enemy_kills;

    switch (type) {
        case wol_gameres_type_int:
            enemy_kills = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Enemy kills: %u", enemy_kills);
            break;
        default:
            WARN1("got unknown gameres type %u for EKIL", type);
            break;
    }
    return 0;
}

static int _client_akil(wol_gameres_type type, int size, void const * data)
{
    unsigned int ally_kills;

    switch (type) {
        case wol_gameres_type_int:
            ally_kills = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Ally kills %u", ally_kills);
            break;
        default:
            WARN1("got unknown gameres type %u for AKIL", type);
            break;
    }
    return 0;
}

static int _client_shot(wol_gameres_type type, int size, void const * data)
{
    unsigned int shots;

    switch (type) {
        case wol_gameres_type_int:
            shots = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Shots %u", shots);
            break;
        default:
            WARN1("got unknown gameres type %u for SHOT", type);
            break;
    }
    return 0;
}

static int _client_hedf(wol_gameres_type type, int size, void const * data)
{
    unsigned int hedf;

    switch (type) {
        case wol_gameres_type_int:
            hedf = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("hedf %u", hedf);
            break;
        default:
            WARN1("got unknown gameres type %u for HEDF", type);
            break;
    }
    return 0;
}

static int _client_torf(wol_gameres_type type, int size, void const * data)
{
    unsigned int torf;

    switch (type) {
        case wol_gameres_type_int:
            torf = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("torf %u", torf);
            break;
        default:
            WARN1("got unknown gameres type %u for TORF", type);
            break;
    }
    return 0;
}

static int _client_armf(wol_gameres_type type, int size, void const * data)
{
    unsigned int armf;

    switch (type) {
        case wol_gameres_type_int:
            armf = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("armf %u", armf);
            break;
        default:
            WARN1("got unknown gameres type %u for ARMF", type);
            break;
    }
    return 0;
}

static int _client_legf(wol_gameres_type type, int size, void const * data)
{
    unsigned int legf;

    switch (type) {
        case wol_gameres_type_int:
            legf = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("legf %u", legf);
            break;
        default:
            WARN1("got unknown gameres type %u for LEGF", type);
            break;
    }
    return 0;
}

static int _client_crtf(wol_gameres_type type, int size, void const * data)
{
    unsigned int crtf;

    switch (type) {
        case wol_gameres_type_int:
            crtf = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("crtf %u", crtf);
            break;
        default:
            WARN1("got unknown gameres type %u for CRTF", type);
            break;
    }
    return 0;
}

static int _client_pups(wol_gameres_type type, int size, void const * data)
{
    unsigned int pups;

    switch (type) {
        case wol_gameres_type_int:
            pups = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("pups %u", pups);
            break;
        default:
            WARN1("got unknown gameres type %u for PUPS", type);
            break;
    }
    return 0;
}

static int _client_vkil(wol_gameres_type type, int size, void const * data)
{
    unsigned int victory_kills;

    switch (type) {
        case wol_gameres_type_int:
            victory_kills = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Victory kills %u", victory_kills);
            break;
        default:
            WARN1("got unknown gameres type %u for VKIL", type);
            break;
    }
    return 0;
}

static int _client_vtim(wol_gameres_type type, int size, void const * data)
{
    unsigned int victory_time;

    switch (type) {
        case wol_gameres_type_int:
            victory_time = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("Victory time %u", victory_time);
            break;
        default:
            WARN1("got unknown gameres type %u for VTIM", type);
            break;
    }
    return 0;
}

static int _client_nkfv(wol_gameres_type type, int size, void const * data)
{
    unsigned int nkfv;

    switch (type) {
        case wol_gameres_type_int:
            nkfv = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("nkfv %u", nkfv);
            break;
        default:
            WARN1("got unknown gameres type %u for NKFV", type);
            break;
    }
    return 0;
}

static int _client_squi(wol_gameres_type type, int size, void const * data)
{
    unsigned int squi;

    switch (type) {
        case wol_gameres_type_int:
            squi = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("squi %u", squi);
            break;
        default:
            WARN1("got unknown gameres type %u for SQUI", type);
            break;
    }
    return 0;
}

static int _client_pcrd(wol_gameres_type type, int size, void const * data)
{
    unsigned int pcrd;

    switch (type) {
        case wol_gameres_type_int:
            pcrd = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("pcrd %u", pcrd);
            break;
        default:
            WARN1("got unknown gameres type %u for PCRD", type);
            break;
    }
    return 0;
}

static int _client_bkil(wol_gameres_type type, int size, void const * data)
{
    unsigned int bkil;

    switch (type) {
        case wol_gameres_type_int:
            bkil = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("bkil %u", bkil);
            break;
        default:
            WARN1("got unknown gameres type %u for BKIL", type);
            break;
    }
    return 0;
}

static int _client_hedr(wol_gameres_type type, int size, void const * data)
{
    unsigned int hedr;

    switch (type) {
        case wol_gameres_type_int:
            hedr = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("hedr %u", hedr);
            break;
        default:
            WARN1("got unknown gameres type %u for HEDR", type);
            break;
    }
    return 0;
}

static int _client_torr(wol_gameres_type type, int size, void const * data)
{
    unsigned int torr;

    switch (type) {
        case wol_gameres_type_int:
            torr = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("torr %u", torr);
            break;
        default:
            WARN1("got unknown gameres type %u for TORR", type);
            break;
    }
    return 0;
}

static int _client_armr(wol_gameres_type type, int size, void const * data)
{
    unsigned int armr;

    switch (type) {
        case wol_gameres_type_int:
            armr = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("armr %u", armr);
            break;
        default:
            WARN1("got unknown gameres type %u for ARMR", type);
            break;
    }
    return 0;
}

static int _client_legr(wol_gameres_type type, int size, void const * data)
{
    unsigned int legr;

    switch (type) {
        case wol_gameres_type_int:
            legr = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("legr %u", legr);
            break;
        default:
            WARN1("got unknown gameres type %u for LEGR", type);
            break;
    }
    return 0;
}

static int _client_crtr(wol_gameres_type type, int size, void const * data)
{
    unsigned int crtr;

    switch (type) {
        case wol_gameres_type_int:
            crtr = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("crtr %u", crtr);
            break;
        default:
            WARN1("got unknown gameres type %u for CRTR", type);
            break;
    }
    return 0;
}

static int _client_flgc(wol_gameres_type type, int size, void const * data)
{
    unsigned int flgc;

    switch (type) {
        case wol_gameres_type_int:
            flgc = (unsigned int) bn_int_nget(*((bn_int *)data));
            DEBUG1("flgc %u", flgc);
            break;
        default:
            WARN1("got unknown gameres type %u for FLGC", type);
            break;
    }
    return 0;
}



/* player related tag handlers */

static int _client_nam0(wol_gameres_type type, int size, void const * data)
{
    DEBUG1("Name of palyer 0: %s",data);
    return 0;
}

static int _client_nam1(wol_gameres_type type, int size, void const * data)
{
    DEBUG1("Name of palyer 1: %s",data);
    return 0;
}

static int _client_nam2(wol_gameres_type type, int size, void const * data)
{
    DEBUG1("Name of palyer 2: %s",data);
    return 0;
}

static int _client_nam3(wol_gameres_type type, int size, void const * data)
{
    DEBUG1("Name of palyer 3: %s",data);
    return 0;
}

static int _client_nam4(wol_gameres_type type, int size, void const * data)
{
    DEBUG1("Name of palyer 4: %s",data);
    return 0;
}

static int _client_nam5(wol_gameres_type type, int size, void const * data)
{
    DEBUG1("Name of palyer 5: %s",data);
    return 0;
}

static int _client_nam6(wol_gameres_type type, int size, void const * data)
{
    DEBUG1("Name of palyer 6: %s",data);
    return 0;
}

static int _client_nam7(wol_gameres_type type, int size, void const * data)
{
    DEBUG1("Name of palyer 7: %s",data);
    return 0;
}

/**/
static int show_player_reslut(int playernum, int result)
{
    /* FIXME: This does not work! */
    DEBUG2("%u player has %u", playernum ,result);
    if (result == 0x100)
        DEBUG0("WIN");
    if (result == 0x200)
        DEBUG0("LOSE");
    if (result == 0x300)
        DEBUG0("DISCONECT");
    return 0;
}
/**/

static int _client_cmp0(wol_gameres_type type, int size, void const * data)
{
    //CMPx 00 06 00 04 00 00 02 00
    //0x0100=WIN 0x0200=LOSE (0x0210) 0x0300=DISCONECT

    int result;

    if (type == wol_gameres_type_int) {
        result = (unsigned int) bn_int_nget(*((bn_int *)data));
        show_player_reslut(0,result);
    }
    return 0;
}

static int _client_cmp1(wol_gameres_type type, int size, void const * data)
{
    int result;

    if (type == wol_gameres_type_int) {
        result = (unsigned int) bn_int_nget(*((bn_int *)data));
        show_player_reslut(1,result);
    }
    return 0;
}

static int _client_cmp2(wol_gameres_type type, int size, void const * data)
{
    int result;

    if (type == wol_gameres_type_int) {
        result = (unsigned int) bn_int_nget(*((bn_int *)data));
        show_player_reslut(2,result);
    }
    return 0;
}

static int _client_cmp3(wol_gameres_type type, int size, void const * data)
{
    int result;

    if (type == wol_gameres_type_int) {
        result = (unsigned int) bn_int_nget(*((bn_int *)data));
        show_player_reslut(3,result);
    }
    return 0;
}

static int _client_cmp4(wol_gameres_type type, int size, void const * data)
{
    int result;

    if (type == wol_gameres_type_int) {
        result = (unsigned int) bn_int_nget(*((bn_int *)data));
        show_player_reslut(4,result);
    }
    return 0;
}

static int _client_cmp5(wol_gameres_type type, int size, void const * data)
{
    int result;

    if (type == wol_gameres_type_int) {
        result = (unsigned int) bn_int_nget(*((bn_int *)data));
        show_player_reslut(5,result);
    }
    return 0;
}

static int _client_cmp6(wol_gameres_type type, int size, void const * data)
{
    int result;

    if (type == wol_gameres_type_int) {
        result = (unsigned int) bn_int_nget(*((bn_int *)data));
        show_player_reslut(6,result);
    }
    return 0;
}

static int _client_cmp7(wol_gameres_type type, int size, void const * data)
{
    int result;

    if (type == wol_gameres_type_int) {
        result = (unsigned int) bn_int_nget(*((bn_int *)data));
        show_player_reslut(7,result);
    }
    return 0;
}

}

}
