/*
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
#ifndef __W3XP_PROTOCOL_H__
#define __W3XP_PROTOCOL_H__

#include "common/bnet_protocol.h"

#define CLIENT_W3XP_COUNTRYINFO 0x72ff
#define CLIENT_W3XP_COUNTRYINFO_UNKNOWN1			0x1F450843
#define CLIENT_W3XP_COUNTRYINFO_LANGID_USENGLISH		0x00000409	
#define CLIENT_W3XP_COUNTRYINFO_COUNTRYNAME_USA			"United States"

#define SERVER_W3XP_AUTHREQ 0x72ff
#define CLIENT_W3XP_AUTHREQ	0x04ff
#define SERVER_W3XP_AUTHREPLY	0x04ff

#define SERVER_W3XP_ECHOREQ	0x08ff
#define CLIENT_W3XP_ECHOREPLY	0x08ff

#define CLIENT_W3XP_FILEINFOREQ 0x73ff
#define CLIENT_W3XP_FILEINFOREQ_TYPE_TOS	0x00000001 /* Terms of Service enUS */
#define CLIENT_W3XP_FILEINFOREQ_TYPE_NWACCT	0x00000002 /* New Account File */
#define CLIENT_W3XP_FILEINFOREQ_TYPE_CHTHLP	0x00000003 /* Chat Help */
#define CLIENT_W3XP_FILEINFOREQ_TYPE_SRVLST	0x00000005 /* Server List */
#define CLIENT_W3XP_FILEINFOREQ_UNKNOWN1	0x00000000
#define CLIENT_W3XP_FILEINFOREQ__FILE_TOSUSA	"termsofservice-enUS.txt"
#define CLIENT_W3XP_FILEINFOREQ__FILE_NWACCT	"newaccount-enUS.txt"
#define CLIENT_W3XP_FILEINFOREQ__FILE_CHTHLP	"chathelp-war3-enUS.txt"
#define CLIENT_W3XP_FILEINFOREQ__FILE_SRVLST	"bnserver-WAR3.ini"
#define SERVER_W3XP_FILEINFOREPLY	0x73ff

#define CLIENT_W3XP_ICONREQ	0x45ff
#define SERVER_W3XP_ICONREPLY	0x45ff

#define CLIENT_W3XP_LOGINREQ 0x43ff
#define SERVER_W3XP_LOGINREPLY	0x43ff
#define CLIENT_W3XP_LOGONPROOFREQ 0x12ff
#define SERVER_W3XP_LOGONPROOFREPLY	0x12ff

#define CLIENT_W3XP_CHANGEGAMEPORT	0x5aff

#define CLIENT_W3XP_MOTD		0x36ff
#define SERVER_W3XP_MOTD	0x36ff

#define CLIENT_W3XP_JOINCHANNEL	0x71ff
#define SERVER_W3XP_MESSAGE	0x59ff

#define CLIENT_W3XP_STATSREQ	0x16ff
#define SERVER_W3XP_STATSREPLY	0x16ff

#define CLIENT_W3XP_CREATEACCOUNT	0x07ff
#define SERVER_W3XP_CREATEACCOUNT	0x07ff

#define CLIENT_W3XP_GAMELISTREQ		0x68FF
#define SERVER_W3XP_GAMELISTREPLY	0x68FF
#define CLIENT_W3XP_STARTGAME		0x79ff
#define CLIENT_W3XP_JOIN_GAME		0x4AFF

#define CLIENT_W3XP_MESSAGE		0x0aff

#define SERVER_W3XP_FRIENDADD_ACK	0x0bff

#define CLIENT_W3XP_FRIENDSLIST_REQ	0x33ff

#define SERVER_W3XP_FRIENDSLIST_REPLY	0x33ff

#define	SERVER_W3XP_FRIENDDEL_ACK	0x3fff

#define	CLIENT_W3XP_FRIENDINFO_REQ	0x20ff

#define	SERVER_W3XP_FRIENDINFO_REPLY	0x20ff

#define CLIENT_W3XP_ANONGAME_REQ	0x34ff

#define SERVER_W3XP_ANONGAME_REPLY	0x34ff

#define CLIENT_W3XP_ADREQ		0x42ff

#define SERVER_W3XP_ADREPLY		0x42ff


/* W3 ROUTE packets */

#define CLIENT_W3XP_W3ROUTE_REQ		0x1EF7

// #define SERVER_W3XP_W3ROUTE_ECHOREQ	0x46F7

