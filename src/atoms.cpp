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

