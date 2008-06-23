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

#define PASTE_SYMBOLS(x,y) x##y

/*
Not necessary when using GCC.
NOTE!  We retain this and even add references to
an _e_executor_label type so that modders who
happen to use *only* GCC will remember to update
the declarations for the benefit of non-GCC
users.
*/
#define DECLARE_EXECUTORS enum _e_executor_label {
#define AN_EXECUTOR(x)  x,
#define END_DECLARE_EXECUTORS __null_executor };

/*NOTE!  no ; or : or {} or other markers*/
DECLARE_EXECUTORS
	AN_EXECUTOR(arc_executor)
	AN_EXECUTOR(ccc)
	AN_EXECUTOR(ccc_helper)
END_DECLARE_EXECUTORS


#ifdef __GNUC__
/*use indirect goto when using GCC*/
typedef void* _executor_label;
#define EXECUTOR_GOTO(x) goto *x
#define DISPATCH_EXECUTORS enum _e_executor_label pc; NEXT_EXECUTOR;
#define EXECUTOR(x) pc = x; PASTE_SYMBOLS(label_, x)
#define THE_EXECUTOR(x) new Executor(&&PASTE_SYMBOLS(label_, x))

#define WHATIS "GNU"

#else //__GNUC__

#define WHATIS "NONGNU"

/*use an enum when using standard C*/
typedef enum _e_executor_label _executor_label;
#define EXECUTOR_GOTO(x) {pc = (x); goto executor_switch;}
#define DISPATCH_EXECUTORS _executor_label pc; NEXT_EXECUTOR;\
	executor_switch: switch(pc)
#define EXECUTOR(x) case x
#define THE_EXECUTOR(x) new Executor(x)

#endif//__GNUC__

/*TODO: 'call* / 'defcall support*/
#define NEXT_EXECUTOR { Closure* c = dynamic_cast<Closure*>(proc.stack[0]);\
	EXECUTOR_GOTO((c->code()).l);}

class Executor{
public:
	virtual ~Executor(){};
	Executor(_executor_label nl) : l(nl) {};
	_executor_label l;
};
class BytecodeSequence;
class ArcExecutor : public Executor{
public:
	virtual ~ArcExecutor(){};
	boost::shared_ptr<BytecodeSequence> b;
	ArcExecutor(_executor_label nl, boost::shared_ptr<BytecodeSequence> nb)
		: Executor(nl), b(nb) {};
};

#endif //EXECUTORS_H

