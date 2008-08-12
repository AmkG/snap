#define NO_EXTERN
#include"variables.hpp"
#include"atoms.hpp"
#include"runsystems.hpp"
#include"phandles.hpp"
#include"ioprocesses.hpp"

void variables_init(void){
	globals.reset(new Globals());
	runsystem.reset(NewRunsystem());
	centralio.reset(new ProcessHandle(NewCentralIOProcess()));
	NILATOM = globals->lookup("nil");
	TATOM = globals->lookup("t");
	CONSATOM = globals->lookup("cons");
	SYMATOM = globals->lookup("sym");
	FNATOM = globals->lookup("fn");
	INTATOM = globals->lookup("int");
	PIDATOM = globals->lookup("pid");
	BINATOM = globals->lookup("binary");
	INTERNALATOM = globals->lookup("internally-used");
	WRITEATOM = globals->lookup("write");
	READ1ATOM = globals->lookup("read1");
	ERRATOM = globals->lookup("err");
	EOLATOM = globals->lookup("eol");
	INPUTATOM = globals->lookup("input");
	OUTPUTATOM = globals->lookup("output");
}

