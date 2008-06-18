#ifndef TYPES_H
#define TYPES_H
#include<stack>
#include<set>
#include<vector>
#include"heaps.hpp"
#include"hashes.hpp"
#include"variables.hpp"

class Semispace;
void* operator new(size_t, Semispace&);
void operator delete(void*, Semispace&);

class ToPointerLock;

class Generic {
private:
	Generic* to_pointer;
protected:
	Generic(void) : to_pointer(NULL) {};
	Generic(Generic const&) : to_pointer(NULL) {};
public:
	friend class Semispace;
	friend class ToPointerLock;

	/*hash should return pretty much just any
	size_t, provided that an unmutated copy of
	that object will have the same hash().
	There is NO NEED TO PROPERLY DISTRIBUTE BITS,
	and it would probably be better not to: the
	table lookup function will use several int-
	to-int hashing functions for the bloom
	filter.
	*/
	virtual size_t hash(void) const =0;
	virtual Generic* clone(Semispace&) const =0;
	/* clone should always have the following code:
	virtual Type* clone(Semispace& sp) const{
		return new(sp) Type(*this);
	}
	*/

	/*gets addresses of references to other objects*/
	virtual void get_refs(std::stack<Generic**>&){};

	/*comparisons*/
	virtual bool is(Generic const* gp) const{return gp == this;}
	virtual bool iso(Generic const* gp) const{return is(gp);}

	virtual size_t get_size(void) const =0;
	/*get_size should always have the following code:
	virtual size_t get_size(void) const {
		return sizeof(Type);
	}
	*/

	/*symbols should change this to check that it's nil*/
	virtual bool isnil(void) const {return 0;};
	bool istrue(void) const {return !isnil();};

	virtual void probe(size_t indent) =0;//for debugging

	virtual ~Generic(){};

	size_t total_size(void);
};

inline bool is(Generic const* a, Generic const* b){ return a->is(b);}
inline bool iso(Generic const* a, Generic const* b){ return a->iso(b);}

/*RAII class to handle the to-pointers
This is used to protect the to-pointers in case of
an exception.  The invariant is, to-pointers are
NULL during normal operation.  If to-pointers must
be used, access to them should be via
ToPointerLock's, so that the to-pointers are reset
to NULL once the non-normal operation (GC,
tracing) are complete.  When the operation
determines that the to-pointers can be left used
(i.e. after a GC run) then it can call the clear()
member function so that it won't bother resetting
the to-pointers.
*/
class ToPointerLock{
private:
	std::set<Generic*> enset;
public:
	void pointto(Generic* from, Generic* to){
		enset.insert(from);
		from->to_pointer = to;
	}
	Generic* to(Generic* from){
		return from->to_pointer;
	}
	/*used to ignore the to-pointers in a successful
	copy-and-sweep GC cycle
	*/
	void good(void){
		enset.clear();
	}
	~ToPointerLock(){
		std::set<Generic*>::iterator i;
		for(i = enset.begin(); i != enset.end(); ++i){
			(*i)->to_pointer = NULL;
		}
	}
};

class Cons : public Generic{
protected:
	Cons(Cons const& s) : Generic(), a(s.a), d(s.d){};
public:
	Generic* a;
	Generic* d;
	/*standard stuff*/
	virtual size_t hash(void) const {
		return a->hash() ^ d->hash();
	}
	virtual Cons* clone(Semispace& sp) const{
		return new(sp) Cons(*this);
	}
	virtual size_t get_size(void) const {
		return sizeof(Cons);
	}

	/*comparisons*/
	virtual bool iso(Generic*gp) const{
		if(is(gp)) {
			return 1;
		} else {
			Cons* cp = dynamic_cast<Cons*>(gp);
			if(cp != NULL){
				return a->iso(cp->a) && d->iso(cp->d);
			} else return 0;
		}
	}

	/*references*/
	virtual void get_refs(std::stack<Generic**>&s){
		s.push(&a);
		s.push(&d);
	}

	virtual void probe(size_t);

	Cons(void) : Generic() {};
	virtual ~Cons(){};

};

class MetadataCons : public Cons {
protected:
	MetadataCons(MetadataCons const& s):Cons(s), line(s.line), file(s.file){};
public:
	Generic* line;
	Generic* file;
	virtual MetadataCons* clone(Semispace& sp) const{
		return new(sp) MetadataCons(*this);
	}
	virtual size_t get_size(void) const {
		return sizeof(MetadataCons);
	}
	virtual void get_refs(std::stack<Generic**>& s){
		Cons::get_refs(s);
		s.push(&line);
		s.push(&file);
	}

	virtual void probe(size_t);

	MetadataCons() : Cons() {};
	virtual ~MetadataCons(){};

};

class Sym : public Generic{
private:
	boost::shared_ptr<Atom> a;
	Sym(void){}; //disallowed!
protected:
	Sym(Sym const& s) : Generic(), a(s.a){};
public:
	/*standard stuff*/
	virtual size_t hash(void) const{
		return (size_t) &(*a);
	}
	virtual Sym* clone(Semispace& sp) const{
		return new(sp) Sym(*this);
	}
	virtual size_t get_size(void) const{
		return sizeof(Sym);
	};

	virtual bool is(Generic* gp) const{
		if(gp == this) return 1;
		Sym* sp = dynamic_cast<Sym*>(gp);
		if(sp != NULL){
			return sp->a == a;
		} else return 0;
	};

	virtual bool isnil(void) const {
		return a == NILATOM;
	}

	virtual void probe(size_t);

	Sym(boost::shared_ptr<Atom> const& na) : Generic(), a(na){};
	virtual ~Sym(){};

	/*new stuff*/
	Atom& atom(void){return *a;};
};

class Executor;

class Closure : public Generic {
private:
	boost::shared_ptr<Executor> cd;
	/*possibly add a parallel vector of variable name atoms for
	debugging?
	*/
	std::vector<Generic*> vars;
protected:
	Closure(Closure const& o) : Generic(), cd(o.cd), vars(o.vars) {}
public:
	Executor const& code(void) {return *cd;};
	/*standard stuff*/
	virtual size_t hash(void) const{
		return (size_t) &*cd;
	}
	virtual Closure* clone(Semispace& sp) const{
		return new(sp) Closure(*this);
	}
	virtual size_t get_size(void) const{
		return sizeof(Closure);
	}

	Closure(boost::shared_ptr<Executor> c, size_t s) :
		Generic(), cd(c), vars(s) {}
};

#endif //TYPES_H

