/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 2000  Onlyer (onlyer@263.net)
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
#ifndef INCLUDED_AUTH_PROTOCOL_TYPES
#define INCLUDED_AUTH_PROTOCOL_TYPES

#ifdef JUST_NEED_TYPES
# include "common/bn_type.h"
#else
# define JUST_NEED_TYPES
# include "common/bn_type.h"
# undef JUST_NEED_TYPES
#endif

/*
 * The "auth" protocol that is used for holding profiles on the server.
 * FIXME: put the #define's into the PROTO section
 */


/******************************************************/
typedef struct
{
    bn_short size;
    bn_byte  type;
} t_auth_header PACKED_ATTR();
/******************************************************/


/******************************************************/
typedef struct
{
    t_auth_header h;
} t_auth_generic PACKED_ATTR();
/******************************************************/


/******************************************************/
/*
SERVER_REALMJOINREPLY CLIENT_AUTHLOGINREQ
01 00 00 00       01 00 00 00         unknown1
FA 9E 0D D1       FA 9E 0D D1         unknown2
D8 94 F6 07       D8 94 F6 07         unknown3
F6 0F 08 00       F6 0F 08 00         sessionkey
D8 94 F6 30                           addr
17 E0                                 port
00 00                                 unknown6
00 00 00 00       00 00 00 00         unknown7       unknown5
42 1B 00 00       42 1B 00 00         unknown8       unknown6
B2 7E 27 44       B2 7E 27 44         unknown9       secret_hash[0]
D5 C5 FB 07       D5 C5 FB 07         secret_hash[0] secret_hash[0]
FA 9E BF 63       FA 9E BF 63         secret_hash[1] secret_hash[0]
1B 57 1B 69       1B 57 1B 69         secret_hash[2] secret_hash[0]
7F 4F A9 C0       7F 4F A9 C0         secret_hash[3] secret_hash[0]
B8 83 E9 4C       B8 83 E9 4C         secret_hash[4] unknown7
4D 6F 4E 6B       4D 6F 4E 6B         charname?
32 6B 00          32 6B 00            charname?


3A 00 01 01 00 00 00 FA   9E 0D D1 D8 94 F6 07 F6    :...............
0F 08 00 00 00 00 00 42   1B 00 00 B2 7E 27 44 D5    .......B....~'D.
C5 FB 07 FA 9E BF 63 1B   57 1B 69 7F 4F A9 C0 B8    ......c.W.i.O...
83 E9 4C 4D 6F 4E 6B 32   6B 00                      ..LMoNk2k.

3A 00 01 01 00 00 00 06   ED 68 67 D8 94 F6 07 89    :........hg.....
6A 21 00 00 00 00 00 91   20 00 00 62 D5 F7 E4 E4    j!...... ..b....
B1 DF C4 A5 1C 65 BD 67   98 17 89 2C 1F 19 53 24    .....e.g...,..S$
D0 08 2F 4E 6F 6D 61 64   58 00                      ../NomadX.

From Diablo II 1.03 to bnetd-0.4.23pre18
      3B 00 01 04 00 00   00 FA 9E 0D D1 D8 94 F6    +.;............. 
07 05 79 73 52 00 00 00   00 42 1B 00 00 69 98 3C    ..ysR....B...i.< 
64 E3 11 3E A5 02 E8 27   89 09 F8 97 E8 AF EE 5F    d..>...'......._ 
30 CC 72 CF 4F 45 6C 66   6C 6F 72 64 00             0.r.OElflord.
*/
#define CLIENT_AUTHLOGINREQ 0x01
typedef struct
{
    t_auth_header h;
    bn_int        unknown1; /* seq number? */ /* from 0x35ff packet */
    bn_int        unknown2; /* from 0x35ff packet */
    bn_int        unknown3; /* reg auth? */ /* from 0x35ff packet */
    bn_int        sessionkey; /* from 0x35ff packet */
    bn_int        unknown7; /* always zero? */ /* from 0x35ff packet */
    bn_int        unknown8; /* always near 0x2000? */
    bn_int        unknown9; /* hash salt? */ /* from 0x35ff packet */
    bn_int        secret_hash[5]; /* from 0x35ff packet */
    /* player name */ /* _not_ character name! */
} t_client_authloginreq PACKED_ATTR();
/******************************************************/


