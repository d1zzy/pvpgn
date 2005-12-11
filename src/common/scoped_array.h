#ifndef PVPGNSCOPED_ARRAY_H
#define PVPGNSCOPED_ARRAY_H

namespace pvpgn
{

template<typename T>
class scoped_array
{
public:
	/** initilize the object aquiring ownership of the given parameter (0 for no onwership) */
	explicit scoped_array(T* ptr_ = 0)
	:ptr(ptr_) {}
	/** release memory if aquired ownershipt */
	~scoped_array() throw() {
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

	const T& operator[](unsigned idx) const {
		return ptr[idx];
	}

	T& operator[](unsigned idx) {
		return ptr[idx];
	}

private:
	T* ptr;

	/* do not allow to copy a scoped_array */
	scoped_array(const scoped_array&);
	scoped_array& operator=(const scoped_array&);

	void cleanup() {
		if (ptr) delete[] ptr;
	}
};

}

#endif
