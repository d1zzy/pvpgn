/*
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

#ifndef PVPGN_SCOPED_PTR_H
#define PVPGN_SCOPED_PTR_H

namespace pvpgn
{

template<typename T>
class scoped_ptr
{
public:
	/** initilize the object aquiring ownership of the given parameter (0 for no onwership) */
	explicit scoped_ptr(T* ptr_ = 0)
	:ptr(ptr_) {}
	/** release memory if aquired ownershipt */
	~scoped_ptr() throw() {
		cleanup();
	}

	/** get the wrapped pointer */
	T* get() const { return ptr; }

	/** release ownership of the memory */
	T* release() {
		T* tmp = ptr;
		ptr = 0;
		return tmp;
	}

	/** reinitilize object, release owned resource first if any */
	void reset(T* ptr_ = 0) {
		cleanup();
		ptr = ptr_;
	}

	const T& operator*() const {
		return *ptr;
	}

	T& operator*() {
		return *ptr;
	}

	const T* operator->() const {
		return ptr;
	}

	T* operator->() {
		return ptr;
	}

private:
	T* ptr;

	/* do not allow to copy a scoped_ptr */
	scoped_ptr(const scoped_ptr&);
	scoped_ptr& operator=(const scoped_ptr&);

	void cleanup() {
		if (ptr) delete ptr;
	}
};

}

#endif
