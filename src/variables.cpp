#define NO_EXTERN
#include"variables.hpp"
#include"atoms.hpp"
#include"runsystem.hpp"

void variables_init(void){
	globals.reset(new Globals());
	runsystem = NewRunsystem();
	NILATOM = globals->lookup("nil");
	TATOM = globals->lookup("t");
	CONSATOM = globals->lookup("cons");
	SYMATOM = globals->lookup("sym");
	FNATOM = globals->lookup("fn");
	INTATOM = globals->lookup("int");
	INTERNALATOM = globals->lookup("internally-used");
}

