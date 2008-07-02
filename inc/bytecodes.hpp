#ifndef BYTECODES_H
#define BYTECODES_H
#include"processes.hpp"
#include"types.hpp"
#include"errors.hpp"
#include<boost/shared_ptr.hpp>

/*
By defining the actual bytecode implementation
here, we can (?) possibly make it easier to
change the interpreter system (for example,
direct threading, maybe even dynamic inlining
some day)
*/

/*parameters are on-stack*/
inline void bytecode_cons(Process& proc, ProcessStack& stack){
	Cons* cp = new(proc) Cons();
	cp->a = stack.top(2);
	cp->d = stack.top(1);
	stack.top(2) = cp;
	stack.pop();
}
inline void bytecode_sym(Process& proc, ProcessStack& stack,
			boost::shared_ptr<Atom> a){
	stack.push(new(proc) Sym(a));
}
inline void bytecode_car(ProcessStack& stack){
	//(car nil) => nil
	if(stack.top()->istrue()){
		Cons* cp = dynamic_cast<Cons*>(stack.top());
		if(cp == NULL) throw ArcError("badargs",
				"'car expects an argument of type 'cons");
		stack.top() = cp->a;
	}
}
inline void bytecode_car_local_push(ProcessStack& stack, int N){
	if(stack[N]->istrue()){
		Cons* cp = dynamic_cast<Cons*>(stack[N]);
		if(cp == NULL) throw ArcError("badargs",
				"'car expects an argument of type 'cons");
		stack.push(cp->a);
	}
}
inline void bytecode_cdr(ProcessStack& stack){
	//(car nil) => nil
	if(stack.top()->istrue()){
		Cons* cp = dynamic_cast<Cons*>(stack.top());
		if(cp == NULL) throw ArcError("badargs",
				"'cdr expects an argument of type 'cons");
		stack.top() = cp->d;
	}
}

#endif //BYTECODES_H

