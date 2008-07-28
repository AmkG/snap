#include"variables.hpp"
#include"processes.hpp"
#include"types.hpp"
#include"atoms.hpp"
#include"bytecodes.hpp"
#include"executors.hpp"
#include"phandles.hpp"

#include<iostream>

void construct_test(Process&);

int main(int argc, const char* argv[]){
	variables_init();

	boost::shared_ptr<ProcessHandle> hproc = NewArcProcess();
	Process& proc=*(static_cast<Process*>(hproc->pproc.get()));
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
 (sym (quote y)) 2
 (sym (quote probe)) 3
 (sym (quote hmm)) 4
 (sym (quote x)) 5
 (sym (quote ccc)) 6
 (closure 0
          (variadic 2)
          (continue-local 2))
 (local 7)
 (global-set (quote list))
 (global (quote $))
 (local 1)
 (local 6)
 (local 4)
 (local 3)
 (local 5)
 (local 2)
 (k-closure 6
            (check-vars 2)
            (global (quote list))
            (local 0)
            (closure-ref 4) 'x
            (closure-ref 5) 'y
            (local 1)       probe
            (closure-ref 0) k
            (closure-ref 1) 'ccc
            (closure-ref 2) 'hmm
            (closure-ref 3) 'probe
            (k-closure-reuse 5
                             (closure-ref 0)
                             (local 0)
                             (local 1)
                             (closure-ref 1) k
                             (closure-ref 2) 'ccc
                             (closure-ref 3) 'hmm
                             (closure-ref 4) 'probe
                             (k-closure-reuse 4
                                              (global (quote $))
                                              (local 0)
                                              (closure-ref 3)
                                              (closure-ref 0)
                                              (closure-ref 3)
                                              (closure-ref 1)
                                              (closure-ref 2)
                                              (k-closure-reuse 4
                                                               (local 1)
                                                               (local 0)
                                                               (closure-ref 3)
                                                               (closure-ref 0)
                                                               (closure-ref 1)
                                                               (closure-ref 2)
                                                               (k-closure-reuse 3
                                                                                (global (quote $))
                                                                                (local 0)
                                                                                (closure-ref 2)
                                                                                (closure-ref 0)
                                                                                (closure-ref 1)
                                                                                (k-closure-reuse 2
                                                                                                 (closure-ref 1)
                                                                                                 (closure 1
                                                                                                          (check-vars 3)
                                                                                                          (global (quote $))
                                                                                                          (local 1)
                                                                                                          (local 2)
                                                                                                          (k-closure 2
                                                                                                                     (check-vars 2)
                                                                                                                     (local 1)
                                                                                                                     (closure-ref 0)
                                                                                                                     (closure-ref 1)
                                                                                                                     (apply 3))
                                                                                                          (closure-ref 0)
                                                                                                          (apply 3))
                                                                                                 (local 1)
                                                                                                 (closure-ref 0)
                                                                                                 (local 2)
                                                                                                 (apply 3))
                                                                                (apply-invert-k 3))
                                                               (apply-invert-k 3))
                                              (apply-invert-k 3))
                             (apply-invert-k 3))
            (apply-invert-k 4))
 (local 3)
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
		globals->lookup("sym"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("y"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("sym"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("probe"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("sym"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("hmm"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("sym"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("x"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("sym"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("ccc"));
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
		globals->lookup("variadic"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("continue-local"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,7);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("global-set"));
	bytecode_sym(proc,stack,
		globals->lookup("quote"));
	bytecode_sym(proc,stack,
		globals->lookup("list"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
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
		globals->lookup("local"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,6);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,4);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,3);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,5);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("k-closure"));
	bytecode_int(proc,stack,6);
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
		globals->lookup("list"));
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,4);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,5);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,3);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("k-closure-reuse"));
	bytecode_int(proc,stack,5);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,3);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,4);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("k-closure-reuse"));
	bytecode_int(proc,stack,4);
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
		globals->lookup("local"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,3);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,3);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("k-closure-reuse"));
	bytecode_int(proc,stack,4);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,3);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("k-closure-reuse"));
	bytecode_int(proc,stack,3);
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
		globals->lookup("local"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("k-closure-reuse"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,
		globals->lookup("check-vars"));
	bytecode_int(proc,stack,3);
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
		globals->lookup("local"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("k-closure"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,
		globals->lookup("check-vars"));
	bytecode_int(proc,stack,2);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,1);
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
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,0);
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
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,1);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("closure-ref"));
	bytecode_int(proc,stack,0);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,2);
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
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("apply-invert-k"));
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
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("apply-invert-k"));
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
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("apply-invert-k"));
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
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("apply-invert-k"));
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
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("apply-invert-k"));
	bytecode_int(proc,stack,4);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,NILATOM);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_sym(proc,stack,
		globals->lookup("local"));
	bytecode_int(proc,stack,3);
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
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
}


