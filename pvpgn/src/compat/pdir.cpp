/*
 * Copyright (C) 2001  Dizzy
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

Directory::Directory(const std::string& path_)
:path(path_)
{
#ifdef WIN32
	if (path.size() + 1 + 3 >= _MAX_PATH)
		throw std::runtime_error("pvpgn::Directory::Directory(): WIN32: path too long");
	path += "/*.*";

	status = 0;
	std::memset(&fileinfo, 0, sizeof(fileinfo));
	lFindHandle = _findfirst(path.c_str(), &fileinfo);
	if (lFindHandle < 0)
		throw OpenError(path);
#else /* POSIX style */
	if (!(dir=opendir(path.c_str())))
		throw OpenError(path);
#endif /* WIN32-POSIX */
}

Directory::~Directory() throw()
{
#ifdef WIN32
	if (status!=-1)
		_findclose(pdir->lFindHandle);
#else /* POSIX */
	closedir(dir);
#endif /* WIN32-POSIX */
}

void
Directory::rewind()
{
#ifdef WIN32
	/* i dont have any win32 around so i dont know if io.h has any rewinddir equivalent */
	/* FIXME: for the time being ill just close and reopen it */
	if (status!=-1) {
		_findclose(lFindHandle);
	}
	status = 0;
	std::memset(&fileinfo, 0, sizeof(fileinfo));
	lFindHandle = _findfirst(path.c_str(), &fileinfo);
	if (lFindHandle < 0) {
		ERROR0("WIN32: couldn't rewind directory");
		status = -1;
	}
#else /* POSIX */
	rewinddir(dir);
#endif
}


char const *
Directory::read()
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
		result = pdir->fileinfo.name;
		break;
	case 1: /* reading */
		if (_findnext(pdir->lFindHandle, &pdir->fileinfo)<0) {
			status = 2;
			return 0;
		} else result = pdir->fileinfo.name;
		break;
	case 2: /* EOF */
		return 0;
	}
#else /* POSIX */
	struct dirent *dentry = readdir(dir);
	if (!dentry) return 0;

	result = dentry->d_name;
#endif /* WIN32-POSIX */

	if (result && !(strcmp(result, ".") && strcmp(result, "..")))
		/* here we presume we don't get an infinite number of "." or ".." ;) */
		return read();
	return result;
}

}
