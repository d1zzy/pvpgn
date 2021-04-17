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

#include "common/setup_before.h"
#include "friends.h"

#include <new>

#include "compat/strcasecmp.h"
#include "common/list.h"
#include "common/eventlog.h"

#include "common/setup_after.h"

namespace pvpgn
{

	namespace bnetd
	{

		extern t_account * friend_get_account(t_friend * fr)
		{
			if (fr == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL friend");
				return nullptr;
			}

			return fr->friendacc;
		}

		extern int friend_set_account(t_friend * fr, t_account * acc)
		{
			if (fr == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL friend");
				return -1;
			}

			if (acc == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL account");
				return -1;
			}

			fr->friendacc = acc;
			return 0;
		}

		extern char friend_get_mutual(t_friend * fr)
		{
			if (fr == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL friend");
				return 0;
			}

			return fr->mutual;
		}

		extern int friend_set_mutual(t_friend * fr, int mutual)
		{
			if (fr == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL friend");
				return -1;
			}

			if (mutual != FRIEND_UNLOADEDMUTUAL && mutual != FRIEND_NOTMUTUAL && mutual != FRIEND_ISMUTUAL)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got invalid mutual status");
				return -1;
			}

			fr->mutual = mutual;
			return 0;
		}

		extern int friendlist_unload(t_list * flist)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return -1;
			}

			t_friend * fr = nullptr;
			t_elem  * curr = nullptr;
			LIST_TRAVERSE(flist, curr)
			{
				fr = (t_friend*)elem_get_data(curr);
				if (fr == nullptr)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
					continue;
				}
				fr->mutual = FRIEND_UNLOADEDMUTUAL;
			}

			return 0;
		}

		extern int friendlist_close(t_list * flist)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_trace, __FUNCTION__, "got NULL flist");
				return -1;
			}

			t_friend * fr = nullptr;
			t_elem * curr = nullptr;
			LIST_TRAVERSE(flist, curr)
			{
				fr = (t_friend*)elem_get_data(curr);
				if (fr == nullptr)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
					continue;
				}

				if (list_remove_elem(flist, &curr) != 0)
					eventlog(eventlog_level_error, __FUNCTION__, "could not remove elem from flist");
				delete(fr);
			}

			list_destroy(flist);

			return 0;
		}

		extern int friendlist_purge(t_list * flist)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return -1;
			}

			t_friend * fr = nullptr;
			t_elem  * curr = nullptr;
			LIST_TRAVERSE(flist, curr)
			{
				fr = (t_friend*)elem_get_data(curr);
				if (fr == nullptr)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
					continue;
				}
				if (fr->mutual == FRIEND_UNLOADEDMUTUAL)
				{
					if (list_remove_elem(flist, &curr) != 0)
						eventlog(eventlog_level_error, __FUNCTION__, "could not remove item from list");
				}
			}
			return 0;
		}

		extern int friendlist_add_account(t_list * flist, t_account * acc, int mutual)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return -1;
			}

			if (acc == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL account");
				return -1;
			}

			if (mutual != FRIEND_UNLOADEDMUTUAL && mutual != FRIEND_NOTMUTUAL && mutual != FRIEND_ISMUTUAL)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got invalid mutual status");
				return -1;
			}

			t_friend * fr = new t_friend; //(t_friend*)xmalloc(sizeof(t_friend));
			fr->friendacc = acc;
			fr->mutual = mutual;
			list_append_data(flist, fr);

			return 0;
		}

		extern int friendlist_remove_friend(t_list * flist, t_friend * fr)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return -1;
			}

			if (fr == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL friend");
				return -1;
			}

			t_elem * elem = nullptr;
			if (list_remove_data(flist, fr, &elem) != 0)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "could not remove item from list");
				return -1;
			}

			delete(fr);
			return 0;
		}

		extern int friendlist_remove_account(t_list * flist, t_account * acc)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return -1;
			}

			if (acc == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL account");
				return -1;
			}

			t_friend * fr = friendlist_find_account(flist, acc);
			if (fr == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "could not find friend");
				return -1;
			}

			t_elem * elem = nullptr;
			if (list_remove_data(flist, fr, &elem) != 0)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "could not remove item from list");
				return -1;
			}

			delete(fr);
			return 0;
		}

		extern int friendlist_remove_username(t_list * flist, const char * accname)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return -1;
			}

			if (accname == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL account name");
				return -1;
			}

			t_friend * fr = friendlist_find_username(flist, accname);
			if (fr == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "could not find friend");
				return -1;
			}

			t_elem * elem = nullptr;
			if (list_remove_data(flist, fr, &elem) != 0)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "could not remove item from list");
				return -1;
			}

			delete(fr);
			return 0;
		}

		extern t_friend * friendlist_find_account(t_list * flist, t_account * acc)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return nullptr;
			}

			if (acc == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL account");
				return nullptr;
			}

			t_friend * fr = nullptr;
			t_elem * curr = nullptr;
			LIST_TRAVERSE(flist, curr)
			{
				fr = (t_friend*)elem_get_data(curr);
				if (fr == nullptr)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
					continue;
				}

				if (fr->friendacc == acc)
					return fr;
			}

			return nullptr;
		}

		extern t_friend * friendlist_find_username(t_list * flist, const char * accname)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return nullptr;
			}

			if (accname == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL account name");
				return nullptr;
			}

			t_friend * fr = nullptr;
			t_elem * curr = nullptr;
			LIST_TRAVERSE(flist, curr)
			{
				fr = (t_friend*)elem_get_data(curr);
				if (fr == nullptr)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
					continue;
				}

				if (strcasecmp(account_get_name(fr->friendacc), accname) == 0)
					return fr;
			}

			return nullptr;
		}

		extern t_friend * friendlist_find_uid(t_list * flist, unsigned int uid)
		{
			if (flist == nullptr)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got NULL flist");
				return nullptr;
			}

			t_friend * fr = nullptr;
			t_elem * curr = nullptr;
			LIST_TRAVERSE(flist, curr)
			{
				fr = (t_friend*)elem_get_data(curr);
				if (fr == nullptr)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
					continue;
				}

				if (account_get_uid(fr->friendacc) == uid)
					return fr;
			}
			return nullptr;
		}

	}

}
