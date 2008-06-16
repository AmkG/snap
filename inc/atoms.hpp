#ifndef ATOMS_H
#define ATOMS_H
#include<string>
#include<iostream>
#include<map>
#include<boost/shared_ptr.hpp>
#include"heaps.hpp"

class Atom {
protected:
	Atom(){};
	Atom(Atom&){};
public:
	virtual ~Atom(){};
};

class Globals;

class GlobalAtom : public Atom {
private:
	std::string utf8string;
	Generic* value;
public:
	GlobalAtom(std::string& s) : utf8string(s), value(NULL){};
	virtual ~GlobalAtom(){};
	void emit(void) const {
		std::cout << utf8string;
	};
	friend class Globals;
};

/*from (uniq)*/
class LocalAtom : public Atom {
public:
	virtual ~LocalAtom(){};
};

class Globals : public Heap {
private:
	std::map< std::string, boost::shared_ptr<Atom> > string_to_atom;
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
	boost::shared_ptr<Atom> lookup(std::string& s){
		/*insert locking of string_to_atom table here*/
		std::map< std::string, boost::shared_ptr<Atom> >::iterator i;
		i = string_to_atom.find(s);
		if(i != string_to_atom.end()){
			std::map< std::string,
				boost::shared_ptr<Atom> >::value_type v = *i;
			return v.second;
		} else {
			boost::shared_ptr<Atom> tmp(new GlobalAtom(s));
			string_to_atom[s].swap(tmp);
			return string_to_atom[s];
		}
	}
	boost::shared_ptr<Atom> lookup(char const * s){
		std::string ss(s);
		return lookup(ss);
	}
	/*precondition: lock for this object must be acquired*/
	void assign(boost::shared_ptr<Atom> a,
		    boost::shared_ptr<Semispace> ns,
		    Generic* g){
		other_spaces.push_back(ns);
		GlobalAtom* gp = dynamic_cast<GlobalAtom*>(&*a);
		if(gp == NULL) throw std::runtime_error("write to non-global");
		gp->value = g;
	}
	void GC(void){Heap::GC(0);};
	virtual ~Globals(){};
};

#endif //ATOMS_H

