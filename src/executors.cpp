#include"variables.hpp"
#include"executors.hpp"
#include"atoms.hpp"
#include"processes.hpp"
#include"bytecodes.hpp"
#include<iostream>
#include<boost/shared_ptr.hpp>
#include<map>

static boost::shared_ptr<Atom> QUOTEATOM;
static std::map<Atom*,_bytecode_label> bytetb;

template<class T>
static inline T* expect_type(Generic* g, char const* s1 = "executor-error", char const* s2 = "Unspecified error"){
	T* tmp = dynamic_cast<T*>(g);
	if(tmp == NULL) throw ArcError(s1, s2);
	return tmp;
}

static _bytecode_label bytecodelookup(boost::shared_ptr<Atom> a){
	std::map<Atom*, _bytecode_label>::iterator i = bytetb.find(&*a);
	if(i == bytetb.end()){
		return i->second;
	} else {
		throw ArcError("compile",
			"Unknown bytecode form");
	}
}

ProcessStatus execute(Process& proc, size_t reductions, bool init=0){
	if(init) goto initialize;
	DISPATCH_EXECUTORS {
		EXECUTOR(arc_executor):
		{	DISPATCH_BYTECODES{//provides the Closure* clos
				BYTECODE(car):
					bytecode_car(proc);
				NEXT_BYTECODE;
				BYTECODE(cdr):
					bytecode_cdr(proc);
				NEXT_BYTECODE;
				BYTECODE(cons):
					bytecode_cons(proc);
				NEXT_BYTECODE;
			}
		} NEXT_EXECUTOR;
		/*
		(fn (k#|1|# f#|2|#)
		  (f k (fn (_ r) (k r))))
		*/
		EXECUTOR(ccc):
		{	Closure* c = new(proc)
				Closure(THE_EXECUTOR(ccc_helper), 1);
			(*c)[0] = proc.stack[1/*k*/];
			proc.stack[0] = proc.stack[2/*f*/];
			proc.stack[2] = c;
		} NEXT_EXECUTOR;
		EXECUTOR(ccc_helper):
		{	Closure* c = expect_type<Closure>(proc.stack[0]);
			proc.stack[0] = (*c)[0/*k*/];
			proc.stack[1] = proc.stack[2/*r*/];
			proc.stack.pop();
		} NEXT_EXECUTOR;
		/*(fn (k l)
		    (compile_helper k (%empty-bytecode-sequence) l))
		*/
		EXECUTOR(compile):
		{	Closure* c = new(proc)
				Closure(THE_EXECUTOR(compile_helper), 0);
			ArcBytecodeSequence* bseq = new(proc)
				ArcBytecodeSequence();
			proc.stack[0] = c;
			/*just do type checking*/
			if(proc.stack[1]->istrue()){
				Cons* cp = expect_type<Cons>(proc.stack[1],
						"compile",
						"Expected bytecode list");
			}
			proc.stack.push(proc.stack[2]);
			proc.stack[2] = bseq;
		} NEXT_EXECUTOR;
		/* (fn (self k b l)
		     (if (no l)
		         (k b)
		         (let (c . l) l
		           (if (~acons c)
		               (do
		                 (%bytecode-sequence-append b (%bytecode (%lookup c)))
		                 (self k b l))
		               (let (c param . params) c
		                 (if
		                   (isa param 'int)
		                     (if params
		                         (self (%closure compile_intseq_bytecode self k b l c param)
		                               (%new-bytecode-sequence) params)
		                         (do
		                           (%bytecode-sequence-append b (%int-bytecode (%lookup c) param))
		                           (self k b l)))
		                   (and (acons param) (caris 'quote))
		                     (do
		                       (%bytecode-sequence-append b (%atom-bytecode (%lookup c) (cadr param)))
		                       (self k b l))
		                     (self (%closure compile_seq_bytecode self k b l c)
		                           (%new-bytecode-sequence) (cons param params))))))))
		*/
		EXECUTOR(compile_helper):
		{	ArcBytecodeSequence* b =
				dynamic_cast<ArcBytecodeSequence*>(
					proc.stack[2]);
		compile_helper_loop:
			/*check l (local 3)*/
			if(proc.stack[3]->isnil()){
				/*(k b)*/
				proc.stack.pop();
				proc.stack.restack(2);
			} else {
				/*split l into c and l*/
				Cons* l = expect_type<Cons>(proc.stack[3],
					"compile",
					"Expected bytecode list in "
					"compile_helper");
				/*c == (local 4)*/
				proc.stack.push(l->a);
				proc.stack[3] = l->d;
				/*determine if c is cons*/
				Cons* c = dynamic_cast<Cons*>(proc.stack[4]);
				if(c == NULL){/*(~acons c)*/
					/*Get the atom*/
					Sym* c = expect_type<Sym>(
						proc.stack[4],
						"compile",
						"Expected bytecode symbol");
					/*(%bytecode-sequence-append ...)*/
					b->append(new Bytecode(
							bytecodelookup(c->a)));
					/*Just pop off c*/
					proc.stack.pop();
					if(--reductions) goto compile_helper_loop; else return running;
				} else {
					/*destructure form*/
					proc.stack[4] = c->a;
					Cons* params = expect_type<Cons>(c->d,
						"compile",
						"Expected proper list for "
						"bytecode with parameter");
					/*param = local 5*/
					proc.stack.push(params->a);
					/*params = local 6*/
					proc.stack.push(params->d);
					Integer* param =
						dynamic_cast<Integer*>(
							params->a);
					if(param != NULL){/*(isa param 'int)*/
						/*check if params is null*/
						if(proc.stack[6]->istrue()){
							/*IntSeq*/
							Closure* clos = new(proc) Closure(THE_EXECUTOR(compile_intseq_bytecode), 6);
							for(int i = 0; i < 6; ++i){
								(*clos)[i] = proc.stack[i];
							}
							/*call new closure*/
							proc.stack[3] = proc.stack[0];
							proc.stack[4] = clos; // have to save this first!
							proc.stack[5] = b = new(proc) ArcBytecodeSequence();
							proc.stack.restack(4);
							if(--reductions) goto compile_helper_loop; else return running;
						} else {
							/*Int*/
							/*Get the atom*/
							Sym* c = expect_type<Sym>(
								proc.stack[4],
								"compile",
								"Expected bytecode symbol "
								"in int-type bytecode form");
							/*(%bytecode-sequence-append ...)*/
							b->append(new IntBytecode(bytecodelookup(c->a), param->integer()));
							/*pop off c, param, and params*/
							proc.stack.pop(3);
							if(--reductions) goto compile_helper_loop; else return running;
						}
					} else {
						Cons* param = dynamic_cast<Cons*>(params->a);
						if(param != NULL){/*(acons param)*/
							Sym* carparam = expect_type<Sym>(param->a,
								"compile",
								"Expected bytecode symbol or quote in parameter to bytecode");
							if(carparam->a == QUOTEATOM){
								Sym* c = expect_type<Sym>(
									proc.stack[4],
									"compile",
									"Expected bytecode symbol "
									"in symbol-parameter bytecode form");
								param = expect_type<Cons>(param->d,
									"compile",
									"Expected proper quote-form in symbol-parameter bytecode");
								carparam = expect_type<Sym>(param->a,
									"compile",
									"Expected symbol in symbol-parameter bytecode");
								b->append(new AtomBytecode(bytecodelookup(c->a), carparam->a));
								/*pop off c, param, and params*/
								proc.stack.pop(3);
								if(--reductions)goto compile_helper_loop; else return running;
							} else goto compile_helper_non_quote_param;
						}
						/*this is effectively our elsemost branch*/
					compile_helper_non_quote_param:
						/*NOTE!  Potential problem.  the params variable
						here is not the params in the source arc code in
						the comments.  Instead it is the (cons param params)
						from which they were destructured.  The problem
						here lies in the fact that it isn't in the Arc
						stack, which is significant if we have to GC.
						From my analysis so far however it seems that
						we don't alloc anything along the code path to
						this part, so we won't trigger GC.
						*/
						proc.stack[5] = params;
						proc.stack.pop();
						Closure* clos = new(proc) Closure(THE_EXECUTOR(compile_seq_bytecode), 5);
						for(int i = 0; i < 5; ++i){
							(*clos)[i] = proc.stack[i];
						}
						proc.stack[1] = clos; // Have to save this first!
						proc.stack[2] = b = new(proc) ArcBytecodeSequence();
						proc.stack[3] = proc.stack.top(); proc.stack.pop();
						if(--reductions) goto compile_helper_loop; else return running;
					}
				}
			}
		} NEXT_EXECUTOR;
		/*
		(fn (clos subseq)
		  (with (b (%closure-ref clos 2)
		         c (%closure-ref clos 4)
		         param (%closure-ref clos 5))
		    (%bytecode-sequence-append b
		      (%int-seq-bytecode (%lookup c) (%integer param) (%seq subseq)))
		    ; compile_helper       k                     b                     l
		    ((%closure-ref clos 0) (%closure-ref clos 1) (%closure-ref clos 2) (%closure-ref clos 3))))
		*/
		EXECUTOR(compile_intseq_bytecode):
		{	Closure* clos =
				dynamic_cast<Closure*>(proc.stack[0]);
			ArcBytecodeSequence* subseq =
				dynamic_cast<ArcBytecodeSequence*>(proc.stack[1]);
			/*(let ...)*/
			ArcBytecodeSequence* b =
				dynamic_cast<ArcBytecodeSequence*>(
					(*clos)[2]);
			/*the type of c wasn't checked in the first place*/
			Sym* c = expect_type<Sym>((*clos)[4],
				"compile",
				"Expected bytecode symbol "
				"in int-seq bytecode form");
			Integer* param =
				dynamic_cast<Integer*>(
					(*clos)[5]);
			b->append(new IntSeqBytecode(bytecodelookup(c->a), param->integer(), subseq->seq));
			/*push the closure elements*/
			for(int i = 0; i < 4; ++i){
				proc.stack.push((*clos)[i]);
			}
			proc.stack.restack(4);
		} NEXT_EXECUTOR;
		/*
		(fn (clos subseq)
		  (with (b (%closure-ref clos 2)
		         c (%closure-ref clos 4))
		    (%bytecode-sequence-append b
		      (%seq-bytecode (%lookup c) (%seq subseq)))
		    ((%closure-ref clos 0) (%closure-ref clos 1) (%closure-ref clos 2) (%closure-ref clos 3))))
		*/
		EXECUTOR(compile_seq_bytecode):
		{	Closure* clos =
				dynamic_cast<Closure*>(proc.stack[0]);
			ArcBytecodeSequence* subseq =
				dynamic_cast<ArcBytecodeSequence*>(proc.stack[1]);
			/*(let ...)*/
			ArcBytecodeSequence* b =
				dynamic_cast<ArcBytecodeSequence*>(
					(*clos)[2]);
			/*the type of c wasn't checked in the first place*/
			Sym* c = expect_type<Sym>((*clos)[4],
				"compile",
				"Expected bytecode symbol "
				"in seq bytecode form");
			b->append(new SeqBytecode(bytecodelookup(c->a), subseq->seq));
			/*push the closure elements*/
			for(int i = 0; i < 4; ++i){
				proc.stack.push((*clos)[i]);
			}
			proc.stack.restack(4);
		} NEXT_EXECUTOR;
		/*used to create a free function (no enclosed variables)
		from a bytecode sequence
		(fn (self k b)
		  (k (%closure b)))
		*/
		EXECUTOR(to_free_fun):
		{	ArcBytecodeSequence* b =
				expect_type<ArcBytecodeSequence>(proc.stack.top());
			Closure* clos =
				new(proc) Closure(THE_ARC_EXECUTOR(arc_executor, b->seq), 0);
			proc.stack[2] = clos;
			proc.stack.restack(2);
		} NEXT_EXECUTOR;
		EXECUTOR(halting_continuation):
			proc.stack.push(proc.stack[1]);
			proc.stack.restack(1);
			return dead;
		NEXT_EXECUTOR;
	}
	return dead;
initialize:
	QUOTEATOM = globals->lookup("quote");
	/*bytecodes*/
	bytetb[&*globals->lookup("car")] = THE_BYTECODE_LABEL(car);
	bytetb[&*globals->lookup("cdr")] = THE_BYTECODE_LABEL(cdr);
	bytetb[&*globals->lookup("cons")] = THE_BYTECODE_LABEL(cons);
	/*assign some bultin globals*/
	return running;
}

