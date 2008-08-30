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

//#include <sstream>
//nclude <cstring>
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

    DEBUG2("[%d] got WOL Gameres packet length %u", conn_get_socket(c), packet_get_size(packet));
    offset = sizeof(t_wolgameres_header);

    while (offset < packet_get_size(packet)) {
        wgtag = (unsigned int) bn_int_nget(*((bn_int *) packet_get_data_const(packet, offset, 4)));
        offset += 4;

        datatype = (unsigned int) bn_short_nget(*((bn_short *) packet_get_data_const(packet, offset, 2)));
        offset += 2;

        datalen = (unsigned int) bn_short_nget(*((bn_short *) packet_get_data_const(packet, offset, 2)));
        offset += 2;

        data = packet_get_data_const(packet, offset, 1);

        if (handle_wolgameres_tag(wgtag, wol_gameres_type_from_int(datatype), datalen, data) != 0) {
            tag_uint_to_str(wgtag_str, wgtag);
            eventlog(eventlog_level_warn, __FUNCTION__, "[%d] got unknown WOL Gameres tag: %s, data type %u, data lent %u",conn_get_socket(c), wgtag_str, datatype, datalen);
        }

        datalen = 4 * ((datalen + 3) / 4);
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

    if (type == wol_gameres_type_int) {
        tournament = (unsigned int) bn_int_nget(*((bn_int *)data));

        if (tournament)
            DEBUG0("game is tournament");
        else
            DEBUG0("game is not tournament");
    }
    else
        WARN1("got unknown gameres type %u for TRNY", type);

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
    /* Game dureation in seconds??? */
    // DURA : Game Duration
    // DURA 00 06 00 04 [00 00 01 F4]
    // DURA 00 06 00 04 [00 00 00 06]


    return 0;
}

static int _client_cred(wol_gameres_type type, int size, void const * data)
{
    int credits;

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
        if (shortgame)
            DEBUG0("Game has shortgame ON");
        else
            DEBUG0("Game has shortgame OFF");
    }
    else
        WARN1("got unknown gameres type %u for SHRT", type);

    return 0;
}

static int _client_supr(wol_gameres_type type, int size, void const * data)
{
    bool superweapons;

    if (type == wol_gameres_type_bool) {
        superweapons = (bool) bn_byte_get(*((bn_byte *)data));
        if (superweapons)
            DEBUG0("Game has superweapons ON");
        else
            DEBUG0("Game has superweapons OFF");
    }
    else
        WARN1("got unknown gameres type %u for SUPR", type);

    return 0;
}

static int _client_mode(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
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
        if (crates)
            DEBUG0("Game has crates ON");
        else
            DEBUG0("Game has crates OFF");
    }
    else
        WARN1("got unknown gameres type %u for CRAT", type);

    return 0;
}

static int _client_aipl(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
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
    /* Not implemented yet */
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
        DEBUG1("Game was played %u players", playercount);
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
    /* PELISH: Time when was game started??? we need to check it ;) */
    std::time_t time;

    if (type == wol_gameres_type_time) {
        time = (std::time_t) bn_int_nget(*((bn_int *)data));
        DEBUG1("Game was start at %s ", ctime(&time));
    }
    else
        WARN1("got unknown gameres type %u for TIME", type);

    return 0;
}

static int _client_afps(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
    return 0;
}

static int _client_proc(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
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
        DEBUG1("Game speed %s", speed);
    }
    else
        WARN1("got unknown gameres type %u for VERS", type);

    return 0;
}

static int _client_vers(wol_gameres_type type, int size, void const * data)
{
    if (type == wol_gameres_type_string) {
        DEBUG1("Version of client %s",data);
    }
    else
        WARN1("got unknown gameres type %u for VERS", type);

    return 0;
}

static int _client_date(wol_gameres_type type, int size, void const * data)
{
    /* Not implemented yet */
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


/* player related tag handlers*/

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
