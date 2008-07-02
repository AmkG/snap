#include"variables.hpp"
#include<iostream>
#include"atoms.hpp"
#include<boost/shared_ptr.hpp>
#include<boost/scoped_ptr.hpp>

int main(int argc, const char* argv[]){
	variables_init();

	/*tests!*/
	/*make sure NILATOM is a global atom*/
	GlobalAtom* p = dynamic_cast<GlobalAtom*>(&*NILATOM);
	if(p != NULL){
		std::cout << "NIL ATOM: ";
		p->emit();
		std::cout << std::endl;
	} else {
		std::cout << "NIL ATOM not global!" << std::endl;
	}
	/*make sure NILATOM is looked up again*/
	boost::shared_ptr<Atom> tmp(globals->lookup("nil"));
	if(tmp != NILATOM){
		std::cout << "Eh??" << std::endl;
		p = dynamic_cast<GlobalAtom*>(&*tmp);
		if(p != NULL){
			std::cout << "Other 'nil atom: ";
			p->emit();
			std::cout << std::endl;
		} else {
			std::cout << "Other 'nil atom isn't even global!"
				<< std::endl;
		}
	} else {
		std::cout << "Got NILATOM on lookup again!" << std::endl;
	}
	/*make sure NILATOM != TATOM*/
	if(NILATOM != TATOM){
		std::cout << "As expected, 'nil is not 't" << std::endl;
	} else {
		std::cout << "Not good: 'nil is somehow 't!" << std::endl;
	}
}