#define SERVER_W3XP_W3ROUTE_PLAYERINFO	0x23f7

#define CLIENT_W3XP_W3ROUTE_GAMERESULT	0x2ef7

#define CLIENT_W3XP_CHANLISTREQ		0x62ff
#define SERVER_W3XP_CHANLISTREPLY	0x62ff

// arranged team handling

#define CLIENT_W3XP_ARRANGEDTEAM_FRIENDSCREEN 0x2eff

#define SERVER_W3XP_ARRANGEDTEAM_FRIENDSCREEN 0x2eff

#define CLIENT_ARRANGEDTEAM_ACCEPT_INVITE 0xfdff
typedef struct
{
	t_bnet_header h;
} t_client_arrangedteam_accept_invite PACKED_ATTR();

// clan handling

#define SERVER_W3XP_CLAN_MEMBER_CHIEFTAIN 0x04
#define SERVER_W3XP_CLAN_MEMBER_SHAMAN 0x03
#define SERVER_W3XP_CLAN_MEMBER_GRUNT 0x02
#define SERVER_W3XP_CLAN_MEMBER_PEON 0x01
#define SERVER_W3XP_CLAN_MEMBER_NEW 0x00
#define SERVER_W3XP_CLAN_MEMBER_OFFLINE 0x00
#define SERVER_W3XP_CLAN_MEMBER_ONLINE 0x01
#define SERVER_W3XP_CLAN_MEMBER_CHANNEL 0x02
#define SERVER_W3XP_CLAN_MEMBER_GAME 0x03
#define SERVER_W3XP_CLAN_MEMBER_PRIVATE_GAME 0x04

/*Paquet #267
0x0000   FF 75 0A 00 00 00 42 54-54 04                     ÿu....BTT.
*/
/*
300: recv class=bnet[0x02] type=unknown[0x70ff] length=12
0000:   FF 70 0C 00 01 00 00 00   00 64 73 66                .p.......dsf    
300: send class=bnet[0x02] type=unknown[0x70ff] length=56
0000:   FF 70 38 00 01 00 00 00   00 09 44 4A 50 32 00 44    .p8.......DJP2.D
0010:   4A 50 33 00 44 4A 50 34   00 44 4A 50 35 00 44 4A    JP3.DJP4.DJP5.DJ
0020:   50 36 00 44 4A 50 37 00   44 4A 50 38 00 44 4A 50    P6.DJP7.DJP8.DJP
0030:   39 00 44 4A 50 31 30 00                              9.DJP10.        
300: recv class=bnet[0x02] type=CLIENT_FRIENDINFOREQ[0x66ff] length=5
0000:   FF 66 05 00 01                                       .f...           
300: send class=bnet[0x02] type=unknown[0x66ff] length=12
0000:   FF 66 0C 00 01 00 00 00   00 00 00 00                .f..........    
300: recv class=bnet[0x02] type=unknown[0x71ff] length=64
0000:   FF 71 40 00 01 00 00 00   74 65 73 74 00 00 64 73    .q@.....test..ds
0010:   66 09 44 4A 50 32 00 44   4A 50 33 00 44 4A 50 34    f.DJP2.DJP3.DJP4
0020:   00 44 4A 50 35 00 44 4A   50 36 00 44 4A 50 37 00    .DJP5.DJP6.DJP7.
0030:   44 4A 50 38 00 44 4A 50   39 00 44 4A 50 31 30 00    DJP8.DJP9.DJP10.
300: send class=bnet[0x02] type=unknown[0x71ff] length=10
0000:   FF 71 0A 00 01 00 00 00   00 00                      .q........      
300: send class=bnet[0x02] type=unknown[0x75ff] length=12
0000:   FF 75 0C 00 00 00 00 00   64 73 66 00                .u......dsf. 
300: recv class=bnet[0x02] type=CLIENT_FRIENDINFOREQ[0x66ff] length=5
0000:   FF 66 05 00 02                                       .f...           
300: send class=bnet[0x02] type=unknown[0x66ff] length=12
0000:   FF 66 0C 00 02 00 00 00   00 00 00 00                .f.......... 
*/

#define CLIENT_W3XP_CLAN_CREATEREQ 0x70ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_int               clantag;
} t_client_w3xp_clan_createreq PACKED_ATTR();


