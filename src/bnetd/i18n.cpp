/*
* Copyright (C) 2014  HarpyWar (harpywar@gmail.com)
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

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <map>
#include <string.h>

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#include "compat/strcasecmp.h"
#include "compat/snprintf.h"

#include "common/token.h"

#include "common/list.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "common/xstring.h"
#include "common/util.h"
#include "common/tag.h"
#include "common/pugixml.h"
#include "common/format.h"

#include "account.h"
#include "connection.h"
#include "message.h"
#include "helpfile.h"
#include "channel.h"
#include "prefs.h"
#include "i18n.h"
#include "common/setup_after.h"

namespace pvpgn
{

	namespace bnetd
	{
		const char * commonfile = "common.xml"; // filename template, actually file name is "common-{lang}.xml"

		/* Array with string translations, each string has array with pair language=translation
			{ 
				original = { 
					{ language = translate }, 
					... 
				}, 
				... 
			} 
		*/
		std::map<std::string, std::map<t_gamelang, std::string> > translations = std::map<std::string, std::map<t_gamelang, std::string> >();

		const char * _find_string(char const * text, t_gamelang gamelang);

		extern const t_gamelang languages[12] = {
			GAMELANG_ENGLISH_UINT,	/* enUS */
			GAMELANG_GERMAN_UINT,	/* deDE */
			GAMELANG_CZECH_UINT,	/* csCZ */
			GAMELANG_SPANISH_UINT,	/* esES */
			GAMELANG_FRENCH_UINT,	/* frFR */
			GAMELANG_ITALIAN_UINT,	/* itIT */
			GAMELANG_JAPANESE_UINT,	/* jaJA */
			GAMELANG_KOREAN_UINT,	/* koKR */
			GAMELANG_POLISH_UINT,	/* plPL */
			GAMELANG_RUSSIAN_UINT,	/* ruRU */
			GAMELANG_CHINESE_S_UINT,	/* zhCN */
			GAMELANG_CHINESE_T_UINT	/* zhTW */
		};


		// http://msdn.microsoft.com/en-us/goglobal/bb896001.aspx
		extern const char * countries[][2] =
		{
			/* English (uses if other not found) */
			{ "USA", "enUS" },

			/* Polish */
			{ "POL", "plPL" },

			/* Korean */
			{ "KOR", "koKR" },

			/* French */
			{ "FRA", "frFR" },

			/* Bulgarian */
			{ "BGR", "bgBG" },

			/* Italian */
			{ "ITA", "itIT" },

			/* Japanese */
			{ "JPN", "jpJA" },

			/* Czech */
			{ "CZE", "csCZ" },

			/* Dutch */
			{ "NLD", "nlNL" },

			/* Portuguese */
			{ "BRA", "ptBR" },
			{ "PRT", "ptBR" },

			/* Swedish */
			{ "SWE", "svSE" },
			{ "FIN", "svSE" },

			/* German */
			{ "DEU", "deDE" },
			{ "AUT", "deDE" },
			{ "LIE", "deDE" },
			{ "LUX", "deDE" },

			/* German */
			{ "DEU", "deDE" },
			{ "AUT", "deDE" },
			{ "LIE", "deDE" },
			{ "LUX", "deDE" },

			/* Russian */
			{ "RUS", "ruRU" },
			{ "UZB", "ruRU" },
			{ "TTT", "ruRU" },
			{ "UKR", "ruRU" },
			{ "AZE", "ruRU" },
			{ "ARM", "ruRU" },

			/* Chinese */
			{ "CHN", "chCN" },
			{ "SGP", "chCN" },
			{ "HKG", "chTW" },
			{ "MCO", "chTW" },
			{ "TWN", "chTW" },

			/* Spanish */
			{ "ESP", "esES" },
			{ "ARG", "esES" },
			{ "BOL", "esES" },
			{ "CHL", "esES" },
			{ "COL", "esES" },
			{ "CRI", "esES" },
			{ "DOM", "esES" },
			{ "ECU", "esES" },
			{ "SLV", "esES" },
			{ "GTM", "esES" },
			{ "HND", "esES" },
			{ "MEX", "esES" },
			{ "NIC", "esES" },
			{ "PAN", "esES" },
			{ "PRY", "esES" },
			{ "PER", "esES" },
			{ "PRI", "esES" },
			{ "URY", "esES" },
			{ "VEN", "esES" },
		};



		extern int i18n_reload(void)
		{
			translations.clear();
			i18n_load();

			return 0;
		}

		extern int i18n_load(void)
		{
			std::string lang_filename;
			pugi::xml_document doc;
			std::string original, translate;

			// iterate language list
			for (int i = 0; i < (sizeof(languages) / sizeof(*languages)); i++)
			{
				lang_filename = i18n_filename(prefs_get_localizefile(), languages[i]);
				if (FILE *f = fopen(lang_filename.c_str(), "r")) {
					fclose(f);

					if (!doc.load_file(lang_filename.c_str()))
					{
						ERROR1("could not parse localization file \"%s\"", lang_filename.c_str());
						continue;
					}
				}
				else {
					// file not exists, ignore it
					continue;
				}

				// root node
				pugi::xml_node nodes = doc.child("root").child("items");

				// load xml strings to map
				for (pugi::xml_node node = nodes.child("item"); node; node = node.next_sibling("item"))
				{
					original = node.child_value("original");
					if (original[0] == '\0')
						continue;

					//std::map<const char *, std::map<t_gamelang, const char *> >::iterator it = translations.find(original);
					// if not found then init
					//if (it == translations.end())
					//	translations[original] = std::map<t_gamelang, const char *>();
					

					// check if translate string has a reference to another translation
					if (pugi::xml_attribute attr = node.child("translate").attribute("refid"))
					{
						if (pugi::xml_node n = nodes.find_child_by_attribute("id", attr.value()))
							translate = n.child_value("translate");
						else
						{
							translate = original;
							//WARN2("could not find translate reference refid=\"%s\", use original string (%s)", attr.value(), lang_filename.c_str());
						}
					}
					else
					{
						translate = node.child_value("translate");
						if (translate[0] == '\0')
						{
							translate = original;
							//WARN2("empty translate for \"%s\", use original string (%s)", original.c_str(), lang_filename.c_str());
						}
					}
					translations[original][languages[i]] = translate;
				}

				INFO1("localization file loaded \"%s\"", lang_filename.c_str());
			}

			return 0;
		}

		extern std::string _localize(t_connection * c, const char * func, const char * fmt, const fmt::ArgList &args)
		{
			const char *format;
			std::string output;
			try
			{
				format = fmt;

				if (t_gamelang lang = conn_get_gamelang(c))
				if (!(format = _find_string(fmt, lang)))
					format = fmt;
			
				output = fmt::format(format, args);
			}
			catch (const std::exception& e)
			{
				ERROR2("Can't format translation string \"%s\" (%s)", fmt, e.what());
			}
			return output;
		}


		/* Find localized text for the given language */
		const char * _find_string(char const * text, t_gamelang gamelang)
		{
			std::map<std::string, std::map<t_gamelang, std::string> >::iterator it = translations.find(text);
			if (it != translations.end())
			{
				if (!it->second[gamelang].empty())
					return it->second[gamelang].c_str();
			}
			return NULL;
		}

		/* Add a locale tag into filename
		example: motd.txt -> motd-ruRU.txt */
		extern const char * i18n_filename(const char * filename, t_tag gamelang)
		{
			// get language string
			char lang_str[sizeof(t_tag)+1];
			std::memset(lang_str, 0, sizeof(lang_str));
			tag_uint_to_str(lang_str, gamelang);

			struct stat sfile;
			const char * _filename;
			
			_filename = buildpath(buildpath(prefs_get_i18ndir(), lang_str), filename);
			// if localized file not found
			if (stat(_filename, &sfile) < 0)
			{
				// use default file
				_filename = buildpath(prefs_get_i18ndir(), filename);
			}

			return _filename;
		}

		/* Return language tag by code */
		extern t_gamelang lang_find_by_country(const char * code)
		{
			if (!code || code[0] == '\0')
				return tag_str_to_uint(countries[0][0]);

			for (int i = 0; i < (sizeof(countries) / sizeof(*countries)); i++)
			if (strcasecmp(code, countries[i][0]) == 0)
				return tag_str_to_uint(countries[i][0]);

			return tag_str_to_uint(countries[0][0]); // default
		}


	}
}
