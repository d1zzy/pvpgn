/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
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
#include "setup.h"

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h> /* needed to include netinet/in.h */
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
# include <arpa/inet.h> /* FIXME: probably not needed... do some systems put types in here or something? */
#endif
#include "compat/psock.h"

#include "d2gs.h"
#include "handle_d2gs.h"
#include "serverqueue.h"
#include "game.h"
#include "connection.h"
#include "prefs.h"
#include "d2cs_d2gs_protocol.h"
#include "common/trans.h"
#include "common/addr.h"
#include "common/eventlog.h"
#include "common/queue.h"
#include "common/bn_type.h"
#include "common/packet.h"
#include "common/setup_after.h"

DECLARE_PACKET_HANDLER(on_d2gs_authreply)
DECLARE_PACKET_HANDLER(on_d2gs_setgsinfo)
DECLARE_PACKET_HANDLER(on_d2gs_echoreply)
DECLARE_PACKET_HANDLER(on_d2gs_creategamereply)
DECLARE_PACKET_HANDLER(on_d2gs_joingamereply)
DECLARE_PACKET_HANDLER(on_d2gs_updategameinfo)
DECLARE_PACKET_HANDLER(on_d2gs_closegame)

static t_packet_handle_table d2gs_packet_handle_table[]={
/* 0x00 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x01 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x02 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x03 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x04 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x05 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x06 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x07 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x08 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x09 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x0a */ { 0,                                  conn_state_none,       NULL                    },
/* 0x0b */ { 0,                                  conn_state_none,       NULL                    },
/* 0x0c */ { 0,                                  conn_state_none,       NULL                    },
/* 0x0d */ { 0,                                  conn_state_none,       NULL                    },
/* 0x0e */ { 0,                                  conn_state_none,       NULL                    },
/* 0x0f */ { 0,                                  conn_state_none,       NULL                    },
/* 0x10 */ { 0,                                  conn_state_none,       NULL                    },
/* FIXME: shouldn't these three be at 0x10-0x12? (But I'm pretty sure I preserved the padding) */
/* 0x11 */ { sizeof(t_d2gs_d2cs_authreply),      conn_state_connected,  on_d2gs_authreply       },
/* 0x12 */ { sizeof(t_d2gs_d2cs_setgsinfo),      conn_state_authed,     on_d2gs_setgsinfo       },
/* 0x13 */ { sizeof(t_d2gs_d2cs_echoreply),      conn_state_any,        on_d2gs_echoreply       },
/* 0x14 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x15 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x16 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x17 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x18 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x19 */ { 0,                                  conn_state_none,       NULL                    },
/* 0x1a */ { 0,                                  conn_state_none,       NULL                    },
/* 0x1b */ { 0,                                  conn_state_none,       NULL                    },
/* 0x1c */ { 0,                                  conn_state_none,       NULL                    },
/* 0x1d */ { 0,                                  conn_state_none,       NULL                    },
/* 0x1e */ { 0,                                  conn_state_none,       NULL                    },
/* 0x1f */ { 0,                                  conn_state_none,       NULL                    },
/* 0x20 */ { sizeof(t_d2gs_d2cs_creategamereply),conn_state_authed,     on_d2gs_creategamereply },
/* 0x21 */ { sizeof(t_d2gs_d2cs_joingamereply),  conn_state_authed,     on_d2gs_joingamereply   },
/* 0x22 */ { sizeof(t_d2gs_d2cs_updategameinfo), conn_state_authed,     on_d2gs_updategameinfo  },
/* 0x23 */ { sizeof(t_d2gs_d2cs_closegame),      conn_state_authed,     on_d2gs_closegame       },
};


