#ifndef PVPGNSCOPED_PTR_H
#define PVPGNSCOPED_PTR_H

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

	/** get the wrapped array pointer */
	T* get() const { return ptr; }

	/** release ownership of the array */
	T* release() {
		T* tmp = ptr;
		ptr = 0;
		return tmp;
	}

	/** reinitilize object, release owned resource first if any */
	void reset(T* ptr_) {
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

	/* do not allow to copy a scoped_array */
	scoped_ptr(const scoped_ptr&);
	scoped_ptr& operator=(const scoped_ptr&);

	void cleanup() {
		if (ptr) delete ptr;
	}
};

}

#endif
