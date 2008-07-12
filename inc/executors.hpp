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
which it must execute itself.

proc.stack[0] contains the closure.  If the Executor
represents a "normal" function, proc.stack[1] is the
"continuation" function and proc.stack[2..n] are the
parameters.  proc.stack.size() is the number of
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
#define EXECUTOR_ENUM(x)  PASTE_SYMBOLS(_executor_, x)
#define AN_EXECUTOR(x) EXECUTOR_ENUM(x),
#define END_DECLARE_EXECUTORS __null_executor };

/*NOTE!  no ; or : or {} or other markers*/
DECLARE_EXECUTORS
	AN_EXECUTOR(arc_executor)
	AN_EXECUTOR(ccc)
	AN_EXECUTOR(ccc_helper)
	AN_EXECUTOR(compile)
	AN_EXECUTOR(compile_helper)
	AN_EXECUTOR(compile_intseq_bytecode)
	AN_EXECUTOR(compile_seq_bytecode)
	AN_EXECUTOR(to_free_fun)
	AN_EXECUTOR(halting_continuation)
	AN_EXECUTOR(bif_dispatch)
END_DECLARE_EXECUTORS

#define DECLARE_BYTECODES enum _e_bytecode_label{
#define BYTECODE_ENUM(x) PASTE_SYMBOLS(_bytecode_, x)
#define A_BYTECODE(x) BYTECODE_ENUM(x),
#define END_DECLARE_BYTECODES __null_bytecode };
DECLARE_BYTECODES
	A_BYTECODE(apply)
	A_BYTECODE(apply_list)
	A_BYTECODE(car)
	A_BYTECODE(car_local_push)
	A_BYTECODE(car_clos_push)
	A_BYTECODE(cdr)
	A_BYTECODE(cdr_local_push)
	A_BYTECODE(cdr_clos_push)
	A_BYTECODE(check_vars)
	A_BYTECODE(closure)
	A_BYTECODE(cons)
	A_BYTECODE(b_continue)
	A_BYTECODE(continue_local)
	A_BYTECODE(continue_on_clos)
	A_BYTECODE(global)
	A_BYTECODE(global_set)
	A_BYTECODE(halt)
	A_BYTECODE(halt_local_push)
	A_BYTECODE(halt_clos_push)
	A_BYTECODE(b_if)
	A_BYTECODE(if_local)
	A_BYTECODE(b_int)
	A_BYTECODE(local)
	A_BYTECODE(rep)
	A_BYTECODE(rep_local_push)
	A_BYTECODE(rep_clos_push)
	A_BYTECODE(sv)
	A_BYTECODE(sv_local_push)
	A_BYTECODE(sv_clos_push)
	A_BYTECODE(sv_ref)
	A_BYTECODE(sv_ref_local_push)
	A_BYTECODE(sv_ref_clos_push)
	A_BYTECODE(sv_set)
	A_BYTECODE(sym)
	A_BYTECODE(tag)
	A_BYTECODE(type)
	A_BYTECODE(type_local_push)
	A_BYTECODE(type_clos_push)
	A_BYTECODE(variadic)
END_DECLARE_BYTECODES


#ifdef __GNUC__
/*use indirect goto when using GCC*/
typedef void* _executor_label;
#define EXECUTOR_GOTO(x) goto *x
#define DISPATCH_EXECUTORS enum _e_executor_label pc; NEXT_EXECUTOR;
#define EXECUTOR(x) pc = EXECUTOR_ENUM(x); PASTE_SYMBOLS(label_, x)
#define THE_EXECUTOR_LABEL(x) &&PASTE_SYMBOLS(label_, x)

typedef void* _bytecode_label;
#define DISPATCH_BYTECODES \
	Closure& clos = *static_cast<Closure*>(proc.stack[0]);\
	ArcExecutor const* e =\
		static_cast<ArcExecutor const*>(&clos.code());\
	Bytecode* current_bytecode = &*e->b->head; \
	enum _e_bytecode_label bpc; NEXT_BYTECODE;
#define BYTECODE_GOTO(x) goto *x
#define BYTECODE(x) bpc = BYTECODE_ENUM(x); PASTE_SYMBOLS(label_b_, x)
#define THE_BYTECODE_LABEL(x) &&PASTE_SYMBOLS(label_b_, x)

#else //__GNUC__

/*use an enum when using standard C*/
typedef enum _e_executor_label _executor_label;
#define EXECUTOR_GOTO(x) {pc = (x); goto executor_switch;}
#define DISPATCH_EXECUTORS _executor_label pc; NEXT_EXECUTOR;\
	executor_switch: switch(pc)
#define EXECUTOR(x) case EXECUTOR_ENUM(x)
#define THE_EXECUTOR_LABEL(x) EXECUTOR_ENUM(x)