static int on_d2gs_authreply(t_connection * c, t_packet * packet)
{
	t_d2gs		* gs;
	t_packet	* rpacket;
	unsigned int	reply;
	unsigned int	try_checksum, checksum;
	unsigned int	version;

	if (!(gs=d2gslist_find_gs(conn_get_d2gs_id(c)))) {
		eventlog(eventlog_level_error,__FUNCTION__,"game server %d not found",conn_get_d2gs_id(c));
		return -1;
	}

	version=bn_int_get(packet->u.d2gs_d2cs_authreply.version);
	try_checksum=bn_int_get(packet->u.d2gs_d2cs_authreply.checksum);
	checksum=d2gs_calc_checksum(c);
	if (prefs_get_d2gs_version() && (version != prefs_get_d2gs_version())) {
		eventlog(eventlog_level_error,__FUNCTION__,"game server %d version mismatch 0x%X - 0x%X",conn_get_d2gs_id(c),
			version,prefs_get_d2gs_version());
		reply=D2CS_D2GS_AUTHREPLY_BAD_VERSION;
	} else if (prefs_get_d2gs_checksum() && try_checksum != checksum) {
		eventlog(eventlog_level_error,__FUNCTION__,"game server %d checksum mismach 0x%X - 0x%X",conn_get_d2gs_id(c),try_checksum,checksum);
		reply=D2CS_D2GS_AUTHREPLY_BAD_CHECKSUM;
	} else {
		reply=D2CS_D2GS_AUTHREPLY_SUCCEED;
	}

	if (reply==D2CS_D2GS_AUTHREPLY_SUCCEED) {
		eventlog(eventlog_level_info,__FUNCTION__,"game server %s authed",addr_num_to_ip_str(d2cs_conn_get_addr(c)));
		d2cs_conn_set_state(c,conn_state_authed);
		d2gs_active(gs,c);
	} else {
		eventlog(eventlog_level_error,__FUNCTION__,"game server %s failed to auth",addr_num_to_ip_str(d2cs_conn_get_addr(c)));
		/* 
		d2cs_conn_set_state(c,conn_state_destroy); 
		*/
	}
	if ((rpacket=packet_create(packet_class_d2gs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_d2gs_authreply));
		packet_set_type(rpacket,D2CS_D2GS_AUTHREPLY);
		bn_int_set(&rpacket->u.d2cs_d2gs_authreply.h.seqno,1);
		bn_int_set(&rpacket->u.d2cs_d2gs_authreply.reply,reply);
		conn_push_outqueue(c,rpacket);
		packet_del_ref(rpacket);
	}
	
	// set d2gs version
	gs->d2gs_version = version;
	
	return 0;
}

static int on_d2gs_setgsinfo(t_connection * c, t_packet * packet)
{
	t_d2gs		* gs;
	t_packet	* rpacket;
	unsigned int	maxgame, prev_maxgame;
	unsigned int	currgame, gameflag;

	if (!(gs=d2gslist_find_gs(conn_get_d2gs_id(c)))) {
		eventlog(eventlog_level_error,__FUNCTION__,"game server %d not found",conn_get_d2gs_id(c));
		return -1;
	}
	maxgame=bn_int_get(packet->u.d2gs_d2cs_setgsinfo.maxgame);
	prev_maxgame=d2gs_get_maxgame(gs);
	currgame = d2gs_get_gamenum(gs);
	eventlog(eventlog_level_info, __FUNCTION__, "change game server %s max game from %d to %d (%d current)",addr_num_to_ip_str(d2cs_conn_get_addr(c)),prev_maxgame, maxgame, currgame);
	d2gs_set_maxgame(gs,maxgame);
	
	if(gs->d2gs_version > 0x1090007)
	{
		gameflag=bn_int_get(packet->u.d2gs_d2cs_setgsinfo.gameflag);
		
                if ((rpacket=packet_create(packet_class_d2gs))) {
		    packet_set_size(rpacket,sizeof(t_d2cs_d2gs_setgsinfo));
		    packet_set_type(rpacket,D2CS_D2GS_SETGSINFO);
		    bn_int_set(&rpacket->u.d2cs_d2gs_setgsinfo.h.seqno,1);
		    bn_int_set(&rpacket->u.d2cs_d2gs_setgsinfo.maxgame,maxgame);
		    bn_int_set(&rpacket->u.d2cs_d2gs_setgsinfo.gameflag,gameflag);
		    conn_push_outqueue(c,rpacket);
		    packet_del_ref(rpacket);
                }
        }
	
	gqlist_check_creategame();
	return 0;
}

