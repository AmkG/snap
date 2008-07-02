/*
*/
#include"types.hpp"
#include"heaps.hpp"

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

void Integer::probe(size_t ind){
	INDENT(ind); std::cout << "INTEGER: " << std::dec << val <<
			" @" << std::hex << ((size_t)(this)) << std::endl;
}

void ArcBytecodeSequence::probe(size_t ind){
	INDENT(ind); std:: cout << "BYTECODE @" <<
			std::hex << ((size_t) seq.get()) << std::endl;
}