/******************************************************/
/*
07 00 01 00 00 00 00                                 .......

07 00 01 0c 00 00 00                                 .......
*/
#define SERVER_AUTHLOGINREPLY 0x01
typedef struct
{
    t_auth_header h;
    bn_int        reply;
} t_server_authloginreply PACKED_ATTR();
#define SERVER_AUTHLOGINREPLY_REPLY_SUCCESS  0x00000000
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED   0x00000001 /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_NOCONN   0x00000002 /* D2DV Beta 1.02 interprets as "Can't connect to server" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED2  0x00000003 /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED3  0x00000004 /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED4  0x00000005 /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED5  0x00000006 /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED6  0x00000007 /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED7  0x00000008 /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED8  0x00000009 /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, try again later" */
#define SERVER_AUTHLOGINREPLY_REPLY_NOAUTH   0x0000000a /* D2DV Beta 1.02 interprets as "Authorization Failed" */
#define SERVER_AUTHLOGINREPLY_REPLY_NOAUTH2  0x0000000b /* D2DV Beta 1.02 interprets as "Authorization Failed" */
#define SERVER_AUTHLOGINREPLY_REPLY_NOAUTH3  0x0000000c /* D2DV Beta 1.02 interprets as "Authorization Failed" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED9  0x0000000d /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED10 0x0000000e /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
#define SERVER_AUTHLOGINREPLY_REPLY_BANNED11 0x0000000f /* D2DV Beta 1.02 interprets as "You are banned from Battle.net, reconnect later" */
/******************************************************/


/******************************************************/
/*
0f 00 02 01 00 00 00 00   00 42 4e 45 54 44 00       .........BNETD.

0d 00 02 00 00 00 00 00   00 61 73 64 00             .........asd.

0e 00 02 00 00 00 00 00   00 71 77 65 72 00          .........qwer.

0e 00 02 04 00 00 00 00   00 51 6c 65 78 00          .........Qlex.

0f 00 02 02 00 00 00 00   00 42 65 6e 6e 79 00       .........Benny.

11 00 02 02 00 00 00 00   00 54 68 6f 72 73 65 6e    .........Thorsen
00                                                   .

12 00 02 03 00 00 00 00   00 51 6c 65 78 54 45 53    .........QlexTES
54 00                                                T.

12 00 02 01 00 00 00 00   00 51 6c 65 78 54 45 53    .........QlexTES
54 00                                                T.

10 00 02 01 00 00 00 00   00 53 68 61 6d 61 72 00    .........Shamar.

0e 00 02 01 00 00 00 00   00 4b 75 72 74 00          .........Kurt.

0e 00 02 01 00 00 00 00   00 42 65 6e 74 00          .........Bent.

From Diablo II 1.03 - amazon
11 00 02 00 00 00   00 00 00 47 6f 64 64 65     .........Godde 
73 73 00                                            ss.

From Diablo II : LoD 1.08 - Amazon
10 00 02 00 00 00 00 20   00 61 6D 61 45 78 70 00    ....... .amaExp.
10 00 02 00 00 00 00 00   00 61 6D 61 4F 6C 64 00    .........amaOld.

From Diablo II : LoD 1.08 - Paladin
10 00 02 03 00 00 00 00   00 70 61 6C 4F 6C 64 00    .........palOld.
10 00 02 03 00 00 00 20   00 70 61 6C 45 78 70 00    ....... .palExp.

hardcore char create
                  13 00   02 01 00 00 00 04 00 6F          .........o
6E 6C 79 65 72 2D 74 68   00                         nlyer-th.
*/
#define CLIENT_CREATECHARREQ 0x02
typedef struct
{
    t_auth_header h;
    bn_short      class;
    bn_short      unknown1; /* always zero? */
    bn_short      expansion; /* FIXME: according to Onlyer this is "type" 0x0=Normal 0x4=hardcore ,the same as in .d2s file */
    /* character name */
} t_client_createcharreq PACKED_ATTR();
#define CLIENT_CREATECHARREQ_CLASS_AMAZON      0x0000
#define CLIENT_CREATECHARREQ_CLASS_SORCERESS   0x0001
#define CLIENT_CREATECHARREQ_CLASS_NECROMANCER 0x0002
#define CLIENT_CREATECHARREQ_CLASS_PALADIN     0x0003
#define CLIENT_CREATECHARREQ_CLASS_BARBARIAN   0x0004
#define CLIENT_CREATECHARREQ_CLASS_DRUID       0x0005
#define CLIENT_CREATECHARREQ_CLASS_ASSASSIN    0x0006
#define CLIENT_CREATECHARREQ_UNKNOWN1          0x0000
#define CLIENT_CREATECHARREQ_EXPANSION_CLASSIC 0x0000
#define CLIENT_CREATECHARREQ_EXPANSION_LOD     0x0020

/******************************************************/


/******************************************************/
/*
07 00 02 00 00 00 00                                 .......

07 00 02 14 00 00 00                                 .......
*/
#define SERVER_CREATECHARREPLY 0x02
typedef struct
{
    t_auth_header h;
    bn_int        reply;
} t_server_createcharreply PACKED_ATTR();
#define SERVER_CREATECHARREPLY_REPLY_SUCCESS  0x00000000
/* FIXME: play with these... what error message does this produce? */
#define SERVER_CREATECHARREPLY_REPLY_REJECTED 0x00000014 /* Name %s rejected by server */
#define SERVER_CREATECHARREPLY_REPLY_ERROR    0x00000001 /* from Onlyer */
#define SERVER_CREATECHARREPLY_REPLY_NOTFOUND 0x00000046 /* from Onlyer */
/******************************************************/


