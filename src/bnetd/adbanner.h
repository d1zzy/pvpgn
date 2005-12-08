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
#include "common/tag.h"

#ifndef INCLUDED_ADBANNER_TYPES
#define INCLUDED_ADBANNER_TYPES

#include <string>

namespace pvpgn
{

class AdBanner
{
public:
	AdBanner(unsigned id_, bn_int extag, unsigned delay_, unsigned next_, const std::string& fname, const std::string& link_, const char* clientstr);
	~AdBanner() throw();

	unsigned getId() const;
	unsigned getNextId() const;
	unsigned getExtensionTag() const;
	char const * getFilename() const;
	char const * getLink() const;
	t_clienttag getClient() const;

private:
	unsigned id;
	unsigned extensiontag;
	unsigned delay; /* in seconds */
	unsigned next; /* adid or 0 */
	const std::string filename;
	const std::string link;
	t_clienttag client;
};

}

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_ADBANNER_PROTOS
#define INCLUDED_ADBANNER_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#undef JUST_NEED_TYPES

namespace pvpgn
{

extern int adbannerlist_create(char const * filename);
extern int adbannerlist_destroy(void);
extern const AdBanner* adbannerlist_pick(t_clienttag ctag, unsigned int prev_id);
extern const AdBanner* adbannerlist_find(t_clienttag ctag, unsigned int id);

}
#endif
#endif