#define SERVER_W3XP_CLAN_CREATEREPLY 0x70ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_byte			   check_result;
  bn_byte			   friend_count;
  // player name in chan or mutual
  //char player_name[sizeof (friend_name)];
} t_server_w3xp_clan_createreply PACKED_ATTR();
#define SERVER_W3XP_CLAN_CREATEREPLY_CHECK_OK 0x00
#define SERVER_W3XP_CLAN_CREATEREPLY_CHECK_ALLREADY_IN_USE 0x01
#define SERVER_W3XP_CLAN_CREATEREPLY_CHECK_TIME_LIMIT 0x02
#define SERVER_W3XP_CLAN_CREATEREPLY_CHECK_EXCEPTION 0x04
#define SERVER_W3XP_CLAN_CREATEREPLY_CHECK_INVALID_CLAN_TAG 0x0a

/*
3852: recv class=bnet[0x02] type=unknown[0x71ff] length=70
0000:   FF 71 46 00 01 00 00 00   53 75 62 57 61 72 5A 6F    .qF.....SubWarZo
0010:   6E 65 00 00 5A 57 53 09   44 4A 50 32 00 44 4A 50    ne..ZWS.DJP2.DJP
0020:   33 00 44 4A 50 34 00 44   4A 50 35 00 44 4A 50 36    3.DJP4.DJP5.DJP6
0030:   00 44 4A 50 37 00 44 4A   50 38 00 44 4A 50 31 30    .DJP7.DJP8.DJP10
0040:   00 44 4A 50 39 00                                    .DJP9.    
*/
#define CLIENT_W3XP_CLAN_CREATEINVITEREQ 0x71ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  // Clan Name (\0 terminated string)
  // bn_int			    clantag;
  // bn_byte			friend_count; //Number of friend selected
  // Name of friend (\0 terminated string)
} t_client_w3xp_clan_createinvitereq PACKED_ATTR();

/*3756: send class=bnet[0x02] type=unknown[0x71ff] length=14
0000:   FF 71 0E 00 02 00 00 00   05 44 4A 50 32 00          .q.......DJP2.  
<- Unable to receive invitation ( PG search, already in a clan, etc... )*/
/*3756: 
Paquet #266
0x0000   FF 71 0A 00 05 00 00 00-00 00                     ÿq........
<- Clan invitation = Sucessfully done */
#define SERVER_W3XP_CLAN_CREATEINVITEREPLY 0x71ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_byte			   status; // 0x05 = Cannot contact(not in channel screen) or already in clan | 0x04 = Decline | 0x00 = OK :)
// Name of failed member(\0 terminated string)
} t_server_w3xp_clan_createinvitereply PACKED_ATTR();

#define SERVER_W3XP_CLAN_CREATEINVITEREQ 0x72ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_int			   clantag;
  // Clan Name (\0 terminated string)
  // Clan Creator (\0 terminated string)
  // bn_byte			friend_count; //Number of friend selected
  // Name of friend (\0 terminated string)
} t_server_w3xp_clan_createinvitereq PACKED_ATTR();

#define CLIENT_W3XP_CLAN_CREATEINVITEREPLY 0x72ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_int			   clantag;
  // Clan Creator (\0 terminated string)
  // bn_byte			reply  /* 0x04--decline 0x05--Cannot contact(not in channel screen) or already in clan 0x06--accept*/ 
} t_client_w3xp_clan_createinvitereply PACKED_ATTR();

/*
3876: recv class=bnet[0x02] type=unknown[0x73ff] length=8
0000:   FF 73 08 00 01 00 00 00                              .s......      */
#define CLIENT_W3XP_CLAN_DISBANDREQ 0x73ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
} t_client_w3xp_clan_disbandreq PACKED_ATTR();

#define SERVER_W3XP_CLAN_DISBANDREPLY 0x73ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_byte              result;   /* 0-- Success  1-- Exception raised  2-- Clan exists less than 1 week, cannot remove */
} t_server_w3xp_clan_disbandreply PACKED_ATTR();
#define SERVER_W3XP_CLAN_DISBANDREPLY_RESULT_OK 0x0
#define SERVER_W3XP_CLAN_DISBANDREPLY_RESULT_EXCEPTION 0x1
#define SERVER_W3XP_CLAN_DISBANDREPLY_RESULT_FAILED 0x2

#define CLIENT_W3XP_CLAN_MEMBERNEWCHIEFREQ 0x74ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  //Player_Name deleted(\0 terminated)
} t_client_w3xp_clan_membernewchiefreq PACKED_ATTR();

