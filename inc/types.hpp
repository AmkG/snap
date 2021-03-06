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
class Process;

class Generic {
private:
	Generic* to_pointer;
protected:
	Generic(void) : to_pointer(NULL) {};
	Generic(Generic const&) : to_pointer(NULL) {};
public:
	friend class Semispace;
	friend class ToPointerLock;

	/*standard stuff*/
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
	virtual size_t get_size(void) const =0;
	/*get_size should always have the following code:
	virtual size_t get_size(void) const {
		return sizeof(Type);
	}
	*/
	virtual void probe(size_t indent) =0;//for debugging
	virtual boost::shared_ptr<Atom> type_atom(void) const {return INTERNALATOM;};

	/*overridable stuff*/

	/*gets addresses of references to other objects*/
	virtual void get_refs(std::stack<Generic**>&){};

	/*comparisons*/
	virtual bool is(Generic const* gp) const{return gp == this;}
	virtual bool iso(Generic const* gp) const{return is(gp);}
	virtual bool isnil(void) const {return 0;};
	bool istrue(void) const {return !isnil();};

	/*types*/
	virtual Generic* type(Process&);
	virtual Generic* rep(void) {return this;};

	/*list*/
	virtual Generic* car(void);
	virtual Generic* cdr(void);

	/*sharedvar*/
	virtual Generic* sv_ref(void);
	Generic* make_sv(Process&);

	virtual ~Generic(){};

	/*truly generic stuff*/
	size_t total_size(ToPointerLock&, std::stack<Generic**>&);
};

