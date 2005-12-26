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

#include <string>
#include <map>

#include "common/tag.h"
#include "common/scoped_ptr.h"
#include "common/bn_type.h"

namespace pvpgn
{

namespace bnetd
{

class AdBanner
{
public:
	AdBanner(unsigned id_, bn_int extag, unsigned delay_, unsigned next_, const std::string& fname, const std::string& link_, t_clienttag client_);
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

class AdBannerComponent
{
public:
	explicit AdBannerComponent(const std::string& fname);
	~AdBannerComponent() throw();

	const AdBanner* pick(t_clienttag ctag, unsigned prev_id) const;
	const AdBanner* find(t_clienttag ctag, unsigned id) const;

private:
	typedef std::map<unsigned, AdBanner> AdIdMap;
	typedef std::map<t_clienttag, AdIdMap> AdCtagMap;
	typedef std::map<unsigned, AdIdMap::const_iterator> AdIdRefMap;
	typedef std::map<t_clienttag, AdIdRefMap> AdCtagRefMap;

	AdCtagMap adlist;
	AdCtagRefMap adlist_init;
	AdCtagRefMap adlist_start;
	AdCtagRefMap adlist_norm;

	const AdBanner* findRandom(const AdCtagRefMap& where, t_clienttag ctag) const;
	void insert(AdCtagRefMap& where, const std::string& fname, unsigned id, unsigned delay, const std::string& link, unsigned next_id, const std::string& client);
};

extern scoped_ptr<AdBannerComponent> adbannerlist;

}

}

#endif
