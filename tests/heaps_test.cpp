#define NO_EXTERN
#include"variables.hpp"
#include"processes.hpp"
#include<iostream>

int main(int argc, const char* argv[]){
	globals.reset(new Globals());
	NILATOM = globals->lookup("nil");
	TATOM = globals->lookup("t");

	/*tests!*/
	Process proc;
	/*allocate stuff*/
	//(cons t nil)
	proc.stack.push(new(proc) Sym(TATOM)); //t
	proc.stack.push(new(proc) Sym(NILATOM)); //nil
	std::cout << proc.stack.size() << std::endl;
	//cons
	Cons* cp = new(proc) Cons();
	cp->a = proc.stack.top(2);
	cp->d = proc.stack.top();
	proc.stack.top(2) = cp;
	proc.stack.pop();

	proc.stack.top()->probe(0);
}

