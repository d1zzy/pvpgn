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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef PVPGN_BNETD_VERSIONCHECK_H
#define PVPGN_BNETD_VERSIONCHECK_H

#include <cstdint>
#include <ctime>
#include <string>

#include "common/tag.h"


namespace pvpgn
{

	namespace bnetd
	{
		class VersionCheck;

		struct file_metadata
		{
			std::time_t timestamp;
			std::uint64_t file_size;
			std::string filename;
		};

		bool load_versioncheck_conf(const std::string& filename);
		void unload_versioncheck_conf();

		const VersionCheck& select_versioncheck_entry(t_tag architecture, t_tag client, std::uint32_t version_id);

		class VersionCheck
		{
		public:
			bool validate_checkrevision_data(std::uint32_t game_version, std::uint32_t checksum, const std::string& unparsed_file_metadata) const;

			std::string get_equation() const noexcept;
			std::string get_checkrevision_filename() const noexcept;
			std::string get_version_tag() const noexcept;

			~VersionCheck() = default;
		private:
			/*******************************************************************************/
			// Friend functions
			/*******************************************************************************/
			friend bool load_versioncheck_conf(const std::string& filename);
			friend const VersionCheck& select_versioncheck_entry(t_tag architecture, t_tag client, std::uint32_t version_id);


			/*******************************************************************************/
			// Constructors
			/*******************************************************************************/
			VersionCheck(std::uint32_t version_id, std::uint32_t game_version, std::uint32_t checksum, t_tag architecture,
				t_tag client, const struct file_metadata& exe_metadata, const std::string& equation,
				const std::string& checkrevision_filename, const std::string& version_tag);
			VersionCheck(const std::string& checkrevision_filename, const std::string& equation, const std::string& version_tag);
			

			/*******************************************************************************/
			// Member variables
			/*******************************************************************************/
			const std::uint32_t			m_version_id; // AKA "Version Byte"
			const std::uint32_t			m_game_version;
			const std::uint32_t			m_checksum;
			const t_tag					m_architecture;
			const t_tag					m_client;
			const struct file_metadata	m_metadata;
			const std::string			m_equation;
			const std::string			m_checkrevision_filename;
			const std::string			m_version_tag;
		};

	} // namespace bnetd

} // namespace pvpgn

#endif //PVPGN_BNETD_VERSIONCHECK_H