/******************************************************/
/*
17 00 03 1A 00 00 10 00   00 01 FF 08 46 64 67 00    ............Fdg.
44 67 00 64 66 67 00                                 Dg.dfg.

13 00 03 1B 00 00 10 00   00 01 FF 08 42 6C 6F 70    ............Blop
00 00 00                                             ...

13 00 03 02 00 00 10 00   00 01 FF 08 54 65 73 74    ............Test
00 00 00                                             ...

1F 00 03 02 00 00 10 00   00 01 05 08 53 74 66 75    ............Stfu
00 42 75 73 74 61 00 68   65 79 20 66 30 30 00       .Busta.hey f00.

33 00 03 02 00 00 10 00   00 01 FF 08 47 61 6D 65    3...........Game
6E 61 6D 65 00 50 61 73   73 77 6F 72 64 66 69 65    name.Passwordfie
6C 64 00 47 61 6D 65 44   65 73 63 72 69 70 74 69    ld.GameDescripti
6F 6E 00                                             on.

36 00 03 02 00 00 10 00   00 01 07 06 47 61 6D 65    6...........Game
6E 61 6D 65 00 50 61 73   73 77 6F 72 64 00 64 65    name.Password.de
73 63 72 69 70 74 69 6F   6E 36 70 6C 37 64 69 66    scription6pl7dif
66 6E 6F 72 6D 00                                    fnorm.

Onlyer: normal?
                  32 00   03 02 00 00 00 00 00 01          2.........
62 05 47 61 6D 65 6E 61   6D 65 00 50 61 73 73 77    b.Gamename.Passw
6F 72 64 00 74 68 69 73   20 69 73 20 61 20 74 65    ord.this.is.a.te
73 74 20 67 61 6D 65 00                              st.game.

Onlyer: hell game: test,8 level diff,4 maxchar
                  13 00   03 06 00 00 20 00 00 01          ..........
08 04 54 65 73 74 00 00   00                         ..Test...

Onlyer: nightmare game: test
                  13 00   03 08 00 00 10 00 00 01          ..........
08 04 54 65 73 74 00 00   00                         ..Test...
*/
#define CLIENT_CREATEGAMEREQ 0x03
typedef struct
{
    t_auth_header h;
    bn_short      unknown1;   /* 02 00, 1b 00, 1a 00 */ /* sequence no? */
    bn_byte       unknown2;   /* always zero? */
    bn_byte       difficulty; /* normal, nightmare, hell */
    bn_short      unknown3;   /* always zero?                             */
    bn_byte       unknown4;   /* always one?                              */
    bn_byte       leveldiff;  /* Only allow people of +/- n level to join */
    bn_byte       maxchar;    /* Maximum number of chars allowed in game  */
    /* game name */
    /* game pass */
    /* game desc */
} t_client_creategamereq PACKED_ATTR();
#define CLIENT_CREATEGAMEREQ_UNKNOWN1             0x0200
#define CLIENT_CREATEGAMEREQ_UNKNOWN2             0x00
#define CLIENT_CREATEGAMEREQ_UNKNOWN3             0x0000
#define CLIENT_CREATEGAMEREQ_UNKNOWN4             0x01
#define CLIENT_CREATEGAMEREQ_DIFFICULTY_NORMAL    0x00
#define CLIENT_CREATEGAMEREQ_DIFFICULTY_NIGHTMARE 0x10
#define CLIENT_CREATEGAMEREQ_DIFFICULTY_HELL      0x20
#define CLIENT_CREATEGAMEREQ_LEVELDIFF_ANY        0xff
/******************************************************/


/******************************************************/
/*
0D 00 03 02 00 00 00 00   00 1F 00 00 00             .............

0D 00 03 02 00 41 23 00   00 00 00 00 00             .....A#......

Onlyer: from FSGS: create ok, join failed
                  0D 00   03 0C 00 53 00 00 00 00          .....S....
00 00 00  

Onlyer: game already exists with same name
                  0D 00   03 10 00 00 00 00 00 1F          ..........
00 00 00                                             ...

Onlyer: success
                  0D 00   03 1C 00 56 00 00 00 00          .....V....
00 00 00                                             ...

Onlyer: no new game can be created now
                  0D 00   03 2C 00 00 00 00 00 20          ...,......
00 00 00                                             ...
*/
#define SERVER_CREATEGAMEREPLY 0x03
typedef struct
{
    t_auth_header h;
    bn_short      unknown1; /* sequence no? same as client? */
    bn_short      unknown2;
    bn_short      unknown3; /* token? */
    bn_int        reply;
} t_server_creategamereply PACKED_ATTR();
#define SERVER_CREATEGAMEREPLY_UNKNOWN2 0x0000
#define SERVER_CREATEGAMEREPLY_UNKNOWN3 0x1234
#define SERVER_CREATEGAMEREPLY_REPLY_OK           0x00000000
#define SERVER_CREATEGAMEREPLY_REPLY_ERROR        0x00000001
#define SERVER_CREATEGAMEREPLY_REPLY_INVALID_NAME 0x0000001e
#define SERVER_CREATEGAMEREPLY_REPLY_NAME_EXIST   0x0000001f /* game exists */
#define SERVER_CREATEGAMEREPLY_REPLY_DISABLE      0x00000020 /* server full? */
/******************************************************/