static int on_d2gs_echoreply(t_connection * c, t_packet * packet)
{
	if (!c || !packet)
	    return 0;
	return 0;
}

static int on_d2gs_creategamereply(t_connection * c, t_packet * packet)
{
	t_packet	* opacket, * rpacket;
	t_sq		* sq;
	t_connection	* client;
	t_game		* game;
	int		result;
	int		reply;
	int		seqno;

	seqno=bn_int_get(packet->u.d2cs_d2gs.h.seqno);
	if (!(sq=sqlist_find_sq(seqno))) {
		eventlog(eventlog_level_error,__FUNCTION__,"seqno %d not found",seqno);
		return 0;
	}
	if (!(client=d2cs_connlist_find_connection_by_sessionnum(sq_get_clientid(sq)))) {
		eventlog(eventlog_level_error,__FUNCTION__,"client %d not found",sq_get_clientid(sq));
		sq_destroy(sq);
		return 0;
	}
	if (!(game=gamelist_find_game_by_id(sq_get_gameid(sq)))) {
		eventlog(eventlog_level_error,__FUNCTION__,"game %d not found",sq_get_gameid(sq));
		sq_destroy(sq);
		return 0;
	}
	if (!(opacket=sq_get_packet(sq))) {
		eventlog(eventlog_level_error,__FUNCTION__,"previous packet not found (seqno: %d)",seqno);
		sq_destroy(sq);
		return 0;
	}

	result=bn_int_get(packet->u.d2gs_d2cs_creategamereply.result);
	if (result==D2GS_D2CS_CREATEGAME_SUCCEED) {
		game_set_d2gs_gameid(game,bn_int_get(packet->u.d2gs_d2cs_creategamereply.gameid));
		game_set_created(game,1);
		eventlog(eventlog_level_info,__FUNCTION__,"game %s created on gs %d",d2cs_game_get_name(game),conn_get_d2gs_id(c));
		reply=D2CS_CLIENT_CREATEGAMEREPLY_SUCCEED;
	} else if (result==D2GS_D2CS_JOINGAME_GAME_FULL) {
		eventlog(eventlog_level_info, "on_d2gs_joingamereply", "failed to add %s to game %s on gs %d (game full)",d2cs_conn_get_charname(client),d2cs_game_get_name(game),conn_get_d2gs_id(c));
		reply=D2CS_CLIENT_JOINGAMEREPLY_GAME_FULL;
	} else {
		eventlog(eventlog_level_warn,__FUNCTION__,"failed to create game %s on gs %d",d2cs_game_get_name(game),conn_get_d2gs_id(c));
		game_destroy(game);
		reply=D2CS_CLIENT_CREATEGAMEREPLY_FAILED;
	}

	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_creategamereply));
		packet_set_type(rpacket,D2CS_CLIENT_CREATEGAMEREPLY);
		bn_short_set(&rpacket->u.d2cs_client_creategamereply.seqno,
				bn_short_get(opacket->u.client_d2cs_creategamereq.seqno));
		bn_short_set(&rpacket->u.d2cs_client_creategamereply.gameid,1);
		bn_short_set(&rpacket->u.d2cs_client_creategamereply.u1,1);
		bn_int_set(&rpacket->u.d2cs_client_creategamereply.reply,reply);
		conn_push_outqueue(client,rpacket);
		packet_del_ref(rpacket);
	}
	sq_destroy(sq);
	return 0;
}

