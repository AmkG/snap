#ifndef TYPES_H
#define TYPES_H
#include<stack>
#include<set>
#include<vector>
#include"heaps.hpp"
#include"hashes.hpp"
#include"variables.hpp"
#include"executors.hpp"

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
	virtual boost::shared_ptr<Atom> type_atom(void) const =0;

	/*overridable stuff*/

	/*gets addresses of references to other objects*/
	virtual void get_refs(std::stack<Generic**>&){};

	/*comparisons*/
	virtual bool is(Generic const* gp) const{return gp == this;}
	virtual bool iso(Generic const* gp) const{return is(gp);}
	virtual bool isnil(void) const {return 0;};
	bool istrue(void) const {return !isnil();};

	virtual Generic* type(Process&) const;
	virtual Generic* rep(void) {return this;};

	virtual ~Generic(){};

	/*truly generic stuff*/
	size_t total_size(ToPointerLock&, std::stack<Generic**>&);
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

	Generic* a;
	Generic* d;

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

	Sym(boost::shared_ptr<Atom> const& na) : Generic(), a(na){};
	virtual ~Sym(){};

	/*new stuff*/
	Atom& atom(void){return *a;};
	boost::shared_ptr<Atom> a;
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
	virtual void probe(size_t);
	virtual boost::shared_ptr<Atom> type_atom(void) const {return FNATOM;};

	/*overrideable stuff*/
	virtual void get_refs(std::stack<Generic**>& s){
		for(size_t i = 0; i < vars.size(); ++i){
			s.push(&vars[i]);
		}
	}

	Closure(Executor* c, size_t s) :
		Generic(), cd(c), vars(s) {};
	Closure(boost::shared_ptr<Executor> c, size_t s) :
		Generic(), cd(c), vars(s) {};

	/*new stuff*/
	Executor const& code(void) {return *cd;};
	/*WARNING
	arc2c references closure values starting at index 1 (index 0
	is the function's code itself, and is never used in practice).
	We will have to translate the numbers properly for those cases
	*/
	Generic* & operator[](int i) {return vars[i];}
	Generic* const & operator[](int i) const {return vars[i];}
};

class Integer : public Generic {
private:
	int val;
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
	int integer(void){return val;}
	Integer(int x) : Generic(), val(x) {}
};

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
	void append(Bytecode* b){
		seq->append(b);
	}
	boost::shared_ptr<BytecodeSequence> seq;
	ArcBytecodeSequence(void) : Generic(), seq(new BytecodeSequence()) {}
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
	virtual Generic* type(Process&) const {
		return type_o;
	}
	virtual Generic* rep(void) {
		return rep_o;
	}

	/*new stuff*/
	Generic* type_o;
	Generic* rep_o;
	Tagged(void) : Generic() {}
};

#endif //TYPES_H