/******************************************************/
/*
10 00 04 03 00 53 74 66   75 00 42 75 73 74 61 00    .....Stfu.Busta.

Onlyer:
                  17 00   04 03 00 47 61 6D 65 6E          .....Gamen
61 6D 65 00 50 61 73 73   77 6F 72 64 00             ame.Password.
*/
#define CLIENT_JOINGAMEREQ2 0x04
typedef struct
{
    t_auth_header h;
    bn_short      unknown1; /* sequence number? */
    /* game name */
    /* game pass */
} t_client_joingamereq2 PACKED_ATTR();
/******************************************************/


/******************************************************/
/*
15 00 04 03 00 CE 00 00   00 D8 94 F6 34 AF F9 07    ............4...
10 00 00 00 00                                       .....

Onlyer: game does not exist
                  15 00   04 0B 00 00 00 00 00 00          ..........
00 00 00 00 00 00 00 2A   00 00 00                   .......*...

Onlyer: join ok (0x56 is from creategame)
                  15 00   04 1D 00 00 00 00 00 C3          ..........
8F 81 73 56 00 00 00 00   00 00 00                   ..sV.......

*this is later in game server to check the sessionnumber
*
*srv                  97 00  00 00 00 00               "8..........
*
*cli                  61 56 00 00 00 00 00 01 03 00   Da.3..aV........
*    00 00 6F 6E 6C 79 65 72 2D 63 68 6E 00 00 24 E0   ..onlyer-chn..$.
*    B8 05                                              ..
*/
#define SERVER_JOINGAMEREPLY2 0x04
typedef struct
{
    t_auth_header h;
    bn_short      unknown1; /* sequence number? same as reqest */
    bn_int        unknown2;
    bn_int        addr; /* game server IP (port is 4000) */ /* 216 148 246 52 */
    bn_int        unknown4; /* token? */
    bn_int        reply;
} t_server_joingamereply2 PACKED_ATTR();
#define SERVER_JOINGAMEREPLY2_UNKNOWN2 0x000003d2
#define SERVER_JOINGAMEREPLY2_UNKNOWN4 0x1007f9af
#define SERVER_JOINGAMEREPLY2_REPLY_OK          0x00000000
#define SERVER_JOINGAMEREPLY2_REPLY_ERROR       0x00000001
#define SERVER_JOINGAMEREPLY2_REPLY_PASS_ERROR  0x00000029
#define SERVER_JOINGAMEREPLY2_REPLY_NOT_EXIST   0x0000002a
#define SERVER_JOINGAMEREPLY2_REPLY_GAME_FULL   0x0000002b
#define SERVER_JOINGAMEREPLY2_REPLY_LEVEL_LIMIT 0x0000002c
/******************************************************/


/******************************************************/
/*
FF 88 93 7B 00 00 07 00  14 16 01 00 00            ...{.........
*/
#define SERVER_CREATEGAME_WAIT 0x14
typedef struct
{
    t_auth_header h;
    bn_int       position; /* position of in line to create a game */
} t_server_creategame_wait;
/******************************************************/


/******************************************************/
#define CLIENT_CANCEL_CREATE 0x13
typedef struct
{
    t_auth_header h;
} t_client_cancel_create;
/******************************************************/


/******************************************************/
/*
0A 00 05 05 00 00 00 00   00 00                      ..........

0A 00 05 06 00 00 00 00   00 00                      ..........
*/
#define CLIENT_D2GAMELISTREQ 0x05
typedef struct
{
    t_auth_header h;
    bn_short      unknown1; /* sequence number? */
    bn_int        unknown2; /* always zero */
    bn_byte       unknown3; /* password? filter? always zero? */
} t_client_d2gamelistreq PACKED_ATTR();
/******************************************************/


