#include"variables.hpp"
#include"executors.hpp"
#include"atoms.hpp"
#include"processes.hpp"
#include<iostream>

ProcessStatus execute(Process& proc, bool init=0){
	if(init) goto initialize;
	DISPATCH_EXECUTORS {
		EXECUTOR(arc_executor):
		NEXT_EXECUTOR;
		/*
		(fn (k#|1|# f#|2|#)
		  (f k (fn (_ r) (k r))))
		*/
		EXECUTOR(ccc):
		{	Closure* c = new(proc)
				Closure(THE_EXECUTOR(ccc_helper), 1);
			(*c)[0] = proc.stack[1/*k*/];
			proc.stack[0] = proc.stack[2/*f*/];
			proc.stack[2] = c;
		} NEXT_EXECUTOR;
		EXECUTOR(ccc_helper):
		{	Closure* c = dynamic_cast<Closure*>(proc.stack[0]);
			proc.stack[0] = (*c)[0/*k*/];
			proc.stack[1] = proc.stack[2/*r*/];
			proc.stack.pop();
		} NEXT_EXECUTOR;
	}
	return dead;
initialize:
	std::cout << WHATIS;
	return running;
}

