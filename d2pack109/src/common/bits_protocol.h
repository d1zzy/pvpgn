/*
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999,2000,2001  Marco Ziech (mmz@gmx.net)
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
#ifndef INCLUDED_BITS_PROTOCOL_TYPES
#define INCLUDED_BITS_PROTOCOL_TYPES

#ifdef JUST_NEED_TYPES
# include "common/bn_type.h"
#else
# define JUST_NEED_TYPES
# include "common/bn_type.h"
# undef JUST_NEED_TYPES
#endif

/*
 * The BITS (BNETD Interserver Transport System) protocol is planned to
 * be used to implement server to server communication.
 */

typedef struct
{
       bn_short type;
       bn_short size;
       bn_short src_addr;
       bn_short dst_addr;
       bn_byte	ttl;
       bn_byte	pad; /* for now to pad the header size to 10 byte (maybe for future use) */
} t_bits_header PACKED_ATTR();

typedef struct
{
       t_bits_header   h;
} t_bits_generic PACKED_ATTR();

#define BITS_CURRENT_VERSION           0

#define BITS_MAX_KEYVAL_LEN            1024

#define BITS_ADDR_BCAST                0xFFFF
#define BITS_ADDR_PEER                 0x0000
#define BITS_ADDR_MASTER               0x0001
#define BITS_ADDR_DYNAMIC_START        0x1000
#define BITS_ADDR_DYNAMIC_END          0xE000

#define BITS_DEFAULT_TTL		255	/* The default time to live */
						/* (max number of hops) for a packet */

#define BITS_STATUS_OK                  0x00    /* OK */
#define BITS_STATUS_AUTHFAILED          0x01    /* BITS auth failed */
#define BITS_STATUS_TOOBUSY             0x02    /* Out of memory? */
#define BITS_STATUS_NOAUTH              0x03    /* OK */
#define BITS_STATUS_NOACCT              0x04    /* tell the client to forget the account */
#define BITS_STATUS_CHANNELFULL         0x30    
#define BITS_STATUS_CHANNELDOESNOTEXIST 0x31
#define BITS_STATUS_CHANNELRESTRICTED   0x32
#define BITS_STATUS_AUTHWRONGVERSION    0xFE    /* if the protocol changes ... */
#define BITS_STATUS_FAIL                0xFF    /* unknown error */

#define BITS_PROTOCOLTAG               "BITS"
#define BITS_CLIENTTAG                 "BNTD"

/*****************************************/
/* Must be the first packet from the client to the server.
 * Used to initiate the connection protocol.
 */ 
#define BITS_SESSION_REQUEST              0x0000

typedef struct
{
       t_bits_header   h;
       bn_int          protocoltag;
       bn_int          clienttag;
       /* client software version information */
} t_bits_session_request PACKED_ATTR();

/*****************************************/
/* Must be the first packet from the server to the client.
 */
#define BITS_SESSION_REPLY              0x0001

typedef struct
{
       t_bits_header   h;
       bn_byte         status;
       bn_int          sessionkey;
       bn_int          sessionnum;
} t_bits_session_reply PACKED_ATTR();

/*****************************************/
/* The first packet after the session is initialized. It is
 * used to authenticate the BITS client. 
 */

#define BITS_AUTH_REQUEST              0x0002

typedef struct
{
       t_bits_header   h;
       bn_int          ticks;
       bn_int          passhash[5]; /* the hashed password */
       /* name */
} t_bits_auth_request PACKED_ATTR();

/*****************************************/
/* This packet indicates if the authentication request
 * was successful or not.
 */
#define BITS_AUTH_REPLY                        0x0003

typedef struct
{
       t_bits_header   h;
       bn_byte                 status;         /* see BITS_STATUS_* */
       bn_short                address;        /* new address of this server */
} t_bits_auth_reply PACKED_ATTR();

/*****************************************/

#define BITS_MASTER_AUTH_REQUEST              0x0004

typedef struct
{
       t_bits_header   h;
       bn_int	       qid;
       bn_int          ticks;
       bn_int          sessionkey;
       bn_int	       ip;
       bn_short        port;
       bn_int          passhash[5]; /* the hashed password */
       /* name */
} t_bits_master_auth_request PACKED_ATTR();


/*****************************************/

#define BITS_MASTER_AUTH_REPLY                        0x0005

typedef struct
{
       t_bits_header   h;
       bn_int          qid;            
       bn_byte         status;         /* see BITS_STATUS_* */
       bn_short        address;        /* new address of this server */
} t_bits_master_auth_reply PACKED_ATTR();

/*****************************************/
/* Sent by a client to create an account with the 
 * given name and password.
 */
#define BITS_VA_CREATE_REQ             0x0100

