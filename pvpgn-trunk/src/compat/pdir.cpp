/*
 * Copyright (C) 2001,2006  Dizzy
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
#define PDIR_INTERNAL_ACCESS
#include "common/setup_before.h"
#include "pdir.h"

#include <cstring>

#include "common/eventlog.h"
#include "common/setup_after.h"


namespace pvpgn
{

Directory::Directory(const std::string& path_, bool lazyread_)
{
	open(path_, lazyread_);
}

Directory::~Directory() throw()
{
	close();
}

void
Directory::close()
{
#ifdef WIN32
	if (lFindHandle >= 0)
		_findclose(lFindHandle);
#else /* POSIX */
	if (dir) closedir(dir);
#endif /* WIN32-POSIX */
}

void
Directory::open(const std::string& path_, bool lazyread_)
{
#ifdef WIN32
	std::string tmp(path_);

	if (tmp.size() + 1 + 3 >= _MAX_PATH)
		throw std::runtime_error("pvpgn::Directory::Directory(): WIN32: path too long");
	tmp += "/*.*";

	status = 0;
	std::memset(&fileinfo, 0, sizeof(fileinfo));
	lFindHandle = _findfirst(tmp.c_str(), &fileinfo);
	if (lFindHandle < 0 && !lazyread_)
		throw OpenError(tmp);

	path = tmp;
#else /* POSIX style */
	if (!(dir=opendir(path_.c_str())) && !lazyread_)
		throw OpenError(path);
	path = path_;
#endif /* WIN32-POSIX */

	lazyread = lazyread_;
}

void
Directory::rewind()
{
#ifdef WIN32
	close();
	status = 0;
	std::memset(&fileinfo, 0, sizeof(fileinfo));
	lFindHandle = _findfirst(path.c_str(), &fileinfo);
	if (lFindHandle < 0 && !lazyread) {
		ERROR0("WIN32: couldn't rewind directory");
		status = -1;
	}
#else /* POSIX */
	if (dir) rewinddir(dir);
#endif
}


char const *
Directory::read() const
{
	const char * result;

#ifdef WIN32
	switch (status) {
	default:
	case -1: /* couldn't rewind */
		ERROR0("got status -1");
		return 0;
	case 0: /* freshly opened */
		status = 1;
		if (lFindHandle < 0) return 0;
		result = fileinfo.name;
		break;
	case 1: /* reading */
		if (lFindHandle < 0) return 0;

		if (_findnext(lFindHandle, &fileinfo)<0) {
			status = 2;
			return 0;
		} else result = fileinfo.name;
		break;
	case 2: /* EOF */
		return 0;
	}
#else /* POSIX */
	struct dirent *dentry = dir ? readdir(dir) : 0;
	if (!dentry) return 0;

	result = dentry->d_name;
#endif /* WIN32-POSIX */

	if (!(strcmp(result, ".") && strcmp(result, "..")))
		/* here we presume we don't get an infinite number of "." or ".." ;) */
		return read();
	return result;
}

Directory::operator bool() const
{
#ifdef WIN32
	return lFindHandle >= 0;
#else
	return dir != 0;
#endif
}

}
