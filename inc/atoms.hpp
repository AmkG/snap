#ifndef ATOMS_H
#define ATOMS_H
#include<string>
#include<iostream>
#include<map>
#include<boost/shared_ptr.hpp>

class Atom {
protected:
	Atom(){};
	Atom(Atom&){};
public:
	virtual ~Atom(){};
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

/*Might need to inherit from Heap at some point*/
class Globals {
private:
	std::map< std::string, boost::shared_ptr<Atom> > string_to_atom;
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
};

#endif //ATOMS_H

