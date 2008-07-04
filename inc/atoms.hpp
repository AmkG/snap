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
	boost::shared_ptr<Atom> lookup(std::string& s){
		/*insert locking of string_to_atom table here*/
		std::map< std::string, boost::shared_ptr<GlobalAtom> >::iterator i;
		i = string_to_atom.find(s);
		if(i != string_to_atom.end()){
			std::map< std::string,
				boost::shared_ptr<Atom> >::value_type v = *i;
			return v.second;
		} else {
			boost::shared_ptr<GlobalAtom> tmp(new GlobalAtom(s));
			string_to_atom[s].swap(tmp);
			return string_to_atom[s];
		}
	}
	boost::shared_ptr<Atom> lookup(char const * s){
		std::string ss(s);
		return lookup(ss);
	}
	/*precondition: lock for globals must be acquired*/
	/*requirements: g must be in the given semispace ns*/
	void assign(boost::shared_ptr<Atom> a,
		    boost::shared_ptr<Semispace> ns,
		    Generic* g){
		other_spaces.push_back(ns);
		a->value = g;
		/*check if local atom*/
		LocalAtom* lp = dynamic_cast<LocalAtom*>(&*a);
		if(lp != NULL){
			/*add a to set*/
			assigned_local_atoms.insert(a);
			/*TODO: figure out how to dynamic_cast shared_ptr<Base>
			to shared_ptr<Derived>, so that assigned_local_atoms
			are indeed local atoms
			*/
		}
	}
	Generic* get(boost::shared_ptr<Atom> a){
		return a->value;
	}
	void GC(void){Heap::GC(0);};
	virtual ~Globals(){};
};

#endif //ATOMS_H

