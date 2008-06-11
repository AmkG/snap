#ifndef TYPES_H
#define TYPES_H
#include<stack>
#include<set>
#include"heaps.hpp"
#include"hashes.hpp"

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
	virtual ~Generic(){};

	size_t total_size(void);
};

inline bool is(Generic const* a, Generic const* b){ return a->is(b);}
inline bool iso(Generic const* a, Generic const* b){ return a->iso(b);}

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
private:
	Generic* a;
	Generic* d;
protected:
	Cons(Cons const&){};
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

	Cons(Generic* na, Generic* nd) : a(na), d(nd) {};
	virtual ~Cons(){};

	/*new stuff*/
	Generic* car(void) const {return a;}
	Generic* cdr(void) const {return d;}
	Generic* scar(Generic* na) {return a = na;}
	Generic* scdr(Generic* nd) {return d = nd;}
};

class MetadataCons : public Cons {
private:
	Generic* line;
	Generic* file;
protected:
	MetadataCons(MetadataCons const&):Cons(*this){};
public:
	virtual MetadataCons* clone(Semispace& sp) const{
		return new(sp) MetadataCons(*this);
	}
	virtual size_t get_size(void) const {
		return sizeof(MetadataCons);
	}

	MetadataCons(Generic* na, Generic* nd, Generic* nline, Generic* nfile)
		: Cons(na, nd), line(nline), file(nfile){};
	virtual ~MetadataCons(){};

	/*can't be changed*/
	Generic* getline(void) const {return line;}
	Generic* getfile(void) const {return file;}
};

class Atom;

class Sym : public Generic{
private:
	boost::shared_ptr<Atom> a;
	Sym(void){}; //disallowed!
protected:
	Sym(Sym const&){};
public:
	/*standard stuff*/
	virtual size_t hash(void) const{
		return distribute_bits((size_t) &(*a));
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

	Sym(boost::shared_ptr<Atom> const& na) : a(na){};
	virtual ~Sym(){};

	/*new stuff*/
	Atom& atom(void){return *a;};
};

#endif //TYPES_H