typedef struct
{
       t_bits_header   h;
       bn_int          qid;
       bn_int          passhash1[5];
       /* username  */
} t_bits_va_create_req PACKED_ATTR();

/*****************************************/

#define BITS_VA_CREATE_REPLY   0x0101

typedef struct
{
       t_bits_header   h;
       bn_int          qid;
       bn_byte         status;
       bn_int          uid;
       /* username */
} t_bits_va_create_reply PACKED_ATTR();

/*****************************************/

#define BITS_VA_SET_ATTR               0x0102

typedef struct
{
       t_bits_header   h;
       bn_int                  uid;
       /* attr name  */
       /* attr value */
} t_bits_va_set_attr PACKED_ATTR();
 
/*****************************************/
/* Get a specific attribute of an account from the server.
 * Obsolete?
 */
#define BITS_VA_GET_ATTR               0x0103
typedef struct
{
       t_bits_header   h;
       bn_int                  uid;
       /* attr name  */
} t_bits_va_get_attr PACKED_ATTR();

/*****************************************/

/* This packet is sent on request and on change. */

#define BITS_VA_ATTR                   0x0104

typedef struct
{
       t_bits_header   h;
       bn_byte                 status;
       bn_int                  uid;
       /* attr name  */
       /* attr value */
} t_bits_va_attr PACKED_ATTR();

/*****************************************/

#define BITS_VA_GET_ALLATTR            0x0105
typedef struct
{
       t_bits_header   h;
       bn_int                  uid;
} t_bits_va_get_allattr PACKED_ATTR();

/*****************************************/

/* This packet is only sent on request (BITS_VA_GET_ALLATTR and BITS_VA_LOAD). */

#define BITS_VA_ALLATTR                    0x0106

typedef struct
{
       t_bits_header   h;
       bn_byte                 status;
       bn_int                  uid;
       /* attr_name attr_value */
       /* ... */
} t_bits_va_allattr PACKED_ATTR();

/*****************************************/

#define BITS_VA_LOCK                           0x0107

typedef struct
{
       t_bits_header   h;
       bn_int          qid;
       /* account name or '#'+userid */
} t_bits_va_lock PACKED_ATTR();

/*****************************************/

#define BITS_VA_LOCK_ACK                       0x010B

typedef struct
{
       t_bits_header   h;
       bn_int          qid;
       bn_byte         status;
       bn_int          uid;
       /* account name */
} t_bits_va_lock_ack PACKED_ATTR();

/*****************************************/

#define BITS_VA_UNLOCK                         0x0108

typedef struct
{
       t_bits_header   h;
       bn_int                  uid;
} t_bits_va_unlock PACKED_ATTR();

/*****************************************/
/* Sent by a client to ask the server if the user can
 * log on to the bits network.
 */
#define BITS_VA_LOGINREQ                       0x0109

typedef struct
{
       t_bits_header   h;
       bn_int          qid;            /* query id */
       bn_int          uid;
       bn_short        game_port; /* Ross: swapped int and short */
       bn_int          game_addr;
       bn_int          clienttag;      
} t_bits_va_loginreq PACKED_ATTR();

/*****************************************/
/* Sent by the server to answer the client's login request.
 * This packet is usually followed by a BITS_VA_LOGIN packet.
 */
#define BITS_VA_LOGINREPLY                     0x010A

typedef struct
{
       t_bits_header   h;
       bn_int          qid;           /* query id */
       bn_byte         status;
       bn_int          sessionid;
       /* username */
} t_bits_va_loginreply PACKED_ATTR();

/*****************************************/
/* Broadcasted to all servers by the master server if
 * a user logs off from the bits network.
 */

#define BITS_VA_LOGOUT                         0x010C

typedef struct
{
       t_bits_header   h;
       bn_int          sessionid;
} t_bits_va_logout PACKED_ATTR();
/*****************************************/
/* Broadcasted to all servers by the master server if
 * a user logs on to the bits network.
 */
#define BITS_VA_LOGIN                          0x010D

typedef struct
{
       t_bits_header   h;
       bn_short        host;
       bn_int          uid;
       bn_int          sessionid;
       bn_int          clienttag;
       bn_int          game_addr;
       bn_short        game_port;      
       /* chatname */
       /* playerinfo */
} t_bits_va_login PACKED_ATTR();

/*****************************************/
#define BITS_VA_UPDATE_PLAYERINFO              0x010E

typedef struct
{
       t_bits_header   h;
       bn_int          sessionid;
       /* playerinfo */
} t_bits_va_update_playerinfo PACKED_ATTR();

/*****************************************/
/* used to maintain routing tables */
/* should always be a broadcast */
/* should be send after bits login */
#define BITS_NET_DISCOVER			0x0200

