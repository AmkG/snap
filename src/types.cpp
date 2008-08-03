/*
*/
#include"types.hpp"
#include"heaps.hpp"
#include"processes.hpp"
#include"errors.hpp"
#include"executors.hpp"
#include"phandles.hpp"

/*O(N), where N is the number of objects involved!*/
/*necessary for determining the size of the semispace
to be allocated when sending messages.
*/
size_t Generic::total_size(ToPointerLock& toptrs, std::stack<Generic**>& s){
	Generic** gpp;
	Generic* to;
	size_t rv = get_size();
	/*Use the to-pointers as expensive marks*/
	toptrs.pointto(this, this);
	get_refs(s);
	while(!s.empty()){
		gpp = s.top(); s.pop();
		to = toptrs.to(*gpp);
		if(to == NULL){
			toptrs.pointto(*gpp, *gpp);
			rv += (*gpp)->get_size();
			(*gpp)->get_refs(s);
		}
	}
	/*toptrs' dtor should clear the to-pointers of the objects*/
	return rv;
}

Generic* Generic::type(Process& proc) {
	return new(proc) Sym(type_atom());
}

Generic* Generic::car(void) {
	throw ArcError("apply",
		"'car expects an object of type 'cons");
}

Generic* Generic::cdr(void) {
	throw ArcError("apply",
		"'cdr expects an object of type 'cons");
}

Generic* Generic::sv_ref(void) {
	throw ArcError("container",
		"expected an object of type 'container");
}

Generic* Generic::make_sv(Process& proc) {
	SharedVar* sp = new(proc) SharedVar();
	sp->val = this;
	return sp;
}

Generic* Cons::car(void){
	return a;
}

Generic* Cons::cdr(void){
	return d;
}

Generic* SharedVar::sv_ref(void) {
	return val;
}

Generic* Sym::car(void){
	if(isnil())	return this;
	else		Generic::car();
}

Generic* Sym::cdr(void){
	if(isnil())	return this;
	else		Generic::cdr();
}

Closure::Closure(boost::shared_ptr<Executor> c)
	: Generic(), cd(c) {}

KClosure::KClosure(boost::shared_ptr<Executor> c)
	: Closure(c), nonreusable(0) {}

ArcBytecodeSequence::ArcBytecodeSequence()
	: Generic(), seq(new BytecodeSequence()) {}

void ArcBytecodeSequence::append(Bytecode* b){
	seq->append(b);
}

#include<iostream>
#define INDENT(ind)	for(size_t __i = 0; __i < ind; ++__i) std::cout << "\t"

/*Closure implementations*/
template<typename C>
class EmptyClosure : public C {
protected:
	EmptyClosure<C>(EmptyClosure<C> const& o)
		: C(o) {}
public:
	/*standard stuff*/
	GENERIC_STANDARD_DEFINITIONS(EmptyClosure<C>)
	virtual void probe(size_t ind){
		C::probe(ind);
		INDENT(ind); std::cout << "(empty)" << std::endl;
	}

	/*new stuff*/
	EmptyClosure<C>(boost::shared_ptr<Executor> c)
		: C(c) {}
	virtual Generic* & operator[](int i) {
		return *((Generic**)NULL);
	}
	virtual Generic* const & operator[](int i) const {
		return *((Generic* const *)NULL);
	}
	virtual size_t size(void) const {return 0;}

	virtual ~EmptyClosure<C>(){}
};

template<typename C, size_t N>
class ClosureArray : public C {
private:
	Generic* vars[N];
	//needed for GENERIC_STANDARD_DEFINITIONS macro, which gets
	//confused by comma in type name
	typedef ClosureArray<C,N> __mytype;
protected:
	ClosureArray<C,N>(ClosureArray<C,N> const& o)
		: C(o) {
		for(size_t i = 0; i < N; ++i){
			vars[i] = o.vars[i];
		}
	}
public:
	/*standard stuff*/
	GENERIC_STANDARD_DEFINITIONS(__mytype)
	virtual void probe(size_t ind) {
		C::probe(ind);
		INDENT(ind); std::cout << "{" << std::endl;
		for(size_t i = 0; i < N; ++i){
			INDENT(ind); std::cout << std::dec << i << ":" <<
					std::endl;
			vars[i]->probe(ind+1);
		}
		INDENT(ind); std::cout << "}" << std::endl;
	}

	/*overrideable stuff*/
	virtual void get_refs(std::stack<Generic**>& s) {
		for(size_t i = 0; i < N; ++i) {
			s.push(&vars[i]);
		}
	}

	/*new stuff*/
	ClosureArray<C,N>(boost::shared_ptr<Executor> c)
		: C(c) {}
	virtual Generic* & operator[](int i) { return vars[i]; }
	virtual Generic* const & operator[](int i) const { return vars[i]; }
	virtual size_t size(void) const {return N;}

	virtual ~ClosureArray<C,N>(){}
};

