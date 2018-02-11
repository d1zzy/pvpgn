/*
 * Copyright (C) 2000 Onlyer (onlyer@263.net)
 * Copyright (C) 2001 Ross Combs (ross@bnetd.org)
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
#include "common/setup_before.h"
#define VERSIONCHECK_INTERNAL_ACCESS
#include "versioncheck.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <forward_list>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>

#include "compat/strcasecmp.h"
#include "common/format.h"
#include "common/eventlog.h"
#include "common/util.h"
#include "common/field_sizes.h"
#include "common/token.h"
#include "common/proginfo.h"

#include "json/json.hpp"

#include "prefs.h"
#include "common/setup_after.h"


using json = nlohmann::json;


namespace pvpgn
{

	namespace bnetd
	{

		std::forward_list<VersionCheck> vc_entries = std::forward_list<VersionCheck>();

		bool versioncheck_conf_is_loaded = false;

		struct file_metadata parse_file_metadata(const std::string& unparsed_metadata);
		bool compare_file_metadata(const struct file_metadata& pattern, const struct file_metadata& match, bool skip_timestamp_match);


		bool load_versioncheck_conf(const std::string& filename)
		{
			if (versioncheck_conf_is_loaded)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "Could not load {}, a versioncheck configuration file is already loaded", filename);
				return false;
			}

			std::ifstream file_stream(filename, std::ios::in);
			if (!file_stream.is_open())
			{
				eventlog(eventlog_level_error, __FUNCTION__, "Could not open file \"{}\" for reading", filename);
				return false;
			}

			json j;
			try
			{
				file_stream >> j;
			}
			catch (const std::exception& e)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "Could not parse JSON data: {}", e.what());
				return false;
			}

			
			int success_count = 0;
			int total_count = 0;
			for (auto jclient : json::iterator_wrapper(j))
			{
				total_count += jclient.value().size();
				for (const auto& entry : jclient.value())
				{
					try
					{
						unsigned long versionid = std::strtoul(entry["versionbyte"].get<std::string>().c_str(), nullptr, 0);
						if (versionid == ULONG_MAX || versionid == 0)
						{
							throw std::runtime_error("Invalid versionbyte \"" + entry["versionbyte"].get<std::string>() + "\" in entry \"" + entry["title"].get<std::string>() + "\"");
						}

						unsigned long game_version;
						if (verstr_to_vernum(entry["version"].get<std::string>().c_str(), &game_version) < 0)
						{
							throw std::runtime_error("Invalid version \"" + entry["version"].get<std::string>() + "\" in entry \"" + entry["title"].get<std::string>() + "\"");
						}

						unsigned long checksum = std::strtoul(entry["hash"].get<std::string>().c_str(), nullptr, 0);
						if (checksum == ULONG_MAX || checksum == 0)
						{
							throw std::runtime_error("Invalid hash \"" + entry["hash"].get<std::string>() + "\" in entry \"" + entry["title"].get<std::string>() + "\"");
						}

						t_tag architecture = tag_str_to_uint(entry["architecture"].get<std::string>().c_str());
						if (!tag_check_arch(architecture))
						{
							throw std::runtime_error("Invalid architecture \"" + entry["architecture"].get<std::string>() + "\" in entry \"" + entry["title"].get<std::string>() + "\"");
						}

						t_tag client = tag_str_to_uint(jclient.key().c_str());
						if (!tag_check_client(client))
						{
							throw std::runtime_error("Invalid client \"" + entry["client"].get<std::string>() + "\" in entry \"" + entry["title"].get<std::string>() + "\"");
						}

						struct file_metadata metadata;
						if (entry["file_metadata"].get<std::string>() == "NULL")
						{
							metadata = {};
						}
						else
						{
							metadata = parse_file_metadata(entry["file_metadata"].get<std::string>());
							if (metadata.filename.empty()
								&& metadata.file_size == 0
								&& metadata.timestamp == 0)
							{
								throw std::runtime_error("Invalid file_metadata \"" + entry["file_metadata"].get<std::string>() + "\" in entry \"" + entry["title"].get<std::string>() + "\"");
							}
						}

						// unable to check for errors at this time
						std::string equation = entry["equation"].get<std::string>();

						std::string checkrevision_filename = entry["checkrevision_file"].get<std::string>();

						std::string version_tag = entry["versiontag"].get<std::string>();

						vc_entries.push_front(VersionCheck(versionid, game_version, checksum, architecture, client,
							metadata, equation, checkrevision_filename, version_tag));

						success_count += 1;
					}
					catch (const std::exception& e)
					{
						eventlog(eventlog_level_error, __FUNCTION__, "{}", e.what());
						continue;
					}
				}
			}

			eventlog(eventlog_level_info, __FUNCTION__, "Successfully loaded {} out of {} versioncheck entries", success_count, total_count);

			versioncheck_conf_is_loaded = true;

			return true;
		}

		void unload_versioncheck_conf()
		{
			if (versioncheck_conf_is_loaded)
			{
				vc_entries.clear();

				eventlog(eventlog_level_info, __FUNCTION__, "Successfully unloaded all version check entries");

				versioncheck_conf_is_loaded = false;
			}
		}

		const VersionCheck& select_versioncheck_entry(t_tag architecture, t_tag client, std::uint32_t version_id)
		{
			static const VersionCheck invalid_entry("IX86ver1.mpq", "A=42 B=42 C=42 4 A=A^S B=B^B C=C^C A=A^S", "NoVC");

			for (const auto& entry : vc_entries)
			{
				if (entry.m_architecture == architecture
					&& entry.m_client == client
					&& entry.m_version_id == version_id)
				{
					return entry;
				}
			}

			return invalid_entry;
		}

		VersionCheck::VersionCheck(std::uint32_t version_id, std::uint32_t game_version, std::uint32_t checksum, t_tag architecture,
			t_tag client, const struct file_metadata& metadata, const std::string& equation,
			const std::string& checkrevision_filename, const std::string& version_tag)
			: m_version_id(version_id),
			m_game_version(game_version),
			m_checksum(checksum),
			m_architecture(architecture),
			m_client(client),
			m_metadata(metadata),
			m_equation(equation),
			m_checkrevision_filename(checkrevision_filename),
			m_version_tag(version_tag)
		{

		}

		VersionCheck::VersionCheck(const std::string& checkrevision_filename, const std::string& equation, const std::string& version_tag)
			: m_version_id(),
			m_game_version(),
			m_checksum(),
			m_architecture(),
			m_client(),
			m_metadata(),
			m_equation(equation),
			m_checkrevision_filename(checkrevision_filename),
			m_version_tag(version_tag)
		{

		}

		bool VersionCheck::validate_checkrevision_data(std::uint32_t game_version, std::uint32_t checksum, const std::string& unparsed_file_metadata) const
		{
			if (this->m_version_tag == "NoVC")
			{
				return false;
			}

			if (this->m_game_version != game_version)
			{
				eventlog(eventlog_level_debug, __FUNCTION__, "Failed CheckRevision: Invalid game version \"{X}\"", game_version);
				return false;
			}


			if (this->m_checksum != 0
				&& prefs_get_allow_bad_version())
			{
				if (this->m_checksum != checksum)
				{
					eventlog(eventlog_level_debug, __FUNCTION__, "Failed CheckRevision: Invalid checksum \"{X}\"", checksum);
					return false;
				}
			}
			else
			{
				// checksum can be disabled globally via setting allow_bad_version = true in conf/bnetd.conf
				// it can also be disabled on individual versioncheck entries by setting 0 in the checksum field in conf/versioncheck.conf
				eventlog(eventlog_level_debug, __FUNCTION__, "Skipping checksum validation");
			}


			if (unparsed_file_metadata != "NULL"
				&& std::strcmp(prefs_get_version_exeinfo_match(), "true") == 0)
			{
				if (unparsed_file_metadata == "badexe")
				{
					// missing or too long file metadata received
					eventlog(eventlog_level_debug, __FUNCTION__, "Failed CheckRevision: Invalid file metadata \"{}\"", unparsed_file_metadata);
					return false;
				}

				struct file_metadata parsed_metadata = parse_file_metadata(unparsed_file_metadata);;
				if (parsed_metadata.filename.empty()
					&& parsed_metadata.file_size == 0
					&& parsed_metadata.timestamp == 0)
				{
					eventlog(eventlog_level_debug, __FUNCTION__, "Failed CheckRevision: Invalid file metadata \"{}\"", unparsed_file_metadata);
					return false;
				}

				// Skip timestamp matching if client is WC3
				// WC3 installers change the file timestamp to time of installation
				if (!compare_file_metadata(this->m_metadata, parsed_metadata, 
					this->m_client == CLIENTTAG_WARCRAFT3_UINT || this->m_client == CLIENTTAG_WAR3XP_UINT ? true : false)
					)
				{
					eventlog(eventlog_level_debug, __FUNCTION__, "Failed CheckRevision: Invalid file metadata \"{}\"", unparsed_file_metadata);
					return false;
				}
			}
			else
			{
				// metadata matching can be disabled globally via setting version_exeinfo_match = false in conf/bnetd.conf
				// it can also be disabled on individual versioncheck entries by setting "NULL" in the exeinfo field in conf/versioncheck.conf
				eventlog(eventlog_level_debug, __FUNCTION__, "Skipping file metadata validation");
			}

			return true;
		}

		std::string VersionCheck::get_equation() const noexcept
		{
			return this->m_equation;
		}

		std::string VersionCheck::get_checkrevision_filename() const noexcept
		{
			return this->m_checkrevision_filename;
		}

		std::string VersionCheck::get_version_tag() const noexcept
		{
			return this->m_version_tag;
		}

		struct file_metadata parse_file_metadata(const std::string& unparsed_metadata)
		{
			// happens when using war3-noCD and having deleted war3.org
			if (unparsed_metadata.empty())
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got empty file metadata string");
				return {};
			}

			// happens when AUTHREQ had no owner/exeinfo entry
			if (unparsed_metadata == "badexe")
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got \"badexe\" as file metadata string");
				return {};
			}

			std::smatch tokens;
			try
			{
				if (std::regex_match(unparsed_metadata, tokens, std::regex{ R"(([[:print:]]+\.exe) (\d\d/\d\d/\d\d \d\d:\d\d:\d\d) (\d+))" }) == false)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "got invalid file metadata string \"{}\"", unparsed_metadata);
					return {};
				}
			}
			catch (const std::regex_error& e)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "{} (code {})", e.what(), e.code());
				return {};
			}

			// Example metadata string
			// "Warcraft III.exe 07/07/17 20:15:59 562152"

			std::string filename = tokens[1];

			std::tm timestamp_raw;
			timestamp_raw.tm_isdst = -1;
			std::istringstream ss(tokens[2]);
			// Since year is 2 digits,
			// Range [69,99] results in values 1969 to 1999, range [00,68] results in 2000-2068
			ss >> std::get_time(&timestamp_raw, "%D %T");
			if (ss.fail())
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got invalid date and time in file metadata string");
				return {};
			}

			std::time_t timestamp = std::mktime(&timestamp_raw);
			if (timestamp == -1)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "time could not be represented as a std::time_t object");
				return {};
			}

			std::uint64_t filesize;
			try
			{
				filesize = std::stoull(tokens[3]);
			}
			catch (const std::exception& e)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "could not convert file size in file metadata string: {}", e.what());
				return {};
			}

			return { timestamp, filesize, filename };
		}

		bool compare_file_metadata(const struct file_metadata& pattern, const struct file_metadata& match, bool skip_timestamp_match)
		{
			if (skip_timestamp_match == false
				&& pattern.timestamp != match.timestamp)
			{
				return false;
			}

			if (pattern.file_size != match.file_size)
			{
				return false;
			}

			// Case insensitive comparison of filename
			if (strcasecmp(pattern.filename.c_str(), match.filename.c_str()) != 0)
			{
				return false;
			}

			return true;
		}

	}

}
