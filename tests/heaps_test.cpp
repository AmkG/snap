#include"variables.hpp"
#include"processes.hpp"
#include<iostream>

int main(int argc, const char* argv[]){
	variables_init();

	/*tests!*/
	Process proc;
	/*allocate stuff*/
	//(cons t ...)
	proc.stack.push(new(proc) Sym(TATOM));//t
	//(cons t nil)
	proc.stack.push(new(proc) Sym(TATOM)); //t
	proc.stack.push(new(proc) Sym(NILATOM)); //nil
	//cons
	{
		Cons* cp = new(proc) Cons();
		cp->a = proc.stack.top(2);
		cp->d = proc.stack.top();
		proc.stack.top(2) = cp;
		proc.stack.pop();
	}
	//another cons
	{
		Cons* cp = new(proc) Cons();
		cp->a = proc.stack.top(2);
		cp->d = proc.stack.top();
		proc.stack.top(2) = cp;
		proc.stack.pop();
	}

	proc.stack.top()->probe(0);
}