#define GENERIC_STANDARD_DEFINITIONS(Type) \
	virtual Type* clone(Semispace& sp) const { return new(sp) Type(*this);}; \
	virtual size_t get_size(void) const {return sizeof(Type);}

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
(i.e. after a GC run) then it can call the good()
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
	virtual void probe(size_t);
	virtual boost::shared_ptr<Atom> type_atom(void) const {return CONSATOM;};

	/*overridden stuff*/
	/*comparisons*/
	virtual bool iso(Generic const* gp) const{
		if(is(gp)) {
			return 1;
		} else {
			Cons const* cp = dynamic_cast<Cons const*>(gp);
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

	/*list*/
	virtual Generic* car(void);
	virtual Generic* cdr(void);

	Generic* a;
	Generic* d;

	/*
	NOTE!  Although it might make sense to define
	Cons(Generic* a, Generic* d), it won't work
	properly.  The problem is that the expected
	use case is something like this:
		gp = new(proc) Cons( ... );
	However, the new operator is an allocating
	operation, and the allocating operation might
	trigger a GC.  And we're using a copying GC.
	A copying GC means that if copying occurs,
	pointers become invalidated.

	This means that if we had passed in Generic
	pointers, those pointers don't get modified
	by the GC (the GC isn't *that* sophisticated,
	besides the GC is for the convenience of the
	Arc programmer, not the C++ implementeer).
	This also means that any pointers that we
	would like to put in Cons (or any other
	container type) should be stored first in a
	member of the root set of the process, such
	as the process stack.

	This is also the reason why a and d aren't
	private or protected - they can't be safely
	initialized by the ctor, so they must be
	initialized afterwards.
	*/
	Cons(void) : Generic() {};
	virtual ~Cons(){};

};

class MetadataCons : public Cons {
protected:
	MetadataCons(MetadataCons const& s):Cons(s), line(s.line), file(s.file){};
public:
	/*standard stuff*/
	virtual MetadataCons* clone(Semispace& sp) const{
		return new(sp) MetadataCons(*this);
	}
	virtual size_t get_size(void) const {
		return sizeof(MetadataCons);
	}
	virtual void probe(size_t);

	/*overrideable stuff*/
	virtual void get_refs(std::stack<Generic**>& s){
		Cons::get_refs(s);
		s.push(&line);
		s.push(&file);
	}

	Generic* line;
	Generic* file;
	MetadataCons() : Cons() {};
	virtual ~MetadataCons(){};

};

class Sym : public Generic{
private:
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
	virtual void probe(size_t);
	virtual boost::shared_ptr<Atom> type_atom(void) const {return SYMATOM;};

	/*overrideable stuff*/
	virtual bool is(Generic const* gp) const{
		if(gp == this) return 1;
		Sym const* sp = dynamic_cast<Sym const*>(gp);
		if(sp != NULL){
			return sp->a == a;
		} else return 0;
	};

	/*comparisons*/
	virtual bool isnil(void) const {
		return a == NILATOM;
	}

	/*list*/
	virtual Generic* car(void);
	virtual Generic* cdr(void);

	/*new stuff*/
	Sym(boost::shared_ptr<Atom> const& na) : Generic(), a(na){};
	virtual ~Sym(){};

	Atom& atom(void){return *a;};
	boost::shared_ptr<Atom> a;
};

class Heap;
class Executor;

class Closure;
class KClosure;
/*Factory functions for closures*/
Closure* NewClosure(Heap&, Executor*, size_t);
Closure* NewClosure(Heap&, boost::shared_ptr<Executor>, size_t);
KClosure* NewKClosure(Heap&, Executor*, size_t);
KClosure* NewKClosure(Heap&, boost::shared_ptr<Executor>, size_t);

/*closure structures shouldn't be
modified after they are constructed
*/
class Closure : public Generic {
protected:
	Closure(Closure const& o)
		: Generic(), cd(o.cd){}
	boost::shared_ptr<Executor> cd;

	Closure(boost::shared_ptr<Executor> c);

public:
	/*standard stuff*/
	virtual size_t hash(void) const{
		return (size_t) &*cd;
	}
	virtual void probe(size_t);
	virtual boost::shared_ptr<Atom> type_atom(void) const {return FNATOM;};

	/*new stuff*/
	Executor const& code(void) {return *cd;};
	/*WARNING
	arc2c references closure values starting at index 1 (index 0
	is the function's code itself, and is never used in practice).
	We will have to translate the numbers properly for those cases
	NOTE: this translation is done in the compiler, specifically
	arc2b/bytecodegen.arc
	*/
	virtual Generic* & operator[](int) =0;
	virtual Generic* const & operator[](int) const =0;
	virtual size_t size(void) const =0;

	virtual ~Closure(){}

	friend Closure* NewClosure(Heap&, Executor*, size_t);
	friend Closure* NewClosure(Heap&, boost::shared_ptr<Executor>, size_t);
};

/*closure type for continuations*/
/*Unlike the Closure class from which it
is derived, KClosure is mutable, as long
as its reusable() member function returns
true.  If you would like to mutate the
KClosure but its reusable() member
function returns false, you must create a
new KClosure yourself and copy each
entry.
*/
class KClosure : public Closure {
private:
	bool nonreusable;
protected:
	KClosure(KClosure const& o)
		: Closure(o), nonreusable(o.nonreusable){}

	KClosure(boost::shared_ptr<Executor> c);

public:
	/*standard stuff*/
	virtual void probe(size_t);

	/*new stuff*/
	void codereset(Executor* ncd);
	bool reusable(void) const {return !nonreusable;}
	void banreuse(void) {
		if(nonreusable) return;
		nonreusable = 1;
		for(size_t i = 0; i < size(); ++i){
			KClosure* kp = dynamic_cast<KClosure*>((*this)[i]);
			if(kp){
				kp->banreuse();
			}
		}
	}

	virtual ~KClosure(){}

	friend KClosure* NewKClosure(Heap&, Executor*, size_t);
	friend KClosure* NewKClosure(
			Heap&, boost::shared_ptr<Executor>, size_t);
};

class Integer : public Generic {
protected:
	Integer(Integer const& o) : Generic(), val(o.val){}
public:
	/*standard stuff*/
	virtual size_t hash() const {
		return (size_t) val;
	}
	virtual Integer* clone(Semispace& sp) const{
		return new(sp) Integer(*this);
	}
	virtual size_t get_size(void) const{
		return sizeof(Integer);
	}
	virtual boost::shared_ptr<Atom> type_atom(void) const {return INTATOM;};

	virtual void probe(size_t);

	/*new stuff*/
	int val;
	int integer(void){return val;}
	Integer(int x) : Generic(), val(x) {}

	virtual ~Integer(){}
};

class BytecodeSequence;
class Bytecode;

/*Used during compilation to hold a bytecode sequence while
converting from a symbolcode list to an internal bytecode
format.
*/
class ArcBytecodeSequence : public Generic {
protected:
	ArcBytecodeSequence(ArcBytecodeSequence const & o)
		: Generic(), seq(o.seq) {}
public:
	/*standard stuff*/
	virtual size_t hash() const{
		return (size_t)(void*) &*seq;
	}
	virtual ArcBytecodeSequence* clone(Semispace& sp) const{
		return new(sp) ArcBytecodeSequence(*this);
	}
	virtual size_t get_size(void) const{
		return sizeof(ArcBytecodeSequence);
	}
	virtual void probe(size_t);
	virtual boost::shared_ptr<Atom> type_atom(void) const {return INTERNALATOM;};

	/*new stuff*/
	void append(Bytecode* b);
	boost::shared_ptr<BytecodeSequence> seq;
	ArcBytecodeSequence(void);

	virtual ~ArcBytecodeSequence(){}
};

/*created by 'annotate*/
class Tagged : public Generic {
protected:
	Tagged(Tagged const& o)
		: Generic(), type_o(o.type_o), rep_o(o.rep_o) {}
public:
	/*standard stuff*/
	virtual size_t hash() const{
		return type_o->hash() + rep_o->hash();
	}
	virtual Tagged* clone(Semispace& sp) const{
		return new(sp) Tagged(*this);
	}
	virtual size_t get_size(void) const{
		return sizeof(Tagged);
	}
	virtual void probe(size_t);
	/*shouldn't be used*/
	virtual boost::shared_ptr<Atom> type_atom(void) const {return INTERNALATOM;};

	/*overridden stuff*/
	virtual void get_refs(std::stack<Generic**>& s){
		s.push(&type_o);
		s.push(&rep_o);
	}
	virtual Generic* type(Process&) {
		return type_o;
	}
	virtual Generic* rep(void) {
		return rep_o;
	}

	/*new stuff*/
	Generic* type_o;
	Generic* rep_o;
	Tagged(void) : Generic() {}

	virtual ~Tagged(){}
};

class SharedVar : public Generic {
protected:
	SharedVar(SharedVar const& o)
		: Generic(), val(o.val) {}
public:
	/*standard stuff*/
	size_t hash(void) const {
		return 0;
	}
	GENERIC_STANDARD_DEFINITIONS(SharedVar)
	virtual void probe(size_t);

	/*overridden stuff*/
	virtual void get_refs(std::stack<Generic**>& s){
		s.push(&val);
	}
	virtual Generic* sv_ref(void);

	/*new stuff*/
	Generic* val;
	SharedVar(void) : Generic() {}

	virtual ~SharedVar(){}
};

class ProcessHandle;

class Pid : public Generic {
protected:
	Pid(Pid const& o)
		: Generic(), hproc(o.hproc) {}
public:
	/*standard stuff*/
	size_t hash(void) const {
		return (size_t) hproc.get();
	}
	GENERIC_STANDARD_DEFINITIONS(Pid)
	virtual void probe(size_t);
	virtual boost::shared_ptr<Atom> type_atom(void) const {
		return PIDATOM;
	};

	/*overridable stuff*/
	virtual bool is(Generic const* o) const {
		if(o == this) return 1;
		Pid const* po = dynamic_cast<Pid const*>(o);
		if(po == NULL) return 0;
		return po->hproc == hproc;
	}

	/*new stuff*/
	boost::shared_ptr<ProcessHandle> hproc;
	Pid(boost::shared_ptr<ProcessHandle> p)
		: Generic(), hproc(p) {}

	virtual ~Pid(){}
};

class BinaryBlob : public Generic{
private:
	void copy_on_write(void){
		if(!pdat.unique()){
			pdat.reset(new std::vector<unsigned char>(*pdat));
		}
	}
protected:
	BinaryBlob(BinaryBlob const& o)
		: Generic(), pdat(o.pdat){}
public:
	/*standard stuff*/
	size_t hash(void) const {
		return 0;
	}
	GENERIC_STANDARD_DEFINITIONS(BinaryBlob)
	virtual void probe(size_t);
	virtual boost::shared_ptr<Atom> type_atom(void) const {
		return BINATOM;
	};

	/*overridable stuff*/
	virtual bool iso(Generic const* gp) const {
		if(is(gp)) return 1;
		BinaryBlob const* bp = dynamic_cast<BinaryBlob const*>(gp);
		if(bp == NULL) return 0;
		return *(bp->pdat) == *pdat;
	}

	/*new stuff*/
	boost::shared_ptr<std::vector<unsigned char> > pdat;
	BinaryBlob(void)
		: Generic(), pdat(new std::vector<unsigned char>()) {}
	void append(unsigned char n){
		copy_on_write();
		pdat->push_back(n);
	}
	unsigned char& operator[](size_t i){
		return (*pdat)[i];
	}
	unsigned char const& operator[](size_t i) const{
		return (*pdat)[i];
	}

	virtual ~BinaryBlob(){}
};

/*object that conceptually contains an object that may be
transmitted to a process.
*/
class SemispacePackage : public Generic{
private:
	SemispacePackage(void){}
protected:
	SemispacePackage(SemispacePackage const & o)
		: Generic(), ns(o.ns), gp(o.gp) {}
public:
	/*standard stuff*/
	size_t hash(void) const {
		return (size_t) ns.get();
	}
	GENERIC_STANDARD_DEFINITIONS(SemispacePackage)
	virtual void probe(size_t);

	/*new stuff*/
	boost::shared_ptr<Semispace> ns;
	Generic* gp; //NOTE!  Should point within the attached semispace
	SemispacePackage(boost::shared_ptr<Semispace> nns, Generic* ngp)
		: Generic(), ns(nns), gp(ngp) {}

	virtual ~SemispacePackage(){}
};

/*Opaque datatype to be used internally by
central I/O process
*/
class PortData;

class ArcPortData : public Generic {
protected:
	ArcPortData(ArcPortData const & o)
		: Generic(), impl(o.impl) {}
public:
	virtual size_t hash(void) const {
		return (size_t) impl.get();
	}
	GENERIC_STANDARD_DEFINITIONS(ArcPortData)
	virtual void probe(size_t);

	/*overrideable stuff*/
	virtual bool is(Generic const* gp) const {
		if(gp == this) return 1;
		ArcPortData const* ip = dynamic_cast<ArcPortData const*>(gp);
		if(ip == NULL) return 0;
		return ip->impl == impl;
	}

	/*new stuff*/
	ArcPortData(boost::shared_ptr<PortData> o)
		: impl(o) {}
	boost::shared_ptr<PortData> impl;
	virtual ~ArcPortData(){}
};

#endif //TYPES_H