/******************************************************/
/*
37 00 05 05 00 A8 32 00   00 01 04 10 00 00 49 6E    7.....2.......In
65 65 64 61 6D 75 6C 65   74 00 6F 66 66 65 72 69    eedamulet.offeri
6E 67 20 67 6F 6C 64 2F   72 61 72 65 2F 73 65 74    ng gold/rare/set
20 69 74 65 6D 73 00 16   00 05 05 00 A4 32 00 00     items.......2..
01 04 10 00 00 50 6F 6F   70 6F 6F 00 00 17 00 05    .....Poopoo.....
05 00 9F 32 00 00 01 04   10 00 00 42 61 6C 6C 65    ...2.......Balle
72 61 00 00 1A 00 05 05   00 9C 32 00 00 01 04 10    ra........2.....
00 00 41 65 72 6F 6E 20   54 65 73 74 00 00 13 00    ..Aeron Test....
05 05 00 9B 32 00 00 02   04 10 00 00 46 75 6E 00    ....2.......Fun.
00 18 00 05 05 00 98 32   00 00 01 04 10 00 00 4F    .......2.......O
70 65 6E 00 6F 70 65 6E   00 1C 00 05 05 00 86 32    pen.open.......2
00 00 03 04 10 00 00 4a   75 73 74 20 50 6c 61 79    .......Just Play
69 6E 67 00 00 16 00 05   05 00 85 32 00 00 01 04    ing........2....
10 00 00 59 61 68 6F 6F   6F 00 00 29 00 05 05 00    ...Yahooo..)....
82 32 00 00 03 04 10 00   00 4C 65 74 27 73 20 4B    .2.......Let's K
69 63 6B 20 41 7A 65 00   41 6C 6C 20 57 65 6C 63    ick Aze.All Welc
6F 6D 65 00 1C 00 05 05   00 80 32 00 00 03 04 10    ome.......2.....
00 00 44 32 69 00 4A 6F   69 6E 20 61 6C 6C 21 00    ..D2i.Join all!.
1B 00 05 05 00 7D 32 00   00 07 04 10 00 00 54 72    .....}2.......Tr
61 64 65 73 20 52 20 55   73 00 00                   ades R Us..

17 00 05 05 00 5E 32 00   00 03 04 10 00 00 48 75    .....^2.......Hu
6E 74 65 72 73 00 00                                 nters..

                          11 00 05 06 00 00 00 00            ........
00 02 00 00 00 00 00 00   00                         .........

Onlyer:

no new game can create

                  10 00   05 24 00 00 00 00 00 00          ...$......
FF FF FF FF 00 00                                    ......


                  10 00   05 25 00 00 00 00 00 74          ...%.....t
FF FF FF FF 00 00                                    ......

*/
#define SERVER_D2GAMELISTREPLY 0x05
typedef struct
{
    t_auth_header h;
    bn_short      unknown1; /* sequence no? same as request */
    bn_int        unknown2; /* token? A8 32 00 00 */
    bn_byte       unknown3; /* currchar? */
    bn_int        unknown4; /* 04 10 00 00  or  00 00 00 00 ??? */
    /* game name */
    /* game info */
    /* empty for END? */
} t_server_d2gamelistreply PACKED_ATTR();
#define SERVER_D2GAMELISTREPLY_UNKNOWN2     0x000032a8
#define SERVER_D2GAMELISTREPLY_UNKNOWN3     0x00000001
#define SERVER_D2GAMELISTREPLY_UNKNOWN4     0x00001004
#define SERVER_D2GAMELISTREPLY_UNKNOWN4_END 0x00000000
/******************************************************/


/******************************************************/
/*
0A 00 06 55 00 4E 6F 70   65 00                      ...U.Nope.

09 00 06 34 00 4B 6F 62   00                         ...4.Kob.
*/
#define CLIENT_GAMEINFOREQ 0x06
typedef struct
{
    t_auth_header h;
    bn_short      unknown1; /* sequence no? */
    /* game name */
} t_client_gameinforeq PACKED_ATTR();
/******************************************************/


/******************************************************/
/*
38 00 06 04 00 04 10 00   00 02 03 04 CC CC CC CC    8...............
CC CC CC CC CC CC CC CC   CC CC 11 08 CC CC CC CC    ................
CC CC CC CC CC CC CC CC   CC CC 00 54 55 52 42 4F    ...........TURBO
00 44 61 50 69 6D 70 00                              .DaPimp.

58 00 06 66 00 04 10 00   00 06 04 04 00 01 03 03    X..f............
CC CC CC CC CC CC CC CC   CC CC 10 08 13 10 14 0D    ................
CC CC CC CC CC CC CC CC   CC CC 00 50 79 6E 65 4B    ...........PyneK
6E 6F 74 00 44 61 50 69   6D 70 00 43 61 6C 6C 65    not.DaPimp.Calle
73 74 6F 00 42 6C 61 69   72 00 4C 79 6F 72 61 6E    sto.Blair.Lyoran
00 53 4F 50 48 41 4C 00                              .SOPHAL.

Onlyer: server game details reply

                  3A 00   06 0C 00 5C 00 00 00 00         :....\....
00 00 00 01 04 04 01 03   00 00 00 00 00 00 00 00   ................
00 00 00 00 00 00 00 01   00 00 00 00 00 00 00 00   ................
00 00 00 00 00 00 00 00   47 6F 6C 64 6D 61 6E 00   ........Goldman.
etime 0:0:0, level 1-5, up to 4 players, Goldman level 1 paladin
*/
#define SERVER_GAMEINFOREPLY 0x06
typedef struct
{
    t_auth_header h;

    bn_short      unknown1; /* sequence no? same as client */
    bn_int        unknown2; /* token? */
    bn_int        unknown3; /* etime? */
    bn_byte       gamelevel;
    bn_byte       leveldiff;
    bn_byte       maxchar;
    bn_byte       currchar;
    bn_byte       class[16];
    bn_byte       level[16];
    bn_byte       unknown4;
    /* characters */
} t_server_gameinforeply PACKED_ATTR();
#define SERVER_GAMEINFOREPLY_UNKNOWN2 0x00100400
#define SERVER_GAMEINFOREPLY_UNKNOWN3 0x00100400
#define SERVER_GAMEINFOREPLY_UNKNOWN2 0x00100400
#define SERVER_GAMEINFOREPLY_UNKNOWN4 0x00
/******************************************************/