typedef struct
{
	t_bits_header  h;
} t_bits_net_discover PACKED_ATTR();

/*****************************************/
/* used to maintain routing tables */
/* should always be a broadcast */
/* should be send before bits logout */
#define BITS_NET_UNDISCOVER			0x0201

typedef struct
{
	t_bits_header  h;
} t_bits_net_undiscover PACKED_ATTR();

/*****************************************/
#define BITS_NET_PING				0x0202

typedef struct
{
	t_bits_header  h;
	bn_int         qid;
} t_bits_net_ping PACKED_ATTR();

/*****************************************/
#define BITS_NET_PONG				0x0203

typedef struct
{
	t_bits_header  h;
	bn_int         qid;
} t_bits_net_pong PACKED_ATTR();

/*****************************************/
#define BITS_NET_MOTD				0x0204

typedef struct
{
	t_bits_header  h;
	/* motd lines (separated by '\0') */
} t_bits_net_motd PACKED_ATTR();

/*****************************************/
/* Sends a message from the connection with the
 * sessionid <src> to the user with the sessionid <dst>
 * with the text <text>.
 */
#define BITS_CHAT_USER				0x0300

typedef struct
{
	t_bits_header	h;
	bn_int		type;
	bn_int		cflags;		/* srcconn->flags */
	bn_int		clatency;	/* srcconn->latency */
	bn_int		src;	/* source account sessionid */
	bn_int		dst;	/* destination account sessionid*/
	/* text */
} t_bits_chat_user PACKED_ATTR();

/*****************************************/
/* Sent if a channel was added to the global channellist.
 */
#define BITS_CHAT_CHANNELLIST_ADD		0x0301

typedef struct
{
	t_bits_header	h;
	bn_int		channelid;
	/* channelname */
	/* short channelname (optional) */
} t_bits_chat_channellist_add PACKED_ATTR();

/*****************************************/
/* Sent if a channel was removed from the global channellist.
 */
#define BITS_CHAT_CHANNELLIST_DEL		0x0302

typedef struct
{
	t_bits_header	h;
	bn_int		channelid;
} t_bits_chat_channellist_del PACKED_ATTR();

/*****************************************/
/* Sent by the client if a user wants to join an existing
 * channel.
 */
#define BITS_CHAT_CHANNEL_JOIN_REQUEST		0x0303

typedef struct
{
	t_bits_header	h;
	bn_int		channelid;
	bn_int		qid;
	bn_int		sessionid;
	bn_int		flags;
	bn_int		latency;
} t_bits_chat_channel_join_request PACKED_ATTR();

/*****************************************/
/* Sent by the client if a user wants to create a new channel
 * and join it.
 */
#define BITS_CHAT_CHANNEL_JOIN_NEW_REQUEST	0x0304
typedef struct
{
	t_bits_header	h;
	bn_int		qid;
	bn_int		sessionid;
	bn_int		flags;
	bn_int		latency;
	/* new channelname */
} t_bits_chat_channel_join_new_request PACKED_ATTR();

/*****************************************/
/* Sent by the client if a user wants to create a new channel
 * and join it.
 */
#define BITS_CHAT_CHANNEL_JOIN_PERM_REQUEST	0x0305
typedef struct
{
	t_bits_header	h;
	bn_int		qid;
	bn_int		sessionid;
	bn_int		flags;
	bn_int		latency;
	/* channelname */ /* FIXME: Maybe we can reduce the traffic here a bit by replacing this with an id */
	/* country */
} t_bits_chat_channel_join_perm_request PACKED_ATTR();

/*****************************************/
/* The server's  response to one of the two requests above.
 */
#define BITS_CHAT_CHANNEL_JOIN_REPLY		0x0306
typedef struct
{
	t_bits_header	h;
	bn_int		channelid; /* The channelid for the new channel */
	bn_int		qid;       /* same as the qid from the request packet */
	bn_byte		status;
} t_bits_chat_channel_join_reply PACKED_ATTR();

/*****************************************/
/* Sent by the client if a user wants to leave the channel
 * with the given channelid. 
 * Sent by the server to all servers which joined the channel
 * to remove the user from their memberlist.
 */
#define BITS_CHAT_CHANNEL_LEAVE			0x0307

typedef struct
{
	t_bits_header	h;
	bn_int		channelid;
	bn_int		sessionid;
} t_bits_chat_channel_leave PACKED_ATTR();

/*****************************************/
/* Sent by the server to all clients which joined the channel
 * to add a member to their memberlist for the channel with 
 * the given channelid.
 */
#define BITS_CHAT_CHANNEL_JOIN			0x0308

