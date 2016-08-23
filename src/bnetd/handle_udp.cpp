/*
 * Copyright (C) 1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
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
#include "handle_udp.h"

#include "common/eventlog.h"
#include "common/packet.h"
#include "common/addr.h"
#include "common/bn_type.h"

#include "connection.h"
#include "udptest_send.h"
#include "common/setup_after.h"

namespace pvpgn
{

	namespace bnetd
	{

		extern int handle_udp_packet(int usock, unsigned int src_addr, unsigned short src_port, t_packet const * const packet)
		{
			if (!packet)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "[{}] got NULL packet", usock);
				return -1;
			}
			if (packet_get_class(packet) != packet_class_udp)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "[{}] got bad packet (class {})", usock, (int)packet_get_class(packet));
				return -1;
			}

			switch (packet_get_type(packet))
			{
			case SERVER_UDPTEST: /* we might get these if a client is running on the same machine as us */
				if (packet_get_size(packet) < sizeof(t_server_udptest))
				{
					eventlog(eventlog_level_error, __FUNCTION__, "[{}] got bad UDPTEST packet (expected {} bytes, got {})", usock, sizeof(t_server_udptest), packet_get_size(packet));
					return -1;
				}
				eventlog(eventlog_level_debug, __FUNCTION__, "[{}] got UDPTEST packet from {} (myself?)", usock, addr_num_to_addr_str(src_addr, src_port));
				return 0;

			case CLIENT_UDPPING:
				if (packet_get_size(packet) < sizeof(t_client_udpping))
				{
					eventlog(eventlog_level_error, __FUNCTION__, "[{}] got bad UDPPING packet (expected {} bytes, got {})", usock, sizeof(t_client_udpping), packet_get_size(packet));
					return -1;
				}
				eventlog(eventlog_level_debug, __FUNCTION__, "[{}] got udpping unknown1={}", usock, bn_int_get(packet->u.client_udpping.unknown1));
				return 0;

			case CLIENT_SESSIONADDR1:
				if (packet_get_size(packet) < sizeof(t_client_sessionaddr1))
				{
					eventlog(eventlog_level_error, __FUNCTION__, "[{}] got bad SESSIONADDR1 packet (expected {} bytes, got {})", usock, sizeof(t_client_sessionaddr1), packet_get_size(packet));
					return -1;
				}

				{
					t_connection * c;

					if (!(c = connlist_find_connection_by_sessionkey(bn_int_get(packet->u.client_sessionaddr1.sessionkey))))
					{
						eventlog(eventlog_level_error, __FUNCTION__, "[{}] address not set (no connection with session key 0x{:08x})", usock, bn_int_get(packet->u.client_sessionaddr1.sessionkey));
						return -1;
					}

					if (conn_get_game_addr(c) != src_addr || conn_get_game_port(c) != src_port)
						eventlog(eventlog_level_info, __FUNCTION__, "[{}][{}] SESSIONADDR1 set new UDP address to {}", conn_get_socket(c), usock, addr_num_to_addr_str(src_addr, src_port));

					conn_set_game_socket(c, usock);
					conn_set_game_addr(c, src_addr);
					conn_set_game_port(c, src_port);

					udptest_send(c);
				}
				return 0;

			case CLIENT_SESSIONADDR2:
				if (packet_get_size(packet) < sizeof(t_client_sessionaddr2))
				{
					eventlog(eventlog_level_error, __FUNCTION__, "[{}] got bad SESSIONADDR2 packet (expected {} bytes, got {})", usock, sizeof(t_client_sessionaddr2), packet_get_size(packet));
					return -1;
				}

				{
					t_connection * c;

					if (!(c = connlist_find_connection_by_sessionnum(bn_int_get(packet->u.client_sessionaddr2.sessionnum))))
					{
						eventlog(eventlog_level_error, __FUNCTION__, "[{}] address not set (no connection with session number {})", usock, bn_int_get(packet->u.client_sessionaddr2.sessionnum));
						return -1;
					}
					if (conn_get_sessionkey(c) != bn_int_get(packet->u.client_sessionaddr2.sessionkey))
					{
						eventlog(eventlog_level_error, __FUNCTION__, "[{}][{}] address not set (expected session key 0x{:08x}, got 0x{:08x})", conn_get_socket(c), usock, conn_get_sessionkey(c), bn_int_get(packet->u.client_sessionaddr2.sessionkey));
						return -1;
					}

					if (conn_get_game_addr(c) != src_addr || conn_get_game_port(c) != src_port)
						eventlog(eventlog_level_info, __FUNCTION__, "[{}][{}] SESSIONADDR2 set new UDP address to {}", conn_get_socket(c), usock, addr_num_to_addr_str(src_addr, src_port));

					conn_set_game_socket(c, usock);
					conn_set_game_addr(c, src_addr);
					conn_set_game_port(c, src_port);

					udptest_send(c);
				}
				return 0;

			case CLIENT_SEARCH_LAN_GAMES: //added by Spider
			{
											  eventlog(eventlog_level_debug, __FUNCTION__, "[{}] got SEARCH_LAN_GAMES packet from {}", usock, addr_num_to_addr_str(src_addr, src_port));
											  return 0;
			}

			default:
				eventlog(eventlog_level_debug, __FUNCTION__, "[{}] got unknown udp packet type 0x{:04x}, len {} from {}", usock, packet_get_type(packet), packet_get_size(packet), addr_num_to_addr_str(src_addr, src_port));
			}

			return 0;
		}

	}

}