/******************************************************/
/*
0B 00 07 4D 6F 4E 6B 2D   65 65 00                   ...MoNk-ee.

07 00 07 61 6D 61 00                                 ...ama.         
*/
#define CLIENT_CHARLOGINREQ 0x07
typedef struct
{
    t_auth_header h;
    /* character name */
} t_client_charloginreq PACKED_ATTR();
/******************************************************/


/******************************************************/
/*
07 00 07 00 00 00 00                                 .......
*/
#define SERVER_CHARLOGINREPLY 0x07
typedef struct
{
    t_auth_header h;
    bn_int        reply;
} t_server_charloginreply PACKED_ATTR();
#define SERVER_CHARLOGINREPLY_REPLY_SUCCESS 0x00000000
#define SERVER_CHARLOGINREPLY_ERROR         0x00000001  /* need disconnect client */
/* FIXME: document error codes */
/******************************************************/


/******************************************************/
/*
05 00 0A 00 00                                       .....
*/
#define CLIENT_DELETECHARREQ 0x0a
typedef struct
{
    t_auth_header h;
    bn_short      unknown1; /* sequence no? */ /* FIXME: Onlyer doesn't have this */
} t_client_deletecharreq PACKED_ATTR();
/******************************************************/


/******************************************************/
/*
09 00 0A 00 00 00 00 00   00                         .........
*/
#define SERVER_DELETECHARREPLY 0x0a
typedef struct
{
    t_auth_header h;
    bn_short      unknown1; /* sequence no? same as client */ /* FIXME: Onlyer doesn't have this */
    bn_int        reply;
} t_server_deletecharreply PACKED_ATTR();
#define SERVER_DELETECHARREPLY_REPLY_SUCCESS 0x00000000
#define SERVER_DELETECHARREPLY_REPLY_ERROR   0x00000001
/* FIXME: document error codes */
/******************************************************/


/******************************************************/
/*
Hardcore, overall
04 00 11 00                                          ....      

Hardcore, by class
04 00 11 01                                          ....
04 00 11 02                                          ....
04 00 11 03                                          ....
04 00 11 04                                          ....
04 00 11 05                                          ....

Standard, overall
04 00 11 09                                          ....

Standard, by class
04 00 11 0A                                          ....
04 00 11 0B                                          ....
04 00 11 0C                                          ....
04 00 11 0D                                          ....
04 00 11 0E                                          ....
*/
#define CLIENT_LADDERREQ2 0x11
typedef struct
{
    t_auth_header h;
    bn_byte       class;
} t_client_ladderreq2 PACKED_ATTR();
#define CLIENT_LADDERREQ2_CLASS_HARDCORE_OVERALL     0x00
#define CLIENT_LADDERREQ2_CLASS_HARDCORE_AMAZON      0x01
#define CLIENT_LADDERREQ2_CLASS_HARDCORE_SORCERESS   0x02
#define CLIENT_LADDERREQ2_CLASS_HARDCORE_NECROMANCER 0x03
#define CLIENT_LADDERREQ2_CLASS_HARDCORE_PALADIN     0x04
#define CLIENT_LADDERREQ2_CLASS_HARDCORE_BARBARIAN   0x05
#define CLIENT_LADDERREQ2_CLASS_STANDARD_OVERALL     0x09
#define CLIENT_LADDERREQ2_CLASS_STANDARD_AMAZON      0x0a
#define CLIENT_LADDERREQ2_CLASS_STANDARD_SORCERESS   0x0b
#define CLIENT_LADDERREQ2_CLASS_STANDARD_NECROMANCER 0x0c
#define CLIENT_LADDERREQ2_CLASS_STANDARD_PALADIN     0x0d
#define CLIENT_LADDERREQ2_CLASS_STANDARD_BARBARIAN   0x0e
/******************************************************/