template<typename C>
class ClosureVector : public C {
private:
	std::vector<Generic*> vars;
protected:
	ClosureVector<C>(ClosureVector<C> const& o)
		: C(o), vars(o.vars){ }
public:
	/*standard stuff*/
	GENERIC_STANDARD_DEFINITIONS(ClosureVector<C>)
	virtual void probe(size_t ind){
		C::probe(ind);
		INDENT(ind); std::cout << "{" << std::endl;
		for(int i = 0; i < vars.size(); ++i){
			INDENT(ind); std::cout << std::dec << i << ":" <<
					std::endl;
			vars[i]->probe(ind+1);
		}
		INDENT(ind); std::cout << "}" << std::endl;
	}

	/*overrideable stuff*/
	virtual void get_refs(std::stack<Generic**>& s) {
		for(size_t i = 0; i < vars.size(); ++i) {
			s.push(&vars[i]);
		}
	}

	/*new stuff*/
	ClosureVector<C>(boost::shared_ptr<Executor> c, size_t sz)
		: C(c), vars(sz) {}
	virtual Generic* & operator[](int i) {return vars[i];}
	virtual Generic* const & operator[](int i) const {return vars[i];}
	virtual size_t size(void) const {return vars.size();}

	~ClosureVector<C>(){}
};

/*Factory functions*/
template<typename C>
static C* NewClosureImpl(
		Heap& hp, boost::shared_ptr<Executor> c, size_t sz) {
	switch(sz){
	case 0:	return new(hp) EmptyClosure<C>(c);
	case 1: return new(hp) ClosureArray<C,1>(c);
	default:
		return new(hp) ClosureVector<C>(c,sz);
	}
}
Closure* NewClosure(Heap& hp, Executor* c, size_t sz) {
	return NewClosure(hp, boost::shared_ptr<Executor>(c), sz);
}
Closure* NewClosure(Heap& hp, boost::shared_ptr<Executor> c, size_t sz) {
	return NewClosureImpl<Closure>(hp, c, sz);
}
KClosure* NewKClosure(Heap& hp, Executor* c, size_t sz) {
	return NewKClosure(hp, boost::shared_ptr<Executor>(c), sz);
}
KClosure* NewKClosure(Heap& hp, boost::shared_ptr<Executor> c, size_t sz) {
	return NewClosureImpl<KClosure>(hp, c, sz);
}

/*DEBUG CODE*/
void Cons::probe(size_t ind){
	INDENT(ind); std::cout << "CONS cell@" <<
			std::hex << ((size_t)(this)) << std::endl;
	a->probe(ind+1);
	d->probe(ind+1);
}

void MetadataCons::probe(size_t ind){
	Cons::probe(ind);
	INDENT(ind); std::cout << "(metadata)" << std::endl;
	line->probe(ind+1);
	file->probe(ind+1);
}
#include"atoms.hpp"

void Sym::probe(size_t ind){
	INDENT(ind); std::cout << "SYMBOL: ";
	GlobalAtom* gp = dynamic_cast<GlobalAtom*>(a.get());
	if(gp != NULL) {gp->emit(); std::cout << std::endl;}
	else	std::cout << "<uniq>" << std::endl;
}

void Closure::probe(size_t ind){
	INDENT(ind); std::cout <<  "CLOSURE: fn@" <<
			std::hex << ((size_t) cd.get()) << std::endl;
}

void KClosure::probe(size_t ind){
	Closure::probe(ind);
	INDENT(ind); std::cout << "(" <<
			(nonreusable ? "" : "reusable ") <<
			"continuation)" << std::endl;
}

void Integer::probe(size_t ind){
	INDENT(ind); std::cout << "INTEGER: " << std::dec << val <<
			" @" << std::hex << ((size_t)(this)) << std::endl;
}

void ArcBytecodeSequence::probe(size_t ind){
	INDENT(ind); std::cout << "BYTECODE @" <<
			std::hex << ((size_t) seq.get()) << std::endl;
}

void Tagged::probe(size_t ind){
	INDENT(ind); std::cout << "TAGGED @" <<
			std::hex << ((size_t) this) << std::endl;
	INDENT(ind); std::cout << "type:" << std::endl;
	type_o->probe(ind+1);
	INDENT(ind); std::cout << "rep:" << std::endl;
	rep_o->probe(ind+1);
}

void SharedVar::probe(size_t ind){
	INDENT(ind); std::cout << "SHAREDVAR @" <<
			std::hex << ((size_t) this) << std::endl;
	val->probe(ind+1);
}

void Pid::probe(size_t ind){
	INDENT(ind); std::cout << "PID @" <<
			std::hex << ((size_t) this) <<
			" referring to: " << ((size_t) hproc->pproc.get()) <<
			std::endl;
}

void BinaryBlob::probe(size_t ind){
	INDENT(ind); std::cout << "BINARY @" << std::hex << ((size_t) this) <<
			" referring to:" << ((size_t) pdat.get()) <<
			std::endl;
	for(size_t i = 0; i < pdat->size(); ++i){
		INDENT(ind+1); std::cout << (*pdat)[i] << std::endl;
	}
}

void SemispacePackage::probe(size_t ind){
	INDENT(ind); std::cout << "PACKAGE @" << std::hex << ((size_t) this) <<
			std::endl;
	INDENT(ind); std::cout << "Semispace @" << ((size_t) ns.get()) <<
			std::endl;
	gp->probe(ind+1);
}

