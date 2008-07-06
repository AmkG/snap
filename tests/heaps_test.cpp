#include"variables.hpp"
#include"processes.hpp"
#include"bytecodes.hpp"
#include<iostream>

int main(int argc, const char* argv[]){
	variables_init();

	/*tests!*/
	Process proc;
	ProcessStack& stack = proc.stack;
	/*allocate stuff*/
	//(cons 'bar ...)
	proc.stack.push(new(proc) Sym(globals->lookup("bar")));
	//(cons 'foo ...)
	proc.stack.push(new(proc) Sym(globals->lookup("foo")));
	//(cons t ...)
	proc.stack.push(new(proc) Sym(TATOM));//t
	//(cons t nil)
	proc.stack.push(new(proc) Sym(TATOM)); //t
	proc.stack.push(new(proc) Sym(NILATOM)); //nil
	//cons
	bytecode_cons(proc,stack);
	//repush stack top to make shared structure
	proc.stack.push(proc.stack.top());
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);
	//int
	proc.stack.push(new(proc) Integer(1));
	proc.stack.push(new(proc) Integer(-1));
	bytecode_cons(proc,stack);
	bytecode_cons(proc,stack);

	proc.stack.top()->probe(0);
}