#define SERVER_W3XP_CLAN_MEMBERNEWCHIEFREPLY 0x74ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_byte              result; /* 0-successful 1-failed */
} t_server_w3xp_clan_membernewchiefreply PACKED_ATTR();
#define SERVER_W3XP_CLAN_MEMBERNEWCHIEFREPLY_SUCCESS 0x00
#define SERVER_W3XP_CLAN_MEMBERNEWCHIEFREPLY_FAILED 0x01

#define SERVER_W3XP_CLAN_CLANACK 0x75ff
typedef struct{
  t_bnet_header		h;
  bn_byte		unknow1; /* 0x00 */
  bn_int		clantag;
  bn_byte		status;  /* member status */
} t_server_w3xp_clan_clanack PACKED_ATTR();

#define SERVER_W3XP_CLAN_CLANLEAVEACK 0x76ff
typedef struct{
  t_bnet_header        h;
  bn_byte              unknown1; /* always be zero? */
} t_server_w3xp_clan_clanleaveack PACKED_ATTR();
#define SERVER_W3XP_CLAN_CLANLEAVEACK_UNKNOWN1 0x00

/*
3876: recv class=bnet[0x02] type=unknown[0x77ff] length=13
0000:   FF 77 0D 00 01 00 00 00   44 4A 50 31 00             .w......DJP1.  */
#define CLIENT_W3XP_CLAN_INVITEREQ 0x77ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  //Player_Name invited
} t_client_w3xp_clan_invitereq PACKED_ATTR();

#define SERVER_W3XP_CLAN_INVITEREPLY 0x77ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_byte              result;  /* 0x04--decline 0x05--Cannot contact(not in channel screen) or already in clan */
} t_server_w3xp_clan_invitereply PACKED_ATTR();

#define CLIENT_W3XP_CLAN_MEMBERDELREQ 0x78ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  //Player_Name deleted(\0 terminated)
} t_client_w3xp_clan_memberdelreq PACKED_ATTR();

#define SERVER_W3XP_CLAN_MEMBERDELREPLY 0x78ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_byte              result; /* 0-successful 1-failed */
} t_server_w3xp_clan_memberdelreply PACKED_ATTR();
#define SERVER_W3XP_CLAN_MEMBERDELREPLY_SUCCESS 0x00
#define SERVER_W3XP_CLAN_MEMBERDELREPLY_FAILED 0x01

#define SERVER_W3XP_CLAN_INVITEREQ 0x79ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_int               clantag;
  //Clan_Name (\0 terminated)
  //Player_Name invited (\0 terminated)
} t_server_w3xp_clan_invitereq PACKED_ATTR();

#define CLIENT_W3XP_CLAN_INVITEREPLY 0x79ff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_int               clantag;
  //Player_Name invited (\0 terminated)
  //bn_byte            reply/* 0x04--decline 0x05--Cannot contact(not in channel screen) or already in clan 0x06--accept 0x07--no privilege to invite 0x08--cannot invite(??any difference from cannot contact?) 0x09--clan full*/
} t_client_w3xp_clan_invitereply PACKED_ATTR();
#define W3XP_CLAN_INVITEREPLY_SUCCESS 0x00
#define W3XP_CLAN_INVITEREPLY_DECLINE 0x04
#define W3XP_CLAN_INVITEREPLY_FAILED 0x05
#define W3XP_CLAN_INVITEREPLY_ACCEPT 0x06
#define W3XP_CLAN_INVITEREPLY_NOPRIVILEGE 0x07
#define W3XP_CLAN_INVITEREPLY_CANNOT 0x08
#define W3XP_CLAN_INVITEREPLY_CLANFULL 0x09

#define CLIENT_W3XP_CLAN_MEMBERCHANGEREQ 0x7aff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  //Player_Name invited(\0 terminated)
  //Player_Status(bn_byte: 1~4)
} t_client_w3xp_clan_memberchangereq PACKED_ATTR();

#define SERVER_W3XP_CLAN_MEMBERCHANGEREPLY 0x7aff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_byte              result; /* 0-successful 1-failed */
} t_server_w3xp_clan_memberchangereply PACKED_ATTR();
#define SERVER_W3XP_CLAN_MEMBERCHANGEREPLY_SUCCESS 0x00
#define SERVER_W3XP_CLAN_MEMBERCHANGEREPLY_FAILED 0x01

