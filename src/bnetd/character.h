/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_CHARACTER_TYPES
#define INCLUDED_CHARACTER_TYPES

#include <cstdint>

namespace pvpgn
{

	namespace bnetd
	{

		typedef enum
		{
			character_class_none,
			character_class_amazon,
			character_class_sorceress,
			character_class_necromancer,
			character_class_paladin,
			character_class_barbarian,
			character_class_druid,
			character_class_assassin
		} t_character_class;

		typedef enum
		{
			character_expansion_none,
			character_expansion_classic,
			character_expansion_lod
		} t_character_expansion;

		typedef struct character
#ifdef CHARACTER_INTERNAL_ACCESS
		{
			char const * name; /* max 15 chars */
			char const * realmname;
			char const * guildname; /* max 3 chars */

			/* stored in Battle.net format for now */
			std::uint8_t      helmgfx;
			std::uint8_t      bodygfx;
			std::uint8_t      leggfx;
			std::uint8_t      lhandweapon;
			std::uint8_t      lhandgfx;
			std::uint8_t      rhandweapon;
			std::uint8_t      rhandgfx;
			std::uint8_t      chclass;
			std::uint8_t      level;
			std::uint8_t      status;
			std::uint8_t      title;
			std::uint8_t      emblembgc;
			std::uint8_t      emblemfgc;
			std::uint8_t      emblemnum;

			/* not sure what these represent */
			std::uint32_t     unknown1;
			std::uint32_t     unknown2;
			std::uint32_t     unknown3;
			std::uint32_t     unknown4;
			std::uint8_t      unknownb1;
			std::uint8_t      unknownb2;
			std::uint8_t      unknownb3;
			std::uint8_t      unknownb4;
			std::uint8_t      unknownb5;
			std::uint8_t      unknownb6;
			std::uint8_t      unknownb7;
			std::uint8_t      unknownb8;
			std::uint8_t      unknownb9;
			std::uint8_t      unknownb10;
			std::uint8_t      unknownb11;
			std::uint8_t      unknownb13;
			std::uint8_t      unknownb14;

			/* Keep some generic "data", basically the blob that is sent between client and server, in an array */
			std::uint8_t      data[64];
			std::uint8_t      datalen;
		}
#endif
		t_character;

	}

}

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_CHARACTER_PROTOS
#define INCLUDED_CHARACTER_PROTOS

#define JUST_NEED_TYPES
#include "account.h"
#undef JUST_NEED_TYPES

namespace pvpgn
{

	namespace bnetd
	{

		extern int character_create(t_account * account, t_clienttag clienttag, char const * realmname, char const * name, t_character_class chclass, t_character_expansion expansion);
		extern char const * character_get_name(t_character const * ch);
		extern char const * character_get_realmname(t_character const * ch);
		extern char const * character_get_playerinfo(t_character const * ch);
		extern char const * character_get_guildname(t_character const * ch);
		extern t_character_class character_get_class(t_character const * ch);
		extern int character_verify_charlist(t_character const * ch, char const * charlist);

		extern int characterlist_create(char const * dirname);
		extern int characterlist_destroy(void);
		extern t_character * characterlist_find_character(char const * realmname, char const * charname);

	}

}

#endif
#endif