/******************************************************/
/*
9A 01 11 09 80 05 90 01   00 00 32 00 00 00 10 00    ..........2.....
00 00 92 A6 15 00 00 00   00 00 01 01 18 00 53 68    ..............Sh
61 6E 69 71 75 61 00 00   00 00 00 00 00 00 09 C8    aniqua..........
14 00 00 00 00 00 00 01   18 00 53 68 61 79 6C 61    ..........Shayla
5F 53 68 61 79 6C 61 00   00 00 24 3B 14 00 00 00    _Shayla...$;....
00 00 03 01 18 00 43 53   2D 52 55 49 4E 45 52 00    ......CS-RUINER.
00 00 00 00 00 00 E8 8B   13 00 00 00 00 00 00 01    ................
17 00 42 61 62 79 43 75   70 00 00 00 00 00 00 00    ..BabyCup.......
00 00 18 7A 13 00 00 00   00 00 03 01 17 00 54 72    ...z..........Tr
75 65 2D 42 6C 6F 6F 64   00 00 00 00 00 00 56 30    ue-Blood......V0
13 00 00 00 00 00 00 01   17 00 44 61 72 6B 53 6E    ..........DarkSn
69 70 65 72 00 00 00 00   00 00 6C F4 11 00 00 00    iper......l.....
00 00 03 01 17 00 45 6C   64 72 69 63 00 00 00 00    ......Eldric....
00 00 00 00 00 00 FE B7   11 00 00 00 00 00 03 01    ................
17 00 47 75 6E 6E 65 72   00 00 00 00 00 00 00 00    ..Gunner........
00 00 60 E0 10 00 00 00   00 00 04 01 17 00 48 69    ..`...........Hi
65 6E 69 6B 65 6E 00 00   00 00 00 00 00 00 34 A0    eniken........4.
10 00 00 00 00 00 01 01   17 00 52 61 73 68 65 6C    ..........Rashel
00 00 00 00 00 00 00 00   00 00 45 51 10 00 00 00    ..........EQ....
00 00 03 01 17 00 41 65   67 69 73 00 00 00 00 00    ......Aegis.....
00 00 00 00 00 00 F6 04   10 00 00 00 00 00 00 01    ................
17 00 4E 6F 73 00 00 00   00 00 00 00 00 00 00 00    ..Nos...........
00 00 0F 64 0F 00 00 00   00 00 03 01 16 00 43 72    ...d..........Cr
6F 6D 65 5F 44 6F 6D 65   00 00 00 00 00 00 6A 24    ome_Dome......j$
0F 00 00 00 00 00 01 01   16 00 52 65 6E 65 65 00    ..........Renee.
00 00 00 00 00 00 00 00   00 00                      ..........

Onlyer:
1.standard - by class
0A- class ama
0B- class sor
0C- class nec
0D- class pal
0E- class bar

                  2A 01  11 0A 20 01 20 01 00 00         *.........
0A 00 00 00 10 00 00 00  BC 13 BD 60 00 00 00 00   ...........`....
00 0C 5A 00 53 68 61 76  65 6E 61 00 00 00 00 00   ..Z.Shavena.....
00 00 00 00 74 7B B5 60  00 00 00 00 00 0B 5A 00   ....t{.`......Z.
4E 69 63 6F 6C 65 5F 41  70 70 6C 65 74 6F 6E 00   Nicole_Appleton.
E5 44 02 50 00 00 00 00  00 0B 57 00 43 61 72 79   .D.P......W.Cary
00 00 00 00 00 00 00 00  00 00 00 00 B0 39 BF 4E   .............9.N
00 00 00 00 00 0C 57 00  41 6C 63 79 6E 6F 65 00   ......W.Alcynoe.
00 00 00 00 00 00 00 00  B4 A7 F2 4A 00 00 00 00   ...........J....
00 0B 57 00 41 6C 61 65  73 69 61 00 00 00 00 00   ..W.Alaesia.....
00 00 00 00 A3 49 91 4A  00 00 00 00 00 0C 57 00   .....I.J......W.
76 61 6C 6B 65 72 69 65  00 00 00 00 00 00 00 00   valkerie........
83 CE F5 47 00 00 00 00  00 0C 56 00 76 61 6E 64   ...G......V.vand
65 6E 62 65 72 67 00 00  00 00 00 00 7C 52 D6 47   enberg......|R.G
00 00 00 00 00 0B 56 00  50 69 77 69 2D 57 75 72   ......V.Piwi-Wur
73 74 00 00 00 00 00 00  6E 5F 64 46 00 00 00 00   st......n_dF....
00 0C 56 00 4B 69 65 66  65 72 48 75 62 5F 41 00   ..V.KieferHub_A.
00 00 00 00 F9 55 73 45  00 00 00 00 00 0C 56 00   .....UsE......V.
74 61 61 72 6E 61 41 4D  00 00 00 00 00 00 00 00   taarnaAM........

for by class-each class shows 10 chars
for by overall-altogether showes 50 chars
each page shows about 14 chars

hc-class header is   20 01 20 01 00 00 0A 00 00 00 10 00 00 00
hc-overall header is 80 05 90 01 00 00 32 00 00 00 10 00 00 00

std-class is the same as hc-class
std-overall the same as hc-overall
*/
#define SERVER_LADDERREPLY2 0x11
typedef struct
{
    t_auth_header h;
    bn_byte       unknown1; /* class? rank? sequence no? */
    bn_short      unknown2; /* total_len? */
    bn_short      unknown3; /* len? */
    bn_short      unknown4; /* cont_len? */   /* FIXME: sort these out */
    bn_int        unknown5; /* cont_len? */
    bn_int        unknown6; /* count? */
} t_server_ladderreply2 PACKED_ATTR();
#define SERVER_LADDERREPLY2_UNKNOWN2 0x0000
#define SERVER_LADDERREPLY2_UNKNOWN3 0x0000
#define SERVER_LADDERREPLY2_UNKNOWN4 0x0000
#define SERVER_LADDERREPLY2_UNKNOWN5 0x00000000
#define SERVER_LADDERREPLY2_UNKNOWN6 0x00000000

