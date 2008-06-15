#define NO_EXTERN
#include"variables.hpp"
#include"atoms.hpp"

void variables_init(void){
	globals.reset(new Globals());
	NILATOM = globals->lookup("nil");
	TATOM = globals->lookup("t");
}

