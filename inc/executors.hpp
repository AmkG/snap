#ifndef EXECUTORS_H
#define EXECUTORS_H

/*
An Executor is effectively the core of a
function.  Arc functions are represented
by a closure structure, which has an
attached Executor object.
*/
/*
Calling convention:
The Executor is given a Process which is the process within
which it must execute itself.  Assume you have:
	virtual void YourExecutor::operator()(Process& proc,
					Closure& clos) const{
		...
	}
proc.stack[0] contains the closure (the same as clos).  If
the Executor represents a "normal" function, proc.stack[1]
is the "continuation" function and proc.stack[2..n] are
the parameters.  proc.stack.size() is the number of
parameters including the function itself and the
continuation.

If the Executor represents a "continuation" function,
proc.stack[1] is always the return value.  proc.stack.size()
will be 2.

When the Executor returns normally (i.e. without throwing)
proc.stack[0] must contain the "next" closure to call,
with the stack set up as described above. (as a hint, you
can probably just push the values on the stack and then
call proc.stack.restack(n))

proc.stack[0] *must* be a Closure with a valid Executor.
*/

#ifdef _GNUC_
/*use indirect goto when using GCC*/
typedef void* _executor_label;
#define GOTO(x) goto *(x)

#else //_GNUC_
/*use an enum when using GCC*/
typedef enum _e_executor_label _executor_label;
#define GOTO(x) {pc = (x); goto executor_switch;}

#endif//_GNUC_

#endif //EXECUTORS_H

