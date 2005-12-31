/*
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 2005 Dizzy
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

#include <stdexcept>
#include <fstream>
#include <sstream>

#include "compat/strcasecmp.h"
#include "compat/strerror.h"
#include "common/eventlog.h"
#include "common/systemerror.h"
#include "connection.h"
#include "common/setup_after.h"


namespace pvpgn
{

namespace bnetd
{

scoped_ptr<AdBannerComponent> adbannerlist;

AdBanner::AdBanner(unsigned id_, bn_int extag, unsigned delay_, unsigned next_, const std::string& fname, const std::string& link_, t_clienttag client_)
:id(id_), extensiontag(bn_int_get(extag)), delay(delay_), next(next_), filename(fname), link(link_), client(client_)
{
	/* I'm aware that this statement looks stupid */
	if (client && (!tag_check_client(client)))
		throw std::runtime_error("banner with invalid clienttag \"" + std::string(clienttag_uint_to_str(client)) + "\"encountered");

	eventlog(eventlog_level_debug,__FUNCTION__, "created ad id=0x%08x filename=\"%s\" extensiontag=0x%04x delay=%u link=\"%s\" next_id=0x%08x client=\"%s\"", id, filename.c_str(), extensiontag, delay, link.c_str(), next, client ? clienttag_uint_to_str(client) : "NULL");
}


AdBanner::~AdBanner() throw()
{
}

const AdBanner*
AdBannerComponent::find(t_clienttag ctag, unsigned id) const
{
	AdCtagMap::const_iterator cit(adlist.find(ctag));
	if (cit == adlist.end()) return 0;

	AdIdMap::const_iterator iit(cit->second.find(id));
	if (iit == cit->second.end()) return 0;

	return &(iit->second);
}

const AdBanner*
AdBannerComponent::pick(t_clienttag ctag, unsigned  prev_id) const
{
    /* eventlog(eventlog_level_debug,__FUNCTION__,"prev_id=%u init_count=%u start_count=%u norm_count=%u",prev_id,adbannerlist_init_count,adbannerlist_start_count,adbannerlist_norm_count); */
    /* if this is the first ad, randomly choose an init sequence (if there is one) */
	if (prev_id==0 && !adlist_init.empty())
		return findRandom(adlist_init, ctag);
//        return list_get_data_by_pos(adbannerlist_init_head,((unsigned int)std::rand())%adbannerlist_init_count);
    /* eventlog(eventlog_level_debug,__FUNCTION__,"not sending init banner"); */

	unsigned next_id = 0;
	const AdBanner* prev(find(ctag, prev_id));
	if (prev) next_id = prev->getNextId();

	/* return its next ad if there is one */
	if (next_id)
	{
		const AdBanner* curr(find(ctag, next_id));
		if (curr) return curr;
		ERROR1("could not locate next requested ad with id 0x%06x", next_id);
	}
    /* eventlog(eventlog_level_debug,__FUNCTION__,"not sending next banner"); */

    /* otherwise choose another starting point randomly */
	if (!adlist_start.empty())
		return findRandom(adlist_start, ctag);

	/* eventlog(eventlog_level_debug,__FUNCTION__,"not sending start banner... nothing to return"); */
	return NULL; /* nothing else to return */
}

unsigned
AdBanner::getId() const
{
	return id;
}

unsigned
AdBanner::getNextId() const
{
	return next;
}

unsigned
AdBanner::getExtensionTag() const
{
	return extensiontag;
}


char const *
AdBanner::getFilename() const
{
	return filename.c_str();
}


char const *
AdBanner::getLink() const
{
	return link.c_str();
}


t_clienttag
AdBanner::getClient() const
{
	return client;
}


const AdBanner*
AdBannerComponent::findRandom(const AdCtagRefMap& where, t_clienttag ctag) const
{
	/* first try to lookup a random banner into the specific client tag map */
	AdCtagRefMap::const_iterator cit(where.find(ctag));
	if (cit != where.end() && cit->second.size() > 0)
	{
		unsigned pos = (static_cast<unsigned>(std::rand())) % cit->second.size();
		/* TODO: optimize this linear search ? */
		for(AdIdRefMap::const_iterator it(cit->second.begin()); it != cit->second.end(); ++it)
		{
			if (!pos) return &(it->second->second);
			--pos;
		}
	}

	if (ctag != 0) return findRandom(where, 0);

	return 0;
}


