/*
 * Copyright (C) 2004 Dizzy
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
#ifndef XALLOC_SKIP

#include "compat/strdup.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#define XALLOC_INTERNAL_ACCESS
#include "common/setup_after.h"
#undef XALLOC_INTERNAL_ACCESS

namespace pvpgn
{

static t_oom_cb oom_cb = NULL;

void *xmalloc_real(size_t size, const char *fn, unsigned ln)
{
    void *res;

    res = malloc(size);
    if (!res) {
	eventlog(eventlog_level_fatal, __FUNCTION__, "out of memory (from %s:%u)",fn,ln);
	if (oom_cb && oom_cb() && (res = malloc(size))) return res;
	abort();
    }

    return res;
}

void *xcalloc_real(size_t nmemb, size_t size, const char *fn, unsigned ln)
{
    void *res;

    res = calloc(nmemb,size);
    if (!res) {
	eventlog(eventlog_level_fatal, __FUNCTION__, "out of memory (from %s:%u)",fn,ln);
	if (oom_cb && oom_cb() && (res = calloc(nmemb,size))) return res;
	abort();
    }

    return res;
}

void *xrealloc_real(void *ptr, size_t size, const char *fn, unsigned ln)
{
    void *res;

    res = realloc(ptr,size);
    if (!res) {
	eventlog(eventlog_level_fatal, __FUNCTION__, "out of memory (from %s:%u)",fn,ln);
	if (oom_cb && oom_cb() && (res = realloc(ptr,size))) return res;
	abort();
    }

    return res;
}

char *xstrdup_real(const char *str, const char *fn, unsigned ln)
{
    char *res;

    res = strdup(str);
    if (!res) {
	eventlog(eventlog_level_fatal, __FUNCTION__, "out of memory (from %s:%u)",fn,ln);
	if (oom_cb && oom_cb() && (res = strdup(str))) return res;
	abort();
    }

    return res;
}

void xfree_real(void *ptr, const char *fn, unsigned ln)
{
    if (!ptr) {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL ptr (from %s:%u)",fn,ln);
	return;
    }

    free(ptr);
}

void xalloc_setcb(t_oom_cb cb)
{
    oom_cb = cb;
}

}

#endif /* XALLOC_SKIP */