#define CLIENT_W3XP_CLAN_MOTDCHG 0x7bff
typedef struct{
  t_bnet_header        h;
  bn_int               unknow1;
  // Motd en string ^^
} t_client_w3xp_clan_motdchg PACKED_ATTR();
#define SERVER_W3XP_CLAN_MOTDREPLY_UNKNOW1 0x00000000

#define SERVER_W3XP_CLAN_MOTDREPLY 0x7cff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_int			   unknow1; // 0x00000000
  // MOTD 
} t_server_w3xp_clan_motdreply PACKED_ATTR();

#define CLIENT_W3XP_CLAN_MOTDREQ 0x7cff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
} t_client_w3xp_clan_motdreq PACKED_ATTR();

/*
Paquet #52
0x0000   FF 7D 10 01 01 00 00 00-13 4D 79 73 74 69 2E 53   ÿ}.......Mysti.S
0x0010   77 5A 00 04 01 00 73 61-75 72 6F 6E 2E 73 77 7A   wZ....sauron.swz
0x0020   00 03 00 00 73 69 6D 6F-6E 2E 53 77 5A 00 03 00   ....simon.SwZ...
0x0030   00 4E 65 6F 2D 56 61 67-72 61 6E 74 2E 73 77 7A   .Neo-Vagrant.swz
0x0040   00 02 00 00 77 4D 7A 00-02 00 00 6B 61 74 6E 6F   ....wMz....katno
0x0050   6D 61 64 2E 53 77 5A 00-02 00 00 47 7A 62 65 75   mad.SwZ....Gzbeu
0x0060   68 2E 53 77 5A 00 02 00-00 53 69 6C 76 65 72 2E   h.SwZ....Silver.
0x0070   53 77 5A 00 02 00 00 4D-61 67 67 65 75 73 00 02   SwZ....Maggeus..
0x0080   00 00 4F 6E 69 2D 4D 75-73 68 61 2E 53 77 5A 00   ..Oni-Musha.SwZ.
0x0090   02 00 00 4D 61 67 67 65-75 53 2E 53 77 5A 00 02   ...MaggeuS.SwZ..
0x00A0   00 00 53 69 72 65 5F 4C-6F 75 70 00 02 00 00 6B   ..Sire_Loup....k
0x00B0   69 6C 6C 69 62 6F 79 00-03 00 00 52 65 64 2E 44   illiboy....Red.D
0x00C0   72 61 4B 65 00 02 00 00-53 69 72 65 2E 53 77 5A   raKe....Sire.SwZ
0x00D0   00 03 00 00 73 74 72 61-69 67 68 74 5F 63 6F 75   ....straight_cou
0x00E0   67 61 72 00 02 00 00 52-65 64 44 72 61 6B 65 2E   gar....RedDrake.
0x00F0   53 77 5A 00 02 00 00 54-72 6F 6C 6C 6F 00 02 00   SwZ....Trollo...
0x0100   00 53 69 6C 76 65 72 62-65 61 72 64 00 00 00 00   .Silverbeard....
*/
#define SERVER_W3XP_CLAN_MEMBERREPLY 0x7dff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
  bn_byte			   member_count;
  // player repeat start
  // Name of player(\0 terminated string)
  // bn_byte		   CHIEFTAIN = 0x04
  //				   SHAMANS = 0x03
  //				   GRUNT = 0x02
  //				   PEON = 0x01
  //				   NEW_MEMBER = 0x00 <- can't be promoted/devoted
  // bn_byte		   online status
  // unknown(always \0)
  // repeat end
} t_server_w3xp_clan_memberreply PACKED_ATTR();

/*
Paquet #51
0x0000   FF 7D 08 00 01 00 00 00-                          ÿ}......
*/

#define CLIENT_W3XP_CLAN_MEMBERREQ 0x7dff
typedef struct{
  t_bnet_header        h;
  bn_int               count;
} t_client_w3xp_clan_memberreq PACKED_ATTR();

#define SERVER_W3XP_CLAN_MEMBERLEAVEACK 0x7eff
typedef struct{
  t_bnet_header        h;
  //Player_Name deleted(\0 terminated)
} t_server_w3xp_clan_memberleaveack PACKED_ATTR();

#define SERVER_W3XP_CLAN_MEMBERCHANGEACK 0x7fff
typedef struct{
  t_bnet_header        h;
  //Player_Name invited(\0 terminated)
  //Player_Status(bn_byte: 1~4)
  //Player_Online(bn_short: 0x0/0x1)
} t_server_w3xp_clan_memberchangeack PACKED_ATTR();

#endif
