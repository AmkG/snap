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

Closure::Closure(Executor* c, size_t s)
	: Generic(), cd(c), vars(s) {}

Closure::Closure(boost::shared_ptr<Executor> c, size_t s)
	: Generic(), cd(c), vars(s) {}

KClosure::KClosure(Executor* c, size_t s)
	: Closure(c, s), nonreusable(0) {}

KClosure::KClosure(boost::shared_ptr<Executor> c, size_t s)
	: Closure(c, s), nonreusable(0) {}

ArcBytecodeSequence::ArcBytecodeSequence()
	: Generic(), seq(new BytecodeSequence()) {}

void ArcBytecodeSequence::append(Bytecode* b){
	seq->append(b);
}

/*DEBUG CODE*/
#include<iostream>
#define INDENT(ind)	for(size_t i = 0; i < ind; ++i) std::cout << "\t"
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
			std::hex << ((size_t) cd.get()) << " {" << std::endl;
	for(size_t i = 0; i < vars.size(); ++i){
		vars[i]->probe(ind+1);
	}
	INDENT(ind); std::cout << "}" << std::endl;
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
	for(size_t i = 0; i < pdat->size; ++i){
		INDENT(ind+1); std::cout << (*pdat)[i] << std::endl;
	}
}

void SemispacePackage(size_t ind){
	INDENT(ind); std::cout << "PACKAGE @" << std::hex << ((size_t) this) <<
			std::endl;
	INDENT(ind); std::cout << "Semispace @" << ((size_t) ns.get()) <<
			std::endl;
	gp->probe(ind+1);
}