static int on_d2gs_joingamereply(t_connection * c, t_packet * packet)
{
	t_sq		* sq;
	t_d2gs		* gs;
	t_connection	* client;
	t_game		* game;
	t_packet	* opacket, * rpacket;
	int		result;
	int		reply;
	int		seqno;
	unsigned int	gsaddr;
	unsigned short	gsport;
			

	seqno=bn_int_get(packet->u.d2cs_d2gs.h.seqno);
	if (!(sq=sqlist_find_sq(seqno))) {
		eventlog(eventlog_level_error,__FUNCTION__,"seqno %d not found",seqno);
		return 0;
	}
	if (!(client=d2cs_connlist_find_connection_by_sessionnum(sq_get_clientid(sq)))) {
		eventlog(eventlog_level_error,__FUNCTION__,"client %d not found",sq_get_clientid(sq));
		sq_destroy(sq);
		return 0;
	}
	if (!(game=gamelist_find_game_by_id(sq_get_gameid(sq)))) {
		eventlog(eventlog_level_error,__FUNCTION__,"game %d not found",sq_get_gameid(sq));
		sq_destroy(sq);
		return 0;
	}
	if (!(gs=game_get_d2gs(game))) {
		eventlog(eventlog_level_error,__FUNCTION__,"try join game without game server set");
		sq_destroy(sq);
		return 0;
	}
	if (!(opacket=sq_get_packet(sq))) {
		eventlog(eventlog_level_error,__FUNCTION__,"previous packet not found (seqno: %d)",seqno);
		sq_destroy(sq);
		return 0;
	}

	result=bn_int_get(packet->u.d2gs_d2cs_joingamereply.result);
	if (result==D2GS_D2CS_JOINGAME_SUCCEED) {
		eventlog(eventlog_level_info,__FUNCTION__,"added %s to game %s on gs %d",d2cs_conn_get_charname(client),
			d2cs_game_get_name(game),conn_get_d2gs_id(c));
		reply=D2CS_CLIENT_JOINGAMEREPLY_SUCCEED;
	} else {
		eventlog(eventlog_level_info,__FUNCTION__,"failed to add %s to game %s on gs %d",d2cs_conn_get_charname(client),
			d2cs_game_get_name(game),conn_get_d2gs_id(c));
		reply=D2CS_CLIENT_JOINGAMEREPLY_FAILED;
	}

	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_joingamereply));
		packet_set_type(rpacket,D2CS_CLIENT_JOINGAMEREPLY);
		bn_short_set(&rpacket->u.d2cs_client_joingamereply.seqno,
				bn_short_get(opacket->u.client_d2cs_joingamereq.seqno));
		bn_short_set(&rpacket->u.d2cs_client_joingamereply.gameid,game_get_d2gs_gameid(game));
		bn_short_set(&rpacket->u.d2cs_client_joingamereply.u1,0);
		bn_int_set(&rpacket->u.d2cs_client_joingamereply.reply,reply);
		if (reply == SERVER_JOINGAMEREPLY2_REPLY_OK) {
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.token,sq_get_gametoken(sq));

			gsaddr = d2gs_get_ip(gs);
			gsport = 4000;
			trans_net(d2cs_conn_get_addr(client), &gsaddr, &gsport);
			
			if(d2gs_get_ip(gs)!=gsaddr)
			{
			    eventlog(eventlog_level_info,__FUNCTION__,"translated gameserver %s -> %s",addr_num_to_ip_str(d2gs_get_ip(gs)),addr_num_to_ip_str(gsaddr));
			} else {
			    eventlog(eventlog_level_info,__FUNCTION__,"no translation required for gamserver %s",addr_num_to_ip_str(gsaddr));
			}
			
			bn_int_nset(&rpacket->u.d2cs_client_joingamereply.addr,gsaddr);

		} else {
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.token,0);
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.addr,0);
		}
		conn_push_outqueue(client,rpacket);
		packet_del_ref(rpacket);
	}
	sq_destroy(sq);
	return 0;
}

