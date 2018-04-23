/*
 * Copyright (C) 2000  Onlyer (onlyer@263.net)
 * Copyright (C) 2001  Ross Combs (ross@bnetd.org)
 * Copyright (C) 2002 Gianluigi Tiesi (sherpya@netfarm.it)
 * Copyright (C) 2004 CreepLord (creeplord@pvpgn.org)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef PVPGN_BNETD_VERSIONCHECK_H
#define PVPGN_BNETD_VERSIONCHECK_H

#include <cstdint>
#include <ctime>
#include <string>
#include <tuple>

#include "common/tag.h"


namespace pvpgn
{

	namespace bnetd
	{
		class VersionCheck;


		// filename, equation
		std::tuple<std::string, std::string> select_checkrevision(t_tag architecture, t_tag client, std::uint32_t version_id);

		const VersionCheck* select_versioncheck(t_tag architecture, t_tag client, std::uint32_t version_id,
			std::uint32_t checkrevision_version, std::uint32_t checkrevision_checksum);


		/*******************************************************************************/
		// Conf
		/*******************************************************************************/
		bool load_versioncheck_conf(const std::string& filename);
		void unload_versioncheck_conf();


		class VersionCheck
		{
		public:
			/*******************************************************************************/
			// Getters
			/*******************************************************************************/
			std::string get_version_tag() const;

			/*******************************************************************************/
			// Deconstructors
			/*******************************************************************************/
			~VersionCheck() = default;
		private:
			/*******************************************************************************/
			// Friend functions
			/*******************************************************************************/
			friend bool load_versioncheck_conf(const std::string& filename);
			friend const VersionCheck* select_versioncheck(t_tag architecture, t_tag client, std::uint32_t version_id,
				std::uint32_t checkrevision_version, std::uint32_t checkrevision_checksum);


			/*******************************************************************************/
			// Constructors
			/*******************************************************************************/
			VersionCheck(const std::string& title, std::uint32_t version_id, const std::string& game_version,
				const std::string& checksum, t_tag architecture, t_tag client, const std::string& version_tag);

			/*******************************************************************************/
			// Member variables
			/*******************************************************************************/
			std::uint32_t			m_version_id;	// AKA "Version Byte"
			std::uint32_t			m_game_version;	// Windows file version
			std::uint32_t			m_checksum;
			t_tag					m_architecture;
			t_tag					m_client;
			std::string				m_version_tag;
		};

	} // namespace bnetd

} // namespace pvpgn

#endif //PVPGN_BNETD_VERSIONCHECK_H