typedef enum _e_bytecode_label _bytecode_label;
#define BYTECODE_GOTO(x) {bpc = (x); goto bytecode_switch;}
#define DISPATCH_BYTECODES \
	Closure& clos = *static_cast<Closure*>(proc.stack[0]);\
	ArcExecutor const* e =\
		static_cast<ArcExecutor const*>(&clos.code());\
	Bytecode* current_bytecode = &*e->b->head; \
	enum _e_bytecode_label bpc; NEXT_BYTECODE; \
	bytecode_switch: switch(bpc)
#define BYTECODE(x) case BYTECODE_ENUM(x)
#define THE_BYTECODE_LABEL(x) BYTECODE_ENUM(x)

#endif//__GNUC__

#define THE_EXECUTOR(x) new Executor(THE_EXECUTOR_LABEL(x))
#define THE_ARC_EXECUTOR(x,y) new ArcExecutor(THE_EXECUTOR_LABEL(x), y)

/*TODO: 'call* / 'defcall support*/
#define NEXT_EXECUTOR if(--reductions != 0){ Closure* c = dynamic_cast<Closure*>(proc.stack[0]);\
	/*TODO: insert check that c is not NULL; if NULL, lookup in call* */\
	if(c->code().l == THE_EXECUTOR_LABEL(arc_executor)) goto arc_executor_top;\
	EXECUTOR_GOTO((c->code()).l);} else {return process_running;}

#define NEXT_BYTECODE { current_bytecode = &*current_bytecode->next;\
	BYTECODE_GOTO(current_bytecode->l);}

#define GOTO_BYTECODE(x) { current_bytecode = (x);\
	BYTECODE_GOTO(current_bytecode->l);}

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

class Bytecode{
public:
	virtual ~Bytecode(){};
	Bytecode(_bytecode_label nl) : l(nl), next() {};
	_bytecode_label l;
	boost::scoped_ptr<Bytecode> next;
};

/*bytecode with an int parameter*/
class IntBytecode : public Bytecode {
public:
	virtual ~IntBytecode(){};
	IntBytecode(_bytecode_label nl, int nnum) : Bytecode(nl), num(nnum){};
	int num;
};

/*bytecode with a bytecode sequence parameter
(e.g. 'if, 'fn)
NOTE!  recursive references not allowed!
*/
class SeqBytecode : public Bytecode {
public:
	virtual ~SeqBytecode(){};
	SeqBytecode(_bytecode_label nl,
		    boost::shared_ptr<BytecodeSequence> nseq)
		: Bytecode(nl), seq(nseq) {};
	boost::shared_ptr<BytecodeSequence> seq;
};

class IntSeqBytecode : public Bytecode{
public:
	virtual ~IntSeqBytecode(){ };
	IntSeqBytecode(_bytecode_label nl,
		    int nnum,
		    boost::shared_ptr<BytecodeSequence> nseq)
		: Bytecode(nl), num(nnum), seq(nseq) {};
	int num;
	boost::shared_ptr<BytecodeSequence> seq;
};

/*bytecode with an atom parameter
(e.g. 'global, 'symbol)
*/
class AtomBytecode : public Bytecode{
public:
	virtual ~AtomBytecode (){};
	AtomBytecode(_bytecode_label nl, boost::shared_ptr<Atom> natom)
		: Bytecode(nl), atom(natom) {};
	boost::shared_ptr<Atom> atom;
};

class BytecodeSequence {
private:
	Bytecode* tail;
public:
	boost::scoped_ptr<Bytecode> head;
	void append(Bytecode* b){
		if(tail != NULL){
			tail->next.reset(b);
		} else {
			head.reset(b);
		}
		tail = b;
	}
	BytecodeSequence() : tail(NULL), head(NULL) {}
};

/*Bytecode definition helper macros*/
/*WARNING!  Assumes that the bytecodes are correct in the first place!*/
#define INTPARAM(i)\
	int& i = (static_cast<IntBytecode*>(current_bytecode))->num

#define SEQPARAM(i)\
	boost::shared_ptr<BytecodeSequence>& i =\
		(static_cast<SeqBytecode*>(current_bytecode))->seq

#define INTSEQPARAM(i,s)\
	int& i = (static_cast<IntSeqBytecode*>(current_bytecode))->num;\
	boost::shared_ptr<BytecodeSequence>& s =\
		(static_cast<IntSeqBytecode*>(current_bytecode))->seq

#define ATOMPARAM(s)\
	boost::shared_ptr<Atom>& s =\
		(static_cast<AtomBytecode*>(current_bytecode))->atom;

class Process;

#include"processes.hpp"

ProcessStatus execute(Process& proc, size_t reductions, bool init=0);

#endif //EXECUTORS_H