static int on_d2gs_updategameinfo(t_connection * c, t_packet * packet)
{
	unsigned int	flag;
	char const	* charname;
	t_game		* game;
	unsigned int	charclass;
	unsigned int	charlevel;
	unsigned int	d2gs_gameid;
	unsigned int	d2gs_id;

	if (!(charname=packet_get_str_const(packet,sizeof(t_d2gs_d2cs_updategameinfo),MAX_CHARNAME_LEN))) {
		eventlog(eventlog_level_error,__FUNCTION__,"got bad charname");
		return 0;
	}
	d2gs_id=conn_get_d2gs_id(c);
	d2gs_gameid=bn_int_get(packet->u.d2gs_d2cs_updategameinfo.gameid);
	charclass=bn_int_get(packet->u.d2gs_d2cs_updategameinfo.charclass);
	charlevel=bn_int_get(packet->u.d2gs_d2cs_updategameinfo.charlevel);
	flag=bn_int_get(packet->u.d2gs_d2cs_updategameinfo.flag);
	if (!(game=gamelist_find_game_by_d2gs_and_id(d2gs_id,d2gs_gameid))) {
		eventlog(eventlog_level_error,__FUNCTION__,"game %d not found on gs %d",d2gs_gameid,d2gs_id);
		return -1;
	}
	if (flag==D2GS_D2CS_UPDATEGAMEINFO_FLAG_ENTER) {
		game_add_character(game,charname,charclass,charlevel);
	} else if (flag==D2GS_D2CS_UPDATEGAMEINFO_FLAG_LEAVE) {
		game_del_character(game,charname);
	} else if (flag==D2GS_D2CS_UPDATEGAMEINFO_FLAG_UPDATE) {
		game_add_character(game,charname,charclass,charlevel);
	} else {
		eventlog(eventlog_level_error,__FUNCTION__,"got bad updategameinfo flag %d",flag);
	}
	return 0;
}

static int on_d2gs_closegame(t_connection * c, t_packet * packet)
{
	t_game	* game;

	if (!(game=gamelist_find_game_by_d2gs_and_id(conn_get_d2gs_id(c),
		bn_int_get(packet->u.d2gs_d2cs_closegame.gameid)))) {
		return 0;
	}
	game_destroy(game);
	return 0;
}

extern int handle_d2gs_packet(t_connection * c, t_packet * packet)
{
	int	type;
	t_d2gs	* gs;
	
	type=packet_get_type(packet);
	
	// ugly d2gs hack, used in backward campability...
	if(type == D2GS_D2CS_SETGSINFO)
	{
		gs=d2gslist_find_gs(conn_get_d2gs_id(c));
		if(gs->d2gs_version <= 0x1090007)
		    d2gs_packet_handle_table[type].size = sizeof(t_d2gs_d2cs_setgsinfo) - sizeof(bn_int);
	}
	
	conn_process_packet(c,packet,d2gs_packet_handle_table,NELEMS(d2gs_packet_handle_table));
	return 0;
}

extern int handle_d2gs_init(t_connection * c)
{
	t_packet	* packet;

	if ((packet=packet_create(packet_class_d2gs))) {
		packet_set_size(packet,sizeof(t_d2cs_d2gs_authreq));
		packet_set_type(packet,D2CS_D2GS_AUTHREQ);
		bn_int_set(&packet->u.d2cs_d2gs_authreq.h.seqno,1);
		bn_int_set(&packet->u.d2cs_d2gs_authreq.sessionnum,d2cs_conn_get_sessionnum(c));
		packet_append_string(packet,prefs_get_realmname());
		conn_push_outqueue(c,packet);
		packet_del_ref(packet);
	}
	eventlog(eventlog_level_info,__FUNCTION__,"sent init packet to d2gs %d (sessionnum=%d)",conn_get_d2gs_id(c),d2cs_conn_get_sessionnum(c));
	return 0;
}
