#ifndef HEAPS_H
#define HEAPS_H
#include<memory>
#include<boost/scoped_ptr.hpp>
#include<boost/shared_ptr.hpp>
#include<vector>
#include<utility>
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

	std::pair<boost::shared_ptr<Semispace>, Generic* >
		clone(Generic*) const;

	friend class Heap;
};

/*last-in-first-out allocation semispace*/
class LifoSemispace {
private:
	void* mem;
	void* allocpt;
	void* end;
	size_t prevalloc;
public:
	LifoSemispace(size_t sz);
	~LifoSemispace();
	void* alloc(size_t sz);
	void dealloc(void*); // used only in a constructor-fail delete
	void normal_dealloc(Generic*);
	bool can_fit(size_t) const;
	size_t size(void) const {
		return (size_t) (((char*) end) - ((char*) mem));
	}
	size_t used(void) const {
		return (size_t) (((char*) end) - ((char*) allocpt));
	}

	/*no need to clone LIFO semispaces: they never get passed around*/
	friend class Heap;
};

class LifoHeap;

class Heap{
private:
	boost::scoped_ptr<Semispace> s;
	boost::scoped_ptr<LifoSemispace> ls;
	bool tight;
protected:
	/*conceptually these should be scoped_ptr's, but
	unfortunately you can't put them in standard
	containers; we could create our own container type
	though (immutable singly-linked lists, or even
	unrolled lists would probably be safest)
	*/
	std::vector<boost::shared_ptr<Semispace> > other_spaces;
	virtual void get_root_set(std::stack<Generic**>&) =0;
	size_t get_total_heap_size(void) const;
	void GC(size_t,bool from_lifo=0);
public:
	void* alloc(size_t);
	void dealloc(void*);
	boost::shared_ptr<Semispace> to_new_semispace(Generic*&) const;
	Heap(void);
	virtual ~Heap(){};

	void* lifo_alloc(size_t);
	void lifo_dealloc(void*);
	void lifo_normal_dealloc(Generic*);

	LifoHeap lifo(void);

};

class LifoHeap {
private:
	Heap* hp;
public:
	LifoHeap(Heap* nhp) : hp(nhp) {}

	void* alloc(size_t sz){return hp->lifo_alloc(sz);}
	void dealloc(void* pt){return hp->lifo_dealloc(pt);}
	void normal_dealloc(Generic* gp){return hp->lifo_normal_dealloc(gp);}
};

inline Heap::Heap(void)
	: s(new Semispace(64)),
	tight(0),
	ls(new LifoSemispace(64)) {}

inline LifoHeap Heap::lifo(void){
	return LifoHeap(this);
}

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
inline void* operator new(size_t n, LifoHeap hp){
	return hp.alloc(n);
}
inline void operator delete(void* p, LifoHeap hp){
	hp.dealloc(p);
}

#endif //HEAPS_H

