/*
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 2005 Dizzy
 * Copyright (C) 2014 HarpyWar
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
#include "adbanner.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "json/json.hpp"

#include "common/bn_type.h"
#include "compat/strcasecmp.h"
#include "common/eventlog.h"
#include "common/systemerror.h"
#include "connection.h"
#include "common/setup_after.h"


using json = nlohmann::json;


namespace pvpgn
{
	namespace bnetd
	{
		std::vector<AdBanner> AdBanner::m_banners;

		AdBanner::AdBanner()
			: m_id(0),
			m_extensiontag(0),
			m_filename(),
			m_url(),
			m_client(0),
			m_lang(0)
		{
		}

		AdBanner::AdBanner(std::size_t id, bn_int extensiontag, const std::string& filename, const std::string& url, t_clienttag clienttag, t_gamelang lang)
			: m_id(id),
			m_extensiontag(bn_int_get(extensiontag)),
			m_filename(filename),
			m_url(url),
			m_client(clienttag),
			m_lang(lang)
		{
			char language[5] = {};
			eventlog(eventlog_level_debug, __FUNCTION__, "Created ad id=0x%08x filename=\"%s\" link=\"%s\" client=\"%s\" lang=\"%s\"", id, filename.c_str(), url.c_str(), clienttag ? clienttag_uint_to_str(clienttag) : "NULL", lang ? tag_uint_to_str(language, lang) : "NULL");
		}

		void AdBanner::load(const std::string& filename)
		{
			m_banners.clear();

			std::ifstream file_stream(filename, std::ios::in);
			if (!file_stream.is_open())
				throw std::runtime_error("Could not open " + filename);

			json j;
			try
			{
				j << file_stream;
			}
			catch (const std::invalid_argument& e)
			{
				throw std::runtime_error("Invalid JSON file: " + std::string(e.what()));
			}

			for (const auto& ad : j["ads"])
			{
				try
				{
					if (ad["filename"].get<std::string>().find('/') != std::string::npos
						|| ad["filename"].get<std::string>().find('\\') != std::string::npos)
						throw std::runtime_error("Paths are not supported (" + ad["filename"].get<std::string>() + ")");

					std::size_t ext = ad["filename"].get<std::string>().find('.');
					if (ext == std::string::npos)
						throw std::runtime_error("Filename must contain an extension");

					std::string file_extension(ad["filename"].get<std::string>().substr(ext + 1));
					bn_int extensiontag = {};
					if (strcasecmp(file_extension.c_str(), "pcx") == 0)
						bn_int_tag_set(&extensiontag, EXTENSIONTAG_PCX);
					else if (strcasecmp(file_extension.c_str(), "mng") == 0)
						bn_int_tag_set(&extensiontag, EXTENSIONTAG_MNG);
					else if (strcasecmp(file_extension.c_str(), "png") == 0)
						bn_int_tag_set(&extensiontag, EXTENSIONTAG_MNG);
					else if (strcasecmp(file_extension.c_str(), "smk") == 0)
						bn_int_tag_set(&extensiontag, EXTENSIONTAG_SMK);
					else
						throw std::runtime_error("Unknown file extension (" + file_extension + ")");

					t_clienttag ctag = 0;
					if (strcasecmp(ad["client"].get<std::string>().c_str(), "NULL") != 0)
					{
						ctag = clienttag_str_to_uint(ad["client"].get<std::string>().c_str());

						if (std::strcmp(clienttag_uint_to_str(ctag), "UNKN") == 0)
							throw std::runtime_error("Unknown client tag (" + ad["client"].get<std::string>() + ")");
					}

					t_gamelang ltag = 0;
					if (strcasecmp(ad["lang"].get<std::string>().c_str(), "any") != 0)
					{
						ltag = tag_str_to_uint(ad["lang"].get<std::string>().c_str());

						if (!tag_check_gamelang(ltag))
							throw std::runtime_error("Unknown language (" + ad["lang"].get<std::string>() + ")");
					}

					m_banners.push_back(AdBanner(m_banners.size(), extensiontag, ad["filename"].get<std::string>(),
						ad["url"].get<std::string>(), ctag, ltag));
				}
				catch (const std::runtime_error& e)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "Could not load ad: %s", e.what());
					continue;
				}
			}
		}

		AdBanner AdBanner::pick(t_clienttag client_tag, t_gamelang client_lang, std::size_t prev_ad_id)
		{
			auto random_ad = [client_tag, client_lang, prev_ad_id]()
			{
				std::vector<AdBanner> candidates = {};
				std::copy_if(m_banners.begin(), m_banners.end(), std::back_inserter(candidates), [=](const AdBanner& a) -> bool
				{
					return a.get_client() == client_tag
						&& a.get_language() == client_lang
						&& a.get_id() != prev_ad_id
						? true : false;
				});

				std::default_random_engine eng{};
				std::uniform_int_distribution<std::size_t> random(0, m_banners.size() - 1);

				return m_banners.at(random(eng));
			};

			switch (m_banners.size())
			{
			case 0:
				return AdBanner();
			case 1:
				return m_banners.at(0);
			default:
				if (prev_ad_id == 0)
					return random_ad();
				else
					return m_banners.at(prev_ad_id + 1);
			}
		}

		AdBanner AdBanner::find(t_clienttag client_tag, t_gamelang client_lang, std::size_t ad_id)
		{
			auto result = std::find_if(m_banners.begin(), m_banners.end(), 
				[client_tag, client_lang, ad_id](const AdBanner& a) -> bool
			{
				return a.get_client() == client_tag && a.get_language() == client_lang && a.get_id() == ad_id ? true : false;
			});
			
			return result != m_banners.end() ? m_banners.at(result - m_banners.begin()) : AdBanner();
		}

		bool AdBanner::empty() const
		{
			return this->m_client == 0
				&& this->m_extensiontag == 0
				&& this->m_filename.empty()
				&& this->m_id == 0
				&& this->m_lang == 0
				&& this->m_url.empty()
				? true : false;
		}

		std::size_t AdBanner::get_id() const
		{
			return this->m_id;
		}

		unsigned int AdBanner::get_extension_tag() const
		{
			return this->m_extensiontag;
		}

		std::string AdBanner::get_filename() const
		{
			return this->m_filename;
		}

		std::string AdBanner::get_url() const
		{
			return this->m_url;
		}

		t_clienttag AdBanner::get_client() const
		{
			return this->m_client;
		}

		t_gamelang AdBanner::get_language() const
		{
			return this->m_lang;
		}
	}
}
