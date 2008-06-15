#ifndef EXECUTORS_H
#define EXECUTORS_H

/*
An Executor is effectively the core of a
function.  Arc functions are represented
by a closure structure, which has an
attached Executor object.
*/

class Process, Closure;

class Executor {
public:
	virtual void operator()(Process&, Closure&) const =0;
	virtual ~Executor(){};
};

#endif //EXECUTORS_H