typedef struct
{
	t_bits_header	h;
	bn_int		channelid;
	bn_int		flags;
	bn_int		latency;
	bn_int 		sessionid;
} t_bits_chat_channel_join PACKED_ATTR();

/*****************************************/
/* Sent by the client to join a channel. */
#define BITS_CHAT_CHANNEL_SERVER_JOIN		0x0309

typedef struct
{
	t_bits_header	h;
	bn_int		channelid;
} t_bits_chat_channel_server_join PACKED_ATTR();

/*****************************************/
/* Sent by the client to leave a channel. */
#define BITS_CHAT_CHANNEL_SERVER_LEAVE		0x030A

typedef struct
{
	t_bits_header	h;
	bn_int		channelid;
} t_bits_chat_channel_server_leave PACKED_ATTR();

/*****************************************/
/* Sent to all servers which joined the channel with the given
 * channelid to broadcast a message to all users in the channel.
 */
#define BITS_CHAT_CHANNEL			0x030B

typedef struct
{
	t_bits_header	h;
	bn_int		channelid;
	bn_int		type;
	bn_int		flags;		/* srcconn->flags */
	bn_int		latency;	/* srcconn->latency */
	bn_int		sessionid;	/* srcconn->sessionid */
	/* text */
} t_bits_chat_channel PACKED_ATTR();

/*****************************************/
#define BITS_GAMELIST_ADD			0x0400

typedef struct
{
	t_bits_header	h;
	bn_int		id;		/* unique id (see game->id) */
	bn_int		type;		/* (t_game_type) game->type */
	bn_int		status;		/* the new status */
	bn_int		clienttag;	/* game->clienttag */
	bn_int		owner;		/* game->owner */
	/* name */
	/* password */
	/* info */
} t_bits_gamelist_add PACKED_ATTR();

/*****************************************/
#define BITS_GAMELIST_DEL			0x0401

typedef struct
{
	t_bits_header	h;
	bn_int		id;	/* unique id (see game->id) */
} t_bits_gamelist_del PACKED_ATTR();

/*****************************************/
#define BITS_GAME_UPDATE			0x0402

typedef struct
{
	t_bits_header	h;
	bn_int		id;	/* unique id (see game->id) */
	bn_byte		field;	/* field to update (see BITS_GAME_UPDATE_*) */
	/* value */
} t_bits_game_update PACKED_ATTR();

#define BITS_GAME_UPDATE_STATUS		0x00
#define BITS_GAME_UPDATE_OWNER		0x01
#define BITS_GAME_UPDATE_OPTION		0x02
/*****************************************/
#define BITS_GAME_JOIN_REQUEST			0x0403

typedef struct
{
	t_bits_header	h;
	bn_int		id;	/* unique id (see game->id) */
	bn_int		sessionid; /* the player's sessionid */
	bn_int		version;
} t_bits_game_join_request PACKED_ATTR();

/*****************************************/
#define BITS_GAME_JOIN_REPLY			0x0404

typedef struct
{
	t_bits_header	h;
	bn_int		id;	/* unique id (see game->id) */
	bn_int		sessionid; /* the player's sessionid */
	bn_byte		status;
} t_bits_game_join_reply PACKED_ATTR();

/*****************************************/
#define BITS_GAME_LEAVE				0x0405

typedef struct
{
	t_bits_header	h;
	bn_int		id;	/* unique id (see game->id) */
	bn_int		sessionid; /* the player's sessionid */
} t_bits_game_leave PACKED_ATTR();

/*****************************************/
#define BITS_GAME_CREATE_REQUEST		0x0406

typedef struct
{
	t_bits_header	h;
	bn_int		sessionid;
	bn_int		type; /* game type */
	bn_int		version; /* game version */
	bn_int		option; /* -> t_game_option (STARTGAME4 only) */
	/* gamename */
	/* gamepass */
	/* gameinfo */
} t_bits_game_create_request PACKED_ATTR();

/*****************************************/
#define BITS_GAME_CREATE_REPLY			0x0407

typedef struct
{
	t_bits_header	h;
	bn_int		sessionid;
	bn_int		gameid; /* game id */ 
	bn_int		version; /* game version (from request) */
	bn_byte		status;
} t_bits_game_create_reply PACKED_ATTR();

/*****************************************/
#define BITS_GAME_REPORT			0x0408

typedef struct
{
	t_bits_header	h;
	bn_int		sessionid; /* sessionid to construct a t_rconn */
	bn_int		gameid;	   /* game id */
	bn_int		len;	   /* length of the following data */
	/* the original packet data from the client */
} t_bits_game_report PACKED_ATTR();


#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_PROTOCOL_PROTOS
#define INCLUDED_BITS_PROTOCOL_PROTOS

#endif
#endif
