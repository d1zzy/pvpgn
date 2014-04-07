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
#ifndef INCLUDED_ICONS_TYPES
#define INCLUDED_ICONS_TYPES

namespace pvpgn
{

	namespace bnetd
	{

		typedef struct
		{
			unsigned int rating;	// rating or something else
			char * rank;			// any string value
			char * icon_code;		// icon code
		} t_icon_info;

		typedef struct
		{
			char * clienttag;
			char * attr_key;
			t_list * icon_info;		// list of t_icon_info

			t_list * vars;		// list of t_icon_var_info
			const char * stats;
		} t_iconset_info;

		typedef struct
		{
			char * key;	
			char * value;
		} t_icon_var_info;
	}

}

#endif

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_ICONS_PROTOS
#define INCLUDED_ICONS_PROTOS

#define JUST_NEED_TYPES
# include "account.h"
#undef JUST_NEED_TYPES

namespace pvpgn
{

	namespace bnetd
	{
		extern int prefs_get_custom_icons();
		extern t_icon_info * get_custom_icon(t_account * account, t_clienttag clienttag);
		extern const char * get_custom_stats_text(t_account * account, t_clienttag clienttag);


		extern int customicons_load(char const * filename);
		extern int customicons_unload(void);

	}

}

/*****/
#endif
#endif
