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
	std::map<std::string, boost::shared_ptr<Atom> >::iterator i;
	boost::shared_ptr<Atom> ap;
	for(i = string_to_atom.begin(); i != string_to_atom.end(); ++i){
		ap = (*i).second;
		GlobalAtom* gp = dynamic_cast<GlobalAtom*>(&*ap);
		if(gp == NULL){
			throw std::runtime_error("Non-global atom in global atoms list");
		}
		s.push(&gp->value);
	}
}

