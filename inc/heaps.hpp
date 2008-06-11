#ifndef HEAPS_H
#define HEAPS_H
#include<memory>
#include<boost/scoped_ptr.hpp>
#include<boost/shared_ptr.hpp>
#include<vector>
#include"types.hpp"

class Heap;

class Generic;

class Semispace{
private:
	void* mem;
	void* allocpt;
	size_t prev_alloc;
	size_t max;
public:
	Semispace(size_t);
	~Semispace();
	void* alloc(size_t);
	void dealloc(void*);
	void resize(size_t);
	bool can_fit(size_t) const;

	size_t size(void) const {return max;};
	size_t used(void) const {
		return (size_t)(((char*) allocpt) - ((char*) mem));};

	friend class Heap;
};

class Heap{
private:
	boost::scoped_ptr<Semispace> s;
	std::vector<boost::shared_ptr<Semispace> > other_spaces;
	bool tight;
protected:
	virtual void get_root_set(std::stack<Generic**>&) =0;
	size_t get_total_heap_size(void) const;
	void GC(size_t);
public:
	void* alloc(size_t);
	void dealloc(void*);
	Heap(void) : s(new Semispace(64)), tight(0) {};
};

inline void* operator new(size_t n, Semispace& sp){
	return sp.alloc(n);
}
/*NOTE: this delete operator should only be called if a
ctor throws.  We should not be performing a delete on any
objects in a Semispace/heap.
*/
inline void operator delete(void* p, Semispace& sp){
	/*we can only deallocate the most recent allocation*/
	/*necessary for proper cleanup in case the ctor throws*/
	sp.dealloc(p);
}
inline void* operator new(size_t n, Heap& hp){
	return hp.alloc(n);
}
inline void operator delete(void* p, Heap& hp){
	/*we can only deallocate the most recent allocation*/
	/*necessary for proper cleanup in case the ctor throws*/
	hp.dealloc(p);
}

#endif //HEAPS_H

