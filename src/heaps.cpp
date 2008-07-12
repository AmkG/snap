/*
*/
#include"heaps.hpp"
#include<cstring>

/*-----------------------------------------------------------------------------
Semispaces
-----------------------------------------------------------------------------*/

static ptrdiff_t moveallreferences(void* mem, void* allocpt, void* nmem){
	char* cmem = (char*)mem; char* cnmem = (char*)nmem;
	ptrdiff_t diff = cnmem - cmem;
	char* callocpt = (char*)allocpt;
	void* nallocpt = callocpt + diff;
	std::stack<Generic**> to_check;
	Generic* cur;
	Generic** gpp;
	char* i;
	for(i = cnmem;
	    i < nallocpt;
	    i += ((Generic*)(void*) i)->get_size()){
		cur = (Generic*)i;
		cur->get_refs(to_check);
		while(!to_check.empty()){
			gpp = to_check.top(); to_check.pop();
			if(mem <= *gpp && *gpp < allocpt){
				char* cgp = (char*)(void*) *gpp;
				cgp += diff;
				*gpp = (Generic*)(void*) cgp;
			}
		}
	}
	return diff;
}

/*should be used only to make the semispace smaller*/
void Semispace::resize(size_t nsize){
	void* nmem = realloc(mem, nsize);
	if(nmem == NULL) throw std::bad_alloc();
	/*shouldn't happen if resized to smaller, but might*/
	if(nmem != mem){
		ptrdiff_t diff = moveallreferences(mem,allocpt,nmem);
		mem = ((char*)mem) + diff;
		allocpt = ((char*)allocpt) + diff;
	}
	max = nsize;
}

/*potentially unsafe; if something in this ctor
throws an exception, then the malloc'ed mem might
not be properly freed.  Maybe use a scoped_ptr
or other smart pointer for mem instead?
*/
Semispace::Semispace(size_t sz) : prev_alloc(0), max(sz), mem(malloc(sz)){
	allocpt = mem;
}

Semispace::~Semispace(){
	Generic* gp;
	char* i;
	size_t sz;
	/*Call the dtors for each object*/
	for(i = (char*)mem; i < allocpt; i += sz){
		gp = (Generic*)(void*)i;
		sz = gp->get_size(); // do this before destructing!
		gp->~Generic(); // should be dynamically overloaded...
	}
	free(mem);
}

void* Semispace::alloc(size_t sz){
	/*assertion, shouldn't be necessary...*/
	if(!can_fit(sz)) throw std::bad_alloc();
	char* s = (char*)allocpt;
	char* n = s + sz;
	prev_alloc = sz;
	allocpt = (void*) n;
	return (void*) s;
}

/*NOTE!  We can only dealloc the most recently
alloc'ed memory.  This shouldn't happen otherwise,
since only an error in a constructor should
trigger a call to this, i.e. only an operator
delete(void*, Semispace&).  Obviously we also
shouldn't attempt to delete an object managed by a
semispace... Another rule, to prevent multiple
dealloc's in succession, is to never allocate on a
semispace while constructing an Arc object.
*/
void Semispace::dealloc(void* check){
	char* s = (char*)allocpt;
	char* n = s - prev_alloc;
	if(n != (char*) check) throw std::bad_alloc();
	allocpt = (void*) n;
	prev_alloc = 0; //prevent further deallocs
}

bool Semispace::can_fit(size_t sz) const{
	char* m = (char*) mem;
	char* a = (char*) allocpt;
	return (m + max) >= (a + sz);
}

std::pair<boost::shared_ptr<Semispace>, Generic* >
		Semispace::clone(Generic* o) const {
	boost::shared_ptr<Semispace> ns(new Semispace(size()));
	/*make a copy. note that we must use Generic::clone() because
	any boost::shared_ptr's in the object will have special semantics
	*/
	Generic* gp;
	for(char* ip = (char*) mem; ip < allocpt; ip += gp->get_size()){
		gp = (Generic*)(void*)ip;
		gp->clone(*ns);
	}
	/*now shift each pointer in the new semispace*/
	ptrdiff_t diff = moveallreferences(mem,allocpt,ns->mem);
	ns->allocpt = ((char*)allocpt) + diff;
	o = (Generic*)(void*)(((char*)(void*)o) + diff);
	return std::pair<boost::shared_ptr<Semispace>, Generic*>(
		ns, o);
}

/*-----------------------------------------------------------------------------
Heaps
-----------------------------------------------------------------------------*/

/*preconditions:
other_spaces must be locked!
*/
size_t Heap::get_total_heap_size(void) const{
	size_t sz = s->size();
	std::vector<boost::shared_ptr<Semispace> >::const_iterator i;
	for(i = other_spaces.begin(); i != other_spaces.end(); ++i){
		sz += (*i)->size();
	}
	return sz;
}

#include<cstdio>

/*used internally by GC*/
static void copy_set(std::stack<Generic**>& tocopy,
		ToPointerLock& toptrs,
		Semispace& ns){
	Generic** gpp;
	Generic* gp;
	Generic* to;
	while(!tocopy.empty()){
		gpp = tocopy.top(); tocopy.pop();
		gp = *gpp;
		to = toptrs.to(gp);
		if(to == NULL || to == gp){
			to = gp->clone(ns);
			toptrs.pointto(gp, to);
			to->get_refs(tocopy);
		}
		*gpp = to;
	}
}

/*explicit recursion copy*/
void Heap::GC(size_t insurance){
	/*insert mutex locking of other_spaces here*/

	size_t sz = get_total_heap_size() + insurance;
	if(tight){ sz = sz * 2;}

	boost::scoped_ptr<Semispace> ns(new Semispace(sz));
	ToPointerLock toptrs;
	std::stack<Generic**> tocopy;

	get_root_set(tocopy);
	copy_set(tocopy, toptrs, *ns);

	s.swap(ns); //shouldn't throw
	/*successful GC, don't bother clearing*/
	toptrs.good();
	other_spaces.clear();
	ns.reset();

	/*determine if resizing necessary*/
	size_t used = s->used() + insurance;
	tight = 0;
	if(used <= sz / 4){
		s->resize(sz / 2);
	} else if(used >= (sz * 3) / 4){
		tight = 1;
	}
}

void* Heap::alloc(size_t sz){
	if(!s->can_fit(sz)){
		GC(sz);
		if(!s->can_fit(sz)){
			throw std::bad_alloc();
		}
	}
	return s->alloc(sz);
}
void Heap::dealloc(void* check){
	s->dealloc(check);
}

boost::shared_ptr<Semispace> Heap::to_new_semispace(Generic*& gp) const {
	/*Determine the size*/
	ToPointerLock toptrs;
	std::stack<Generic**> tocopy;
	size_t nsz = gp->total_size(toptrs, tocopy);
	/*create a new semispace for the message, then
	copy the entire message to the new semispace
	*/
	boost::shared_ptr<Semispace> ns(new Semispace(nsz));

	gp->get_refs(tocopy);
	tocopy.push(&gp);
	copy_set(tocopy, toptrs, *ns);

	return ns;
	/*let ToPointerLock clean up to-pointers*/
}

