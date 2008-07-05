#ifndef ATOMS_H
#define ATOMS_H
#include<string>
#include<iostream>
#include<map>
#include<set>
#include<boost/shared_ptr.hpp>
#include<stdexcept>
#include"heaps.hpp"

class Globals;

class Atom {
private:
	Generic* value;
protected:
	Atom() : value(NULL){};
	Atom(Atom&){};
public:
	virtual ~Atom(){};
	friend class Globals;
};

class GlobalAtom : public Atom {
private:
	std::string utf8string;
public:
	GlobalAtom(std::string& s) : utf8string(s){};
	virtual ~GlobalAtom(){};
	void emit(void) const {
		std::cout << utf8string;
	};
};

/*from (uniq)*/
class LocalAtom : public Atom {
public:
	virtual ~LocalAtom(){};
};

/*NOTE!  intended to be instantiated only once (singleton)*/
class Globals : public Heap {
private:
	std::map< std::string, boost::shared_ptr<GlobalAtom> > string_to_atom;
	std::set<boost::shared_ptr<Atom> > assigned_local_atoms;
	/*insert mutex here*/
	/*The mutex must be locked for the following cases:
	1. Setting of global variables
	2. Garbage collection.  1 & 2 will probably need to be done
	   right after one another.
	3. Actual reading of variable (if variable is not in cache)
	*/
protected:
	virtual void get_root_set(std::stack<Generic**>&);
public:
	boost::shared_ptr<Atom> lookup(std::string&);
	boost::shared_ptr<Atom> lookup(char const * s){
		std::string ss(s);
		return lookup(ss);
	}
	void assign(boost::shared_ptr<Atom>, boost::shared_ptr<Semispace>,
		Generic*);
	Generic* get(boost::shared_ptr<Atom> a){
		return a->value;
	}
	void GC(void){Heap::GC(0);};
	virtual ~Globals(){};
};

#endif //ATOMS_H

