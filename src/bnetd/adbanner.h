/*
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_ADBANNER_H
#define INCLUDED_ADBANNER_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "common/tag.h"
#include "common/bn_type.h"


namespace pvpgn
{
	namespace bnetd
	{
		/**
		*	Forward Declarations
		*/
		class AdBanner;

		/***************************************************************************************************/

		class AdBannerSelector
		{
		public:
			AdBannerSelector() = default;
			~AdBannerSelector() = default;

			bool is_loaded() const noexcept;
			void load(const std::string& filename);
			void unload() noexcept;

			std::size_t size() const noexcept;

			const AdBanner* const pick(t_clienttag client_tag, t_gamelang client_lang, std::size_t prev_ad_id);
			const AdBanner* const find(t_clienttag client_tag, t_gamelang client_lang, std::size_t ad_id);

		private:
			bool m_loaded = false;
			std::vector<AdBanner> m_banners;
		};
		extern AdBannerSelector AdBannerList;

		/***************************************************************************************************/

		class AdBanner
		{
		public:
			AdBanner() = delete;

			bool is_empty() const;

			std::size_t get_id() const noexcept;
			std::string get_filename() const noexcept;
			unsigned int get_extension_tag() const noexcept;
			std::string get_url() const noexcept;
			t_clienttag get_client() const noexcept;
			t_gamelang get_language() const noexcept;

		private:
			friend void AdBannerSelector::load(const std::string& filename);

			AdBanner(std::size_t id, bn_int extensiontag, const std::string& filename, const std::string& url, t_clienttag clienttag, t_gamelang language);

			const std::size_t m_id;
			const std::string m_filename;
			const unsigned int m_extensiontag;
			const std::string m_url;
			const t_clienttag m_client;
			const t_gamelang m_language;
		};
	}
}

#endif
