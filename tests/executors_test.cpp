#include"variables.hpp"
#include"processes.hpp"
#include"types.hpp"
#include"atoms.hpp"
#include"bytecodes.hpp"
#include"executors.hpp"

#include<iostream>

void construct_test(Process&);

int main(int argc, const char* argv[]){
	variables_init();
	boost::shared_ptr<ProcessBase> pproc = NewArcProcess();
	Process& proc=*(static_cast<Process*>(pproc.get()));
	execute(proc, 0, 1); /*initialize*/

	ProcessStack& stack = proc.stack;

	/*compile the bytecode*/
	stack.push(proc.get(globals->lookup("<snap>compile</snap>")));
	stack.push(proc.get(globals->lookup("<snap>halt</snap>")));
	construct_test(proc);
	/*execute it*/
	std::cout << "compiling to bytecode" << std::endl;
	while(proc.run() != process_dead){
	}
	/*transform bytecode object to global function object*/
	stack.push(proc.get(globals->lookup("<snap>to-free-fun</snap>")));
	stack.push(proc.get(globals->lookup("<snap>halt</snap>")));
	stack.push(stack[0]);
	stack.restack(3);
	/*execute it*/
	std::cout << "converting to free function" << std::endl;
	while(proc.run() != process_dead){
	}

	/*finally, execute the process*/
	stack.push(proc.get(globals->lookup("<snap>halt</snap>")));
	stack.restack(2);
	std::cout << "executing free function" << std::endl;
	while(proc.run() != process_dead){
	}
	stack.top()->probe(0);
	std::cout << "done" << std::endl;
}

void construct_test(Process& proc){
/*
((check-vars 2)
 (global (quote $))
 (closure 0
          (check-vars 2)
          (sym (quote foo))
          (int 1)
          cons
          (global-set 'temp)
          (global 'temp)
          continue)
 (sym (quote halt))
 (apply 3))
*/
	ProcessStack& stack = proc.stack;
	bytecode_sym(proc,stack,
		globals->lookup("check-vars"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("global"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("$"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,
		globals->lookup("check-vars"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("sym"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("foo"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("int"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("cons"));
	bytecode_sym(proc,stack,
		globals->lookup("continue"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("sym"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("halt"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("apply"));
	bytecode_int(proc,stack,3);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
}


