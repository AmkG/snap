#ifndef ERRORS_H
#define ERRORS_H
#include<stdexcept>
#include<string>
#include<boost/shared_ptr.hpp>
#include"atoms.hpp"
#include"variables.hpp"

class ArcError : public std::runtime_error{
private:
	boost::shared_ptr<Atom> type;
	std::string msg;
public:
	explicit ArcError(char* a, char* s) :
		std::runtime_error("Arc Error"),
		type(globals->lookup(a)),
		msg(a){};
	virtual const char * what() const throw(){
		desc.c_str(msg);
	}
	/*insert code to create Arc tagged object here*/
};

#endif //ERRORS_H