void
AdBannerComponent::insert(AdCtagRefMap& where, const std::string& fname, unsigned id, unsigned delay, const std::string& link, unsigned next_id, const std::string& client)
{
	std::string::size_type idx(fname.rfind('.'));
	if (idx == std::string::npos || idx + 4 != fname.size())
	{
		ERROR1("Invalid extension for '%s'", fname.c_str());
		return;
	}

	std::string ext(fname.substr(idx + 1));

	bn_int bntag;
	if (strcasecmp(ext.c_str(), "pcx") == 0)
		bn_int_tag_set(&bntag, EXTENSIONTAG_PCX);
	else if (strcasecmp(ext.c_str(), "mng") == 0)
		bn_int_tag_set(&bntag, EXTENSIONTAG_MNG);
	else if (strcasecmp(ext.c_str(), "smk") == 0)
		bn_int_tag_set(&bntag, EXTENSIONTAG_SMK);
	else {
		ERROR1("unknown extension on filename \"%s\"", fname.c_str());
		return;
	}

	t_clienttag ctag;
	if (!strcasecmp(client.c_str(), "NULL"))
		ctag = 0;
	else
		ctag = clienttag_str_to_uint(client.c_str());

	AdCtagMap::iterator cit(adlist.find(ctag));
	if (cit == adlist.end())
	{
		std::pair<AdCtagMap::iterator, bool> res(adlist.insert(std::make_pair(ctag, AdIdMap())));
		if (!res.second)
			throw std::runtime_error("Could not insert unexistent element into map?!");
		cit = res.first;
	}

	std::pair<AdIdMap::iterator, bool> res(cit->second.insert(std::make_pair(id, AdBanner(id, bntag, delay, next_id, fname, link, ctag))));
	if (!res.second)
	{
		ERROR0("Couldnt insert new ad banner, duplicate banner IDs ?!");
		return;
	}

	AdCtagRefMap::iterator cit2(where.find(ctag));
	if (cit2 == where.end())
	{
		std::pair<AdCtagRefMap::iterator, bool> res2(where.insert(std::make_pair(ctag, AdIdRefMap())));
		if (!res2.second)
		{
			cit->second.erase(res.first);
			throw std::runtime_error("Could not insert unexistent element into map?!");
		}
		cit2 = res2.first;
	}

	if (!cit2->second.insert(std::make_pair(id, res.first)).second)
	{
		cit->second.erase(res.first);
		throw std::runtime_error("Could not insert unexistent element into map?!");
	}
}

AdBannerComponent::AdBannerComponent(const std::string& fname)
:adlist(), adlist_init(), adlist_start(), adlist_norm()
{
	std::ifstream fp(fname.c_str());
	if (!fp)
	{
		ERROR2("could not open adbanner file \"%s\" for reading (std::fopen: %s)", fname.c_str(), pstrerror(errno));
		throw SystemError("open");
	}

	unsigned delay;
	unsigned next_id;
	unsigned id;

	std::string name, when, link, client;
	std::string buff;
	for(unsigned line = 1; std::getline(fp, buff); ++line)
	{
		std::string::size_type idx(buff.find('#'));
		if (idx != std::string::npos) buff.erase(idx);

		idx = buff.find_first_not_of(" \t");
		if (idx == std::string::npos) continue;

		std::istringstream is(buff);

		is >> name;
		is >> id;
		is >> when;
		is >> delay;
		is >> link;
		is >> std::hex >> next_id;
		is >> client;

		if (!is || name.size() < 2 || link.size() < 2 || client.size() < 2 || id < 1)
		{
			ERROR2("malformed line %u in file \"%s\"", line, fname.c_str());
			continue;
		}

		name.erase(0, 1);
		name.erase(name.size() - 1);
		link.erase(0, 1);
		link.erase(link.size() - 1);
		client.erase(0, 1);
		client.erase(client.size() - 1);

		if (when == "init")
			insert(adlist_init, name, id, delay, link, next_id, client);
		else if (when == "start")
			insert(adlist_start, name, id, delay, link, next_id, client);
		else if (when == "norm")
			insert(adlist_norm, name, id, delay, link, next_id, client);
		else
/*
			ERROR4("when field has unknown value on line %u in file \"%s\": \"%s\" when == init: %d", line, fname.c_str(), when.c_str(), when == "init");
*/
			eventlog(eventlog_level_error, __FUNCTION__, "when field has unknown value on line %u in file \"%s\": \"%s\" when == init: %d", line, fname.c_str(), when.c_str(), when == "init");
	}
}


AdBannerComponent::~AdBannerComponent() throw()
{}

}

}