typedef struct
{
    bn_int  exp;
    bn_int  unknown1; /* always zero? */
    bn_byte class; 
    bn_byte title; 
    bn_byte level; 
    bn_byte unknown2; 
    /* character name? */ /* or char charname[16]; */  /* FIXME */
} t_server_ladderreply2_entry PACKED_ATTR();
#define SERVER_LADDERREPLY2_ENTRY_UNKNOWN1 0x00000000
#define SERVER_LADDERREPLY2_ENTRY_UNKNOWN2 0x00
/******************************************************/

/******************************************************/
/* Diablo II 1.05 */
/* 0x0000: 07 00 17 08 00 00 00 */
/* Diablo II LOD 1.08 */
/* 0x0000: 07 00 17 08 00 00 00 */
#define CLIENT_CHARLISTREQ 0x17
typedef struct /* FIXME: This seems to be a request for the character list */
{
    t_auth_header h;
    bn_short      unknown1;    /* FIXME: Possibly max number of characters? */
    bn_short      unknown2;
} t_client_charlistreq PACKED_ATTR();
/******************************************************/


/******************************************************/
/* Diablo II 1.08 LOD */
/* 
 * 0x0000: 0B 00 17 08 00 00 00 00   00 00 00
 */

/*
 * 0000:   3A 00 17 08 00 01 00 00   00 01 00 68 61 6B 61 6E    :..........hakan
 * 0010:   41 73 73 54 65 6D 70 00   84 80 FF FF FF FF FF FF    AssTemp.........
 * 0020:   FF FF FF FF FF 07 FF FF   FF FF FF FF FF FF FF FF    ................
 * 0030:   FF 01 A1 80 80 80 FF FF   FF 00     
 */

/*
 * 0000:   98 00 17 08 00 03 00 00   00 03 00 68 61 6B 61 6E    ...........hakan
 * 0010:   41 73 73 54 65 6D 70 00   84 80 FF FF FF FF FF FF    AssTemp.........
 * 0020:   FF FF FF FF FF 07 FF FF   FF FF FF FF FF FF FF FF    ................
 * 0030:   FF 01 A1 80 80 80 FF FF   FF 00 68 61 6B 61 6E 44    ..........hakanD
 * 0040:   72 75 54 65 6D 70 00 84   80 FF FF FF FF FF FF FF    ruTemp..........
 * 0050:   FF FF FF FF 06 FF FF FF   FF FF FF FF FF FF FF FF    ................
 * 0060:   01 A1 80 80 80 FF FF FF   00 68 61 6B 61 6E 50 61    .........hakanPa
 * 0070:   6C 4F 6C 5F 64 00 84 80   FF FF FF FF FF FF FF FF    lOl_d...........
 * 0080:   FF FF FF 04 FF FF FF FF   FF FF FF FF FF FF FF 01    ................
 * 0090:   81 80 80 80 FF FF FF 00                              ........        
 */
#define SERVER_CHARLISTREPLY 0x17
typedef struct
{
    t_auth_header h;
    bn_short      unknown1;  /* FIXME: This seems to be the value of unknown1 from the CHARLISTREQ */
    bn_short      nchars_1;
    bn_short      unknown2;  /* This seems to be always 00 00 */
    bn_short      nchars_2;  /* FIXME: Why is here the number of characters for a second time? */

    /* num_chars times character blocks */

} t_server_charlistreply PACKED_ATTR();
/******************************************************/


/******************************************************/
#define CLIENT_CONVERTCHARREQ 0x18
/* Diablo II LoD 1.08 */
/* 0000:   0C 00 18 4E 65 63 72 6F   58 58 58 00                ...NecroXXX.    */
typedef struct
{
    t_auth_header h;
} t_client_convertcharreq PACKED_ATTR();
/******************************************************/


/******************************************************/
#define SERVER_CONVERTCHARREPLY 0x18
/* Diablo II LoD 1.08 */
/* # 249 packet from server: type=0x0018(SERVER_CONVERTCHARREPLY) length=7 class=auth */
/* 0000:   07 00 18 00 00 00 00                                 ....... */
typedef struct
{
    t_auth_header h;
    bn_int        unknown1;  /* This seems to be always 00 00 00 00 */
} t_server_convertcharreply PACKED_ATTR();
#define SERVER_CONVERTCHARREPLY_UNKNOWN1 0x00000000
/******************************************************/


/******************************************************/
#define CLIENT_AUTHMOTDREQ 0x12
/* Diablo II 1.03 */
/* 0x0000: 03 00 12 */
typedef struct /* FIXME: ping request? */ /* is there normally a reply? */
{
    t_auth_header h;
} t_client_authmotdreq PACKED_ATTR();
/******************************************************/


/******************************************************/
#define SERVER_AUTHMOTDREPLY 0x12
typedef struct
{
    t_auth_header   h;
    bn_byte         unknown1; /* flag to control the following char ? */
                              /* unknown1 any char works */
    /* a string message */
} t_server_authmotdreply;
#define SERVER_AUTHMOTDREPLY_UNKNOWN1 0x00
/******************************************************/

#endif
