#include"atoms.hpp"
#include"types.hpp"
#include<stack>
#include<string>
#include<boost/shared_ptr.hpp>
#include<stdexcept>

/*-----------------------------------------------------------------------------
Globals
-----------------------------------------------------------------------------*/
void Globals::get_root_set(std::stack<Generic**>& s){
	std::map<std::string, boost::shared_ptr<GlobalAtom> >::iterator i;
	Atom* ap;
	for(i = string_to_atom.begin(); i != string_to_atom.end(); ++i){
		ap = i->second.get();
		if(ap->value != NULL) s.push(&ap->value);
	}
	std::set<boost::shared_ptr<Atom> >::iterator ii;
	for(ii = assigned_local_atoms.begin();
	    ii != assigned_local_atoms.end();
	    ++i){
		ap = ii->get();
		s.push(&ap->value);
	}
}

boost::shared_ptr<Atom> Globals::lookup(std::string& s){
	/*insert locking of string_to_atom table here*/
	std::map< std::string, boost::shared_ptr<GlobalAtom> >::iterator i;
	i = string_to_atom.find(s);
	if(i != string_to_atom.end()){
		return i->second;
	} else {
		boost::shared_ptr<GlobalAtom> tmp(new GlobalAtom(s));
		string_to_atom[s].swap(tmp);
		return string_to_atom[s];
	}
}

/*requirements: g must be in the given semispace ns*/
void Globals::assign(boost::shared_ptr<Atom> a,
	    boost::shared_ptr<Semispace> ns,
	    Generic* g){
	{/*insert locking for atom here*/
		a->value = g;
		a->s = ns; // this release the old data
	}
	/*check if local atom*/
	LocalAtom* lp = dynamic_cast<LocalAtom*>(&*a);
	if(lp != NULL){
		/*insert locking for assigned_local_atoms here*/
		/*add a to set*/
		assigned_local_atoms.insert(a);
		/*TODO: figure out how to dynamic_cast shared_ptr<Base>
		to shared_ptr<Derived>, so that assigned_local_atoms
		are indeed local atoms
		*/
	}
}
