#include"variables.hpp"
#include"executors.hpp"
#include"atoms.hpp"
#include"processes.hpp"
#include"bytecodes.hpp"
#include<iostream>
#include<boost/shared_ptr.hpp>
#include<map>

static boost::shared_ptr<Atom> QUOTEATOM;
static boost::shared_ptr<Atom> CCCATOM;
static boost::shared_ptr<Atom> BYTECODERATOM;
static boost::shared_ptr<Atom> FREEFUNATOM;
static boost::shared_ptr<Atom> HALTATOM;
static std::map<Atom*,_bytecode_label> bytetb;

template<class T>
static inline T* expect_type(Generic* g,
				char const* s1 = "executor-error",
				char const* s2 = "Unspecified error"){
	T* tmp = dynamic_cast<T*>(g);
	if(tmp == NULL) throw ArcError(s1, s2);
	return tmp;
}

static _bytecode_label bytecodelookup(boost::shared_ptr<Atom> a){
	std::map<Atom*, _bytecode_label>::iterator i = bytetb.find(&*a);
	if(i != bytetb.end()){
		return i->second;
	} else {
		throw ArcError("compile",
			"Unknown bytecode form");
	}
}

ProcessStatus execute(Process& proc, size_t reductions, bool init=0){
	ProcessStack& stack = proc.stack;
	if(init) goto initialize;
	DISPATCH_EXECUTORS {
		EXECUTOR(arc_executor):
		arc_executor_top://must be named exactly so, and must exist
		{	DISPATCH_BYTECODES{//provides the Closure clos
				BYTECODE(apply):
				{INTPARAM(N);
					stack.restack(N);
				} /***/ NEXT_EXECUTOR; /***/
				BYTECODE(apply_list):
					Generic* tmp;
					stack.restack(3);
					/*destructure until stack top is nil*/
					while(stack.top()->istrue()){
						tmp = stack.top();
						bytecode_car(stack);
						stack.push(tmp);
						bytecode_cdr(stack);
					}
					stack.pop();
				/***/ NEXT_EXECUTOR; /***/
				BYTECODE(car):
					bytecode_car(stack);
				NEXT_BYTECODE;
				BYTECODE(car_local_push):
				{INTPARAM(N);
					bytecode_car_local_push(stack,N);
				} NEXT_BYTECODE;
				BYTECODE(car_clos_push):
				{INTPARAM(N);
					bytecode_car_clos_push(stack,clos,N);
				} NEXT_BYTECODE;
				BYTECODE(cdr):
					bytecode_cdr(stack);
				NEXT_BYTECODE;
				BYTECODE(cdr_local_push):
				{INTPARAM(N);
					bytecode_cdr_local_push(stack,N);
				} NEXT_BYTECODE;
				BYTECODE(cdr_clos_push):
				{INTPARAM(N);
					bytecode_cdr_clos_push(stack,clos,N);
				} NEXT_BYTECODE;
				BYTECODE(check_vars):
				{INTPARAM(N);
					if(N != stack.size()){
						throw ArcError("apply",
							"Incorrect number of "
							"parameters");
					}
				} NEXT_BYTECODE;
				BYTECODE(closure):
				{INTSEQPARAM(N,S);
					Closure* nclos =
						new(proc) Closure(
							THE_ARC_EXECUTOR(
								arc_executor,
								S),
							N);
					for(int i = N; i ; --i){
						(*nclos)[i - 1] = stack.top();
						stack.pop();
					}
					stack.push(nclos);
				} NEXT_BYTECODE;
				BYTECODE(cons):
					bytecode_cons(proc,stack);
				NEXT_BYTECODE;
				BYTECODE(b_continue):
					stack.top(2) = stack[1];
					stack.restack(2);
				/***/ NEXT_EXECUTOR; /***/
			}
		} NEXT_EXECUTOR;
		/*
		(fn (k#|1|# f#|2|#)
		  (f k (fn (_ r) (k r))))
		*/
		EXECUTOR(ccc):
		{	Closure* c = new(proc)
				Closure(THE_EXECUTOR(ccc_helper), 1);
			(*c)[0] = stack[1/*k*/];
			stack[0] = stack[2/*f*/];
			stack[2] = c;
		} NEXT_EXECUTOR;
		EXECUTOR(ccc_helper):
		{	Closure* c = expect_type<Closure>(stack[0]);
			stack[0] = (*c)[0/*k*/];
			stack[1] = stack[2/*r*/];
			stack.pop();
		} NEXT_EXECUTOR;
		/*(fn (k l)
		    (compile_helper k (%empty-bytecode-sequence) l))
		*/
		EXECUTOR(compile):
		{	Closure* c = new(proc)
				Closure(THE_EXECUTOR(compile_helper), 0);
			ArcBytecodeSequence* bseq = new(proc)
				ArcBytecodeSequence();
			stack[0] = c;
			/*just do type checking*/
			if(stack[1]->istrue()){
				Cons* cp = expect_type<Cons>(stack[1],
						"compile",
						"Expected bytecode list");
			}
			stack.push(stack[2]);
			stack[2] = bseq;
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
					stack[2]);
		compile_helper_loop:
			/*check l (local 3)*/
			if(stack[3]->isnil()){
				/*(k b)*/
				stack.pop();
				stack.restack(2);
			} else {
				/*split l into c and l*/
				Cons* l = expect_type<Cons>(stack[3],
					"compile",
					"Expected bytecode list in "
					"compile_helper");
				/*c == (local 4)*/
				stack.push(l->a);
				stack[3] = l->d;
				/*determine if c is cons*/
				Cons* c = dynamic_cast<Cons*>(stack[4]);
				if(c == NULL){/*(~acons c)*/
					/*Get the atom*/
					Sym* c = expect_type<Sym>(
						stack[4],
						"compile",
						"Expected bytecode symbol");
					/*(%bytecode-sequence-append ...)*/
					b->append(new Bytecode(
							bytecodelookup(c->a)));
					/*Just pop off c*/
					stack.pop();
					if(--reductions) goto compile_helper_loop; else return process_running;
				} else {
					/*destructure form*/
					stack[4] = c->a;
					Cons* params = expect_type<Cons>(c->d,
						"compile",
						"Expected proper list for "
						"bytecode with parameter");
					/*param = local 5*/
					stack.push(params->a);
					/*params = local 6*/
					stack.push(params->d);
					Integer* param =
						dynamic_cast<Integer*>(
							params->a);
					if(param != NULL){/*(isa param 'int)*/
						/*check if params is null*/
						if(stack[6]->istrue()){
							/*IntSeq*/
							Closure* clos = new(proc) Closure(THE_EXECUTOR(compile_intseq_bytecode), 6);
							for(int i = 0; i < 6; ++i){
								(*clos)[i] = stack[i];
							}
							/*call new continuation*/
							stack[3] = stack[0];
							stack[4] = clos; // have to save this first!
							stack[5] = b = new(proc) ArcBytecodeSequence(); // make sure to reassign b
							stack.restack(4);
							if(--reductions) goto compile_helper_loop; else return process_running;
						} else {
							/*Int*/
							/*Get the atom*/
							Sym* c = expect_type<Sym>(
								stack[4],
								"compile",
								"Expected bytecode symbol "
								"in int-type bytecode form");
							/*(%bytecode-sequence-append ...)*/
							b->append(new IntBytecode(bytecodelookup(c->a), param->integer()));
							/*pop off c, param, and params*/
							stack.pop(3);
							if(--reductions) goto compile_helper_loop; else return process_running;
						}
					} else {
						Cons* param = dynamic_cast<Cons*>(params->a);
						if(param != NULL){/*(acons param)*/
							Sym* carparam = expect_type<Sym>(param->a,
								"compile",
								"Expected bytecode symbol or quote in parameter to bytecode");
							if(carparam->a == QUOTEATOM){
								Sym* c = expect_type<Sym>(
									stack[4],
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
								stack.pop(3);
								if(--reductions)goto compile_helper_loop; else return process_running;
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
						stack[5] = params;
						stack.pop();
						Closure* clos = new(proc) Closure(THE_EXECUTOR(compile_seq_bytecode), 5);
						for(int i = 0; i < 5; ++i){
							(*clos)[i] = stack[i];
						}
						stack[1] = clos; // Have to save this first!
						stack[2] = b = new(proc) ArcBytecodeSequence(); // make sure to reassign b
						stack[3] = stack.top(); stack.pop();
						if(--reductions) goto compile_helper_loop; else return process_running;
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
				dynamic_cast<Closure*>(stack[0]);
			ArcBytecodeSequence* subseq =
				dynamic_cast<ArcBytecodeSequence*>(stack[1]);
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
				stack.push((*clos)[i]);
			}
			stack.restack(4);
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
				dynamic_cast<Closure*>(stack[0]);
			ArcBytecodeSequence* subseq =
				dynamic_cast<ArcBytecodeSequence*>(stack[1]);
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
				stack.push((*clos)[i]);
			}
			stack.restack(4);
		} NEXT_EXECUTOR;
		/*used to create a free function (no enclosed variables)
		from a bytecode sequence
		(fn (self k b)
		  (k (%closure b)))
		*/
		EXECUTOR(to_free_fun):
		{	ArcBytecodeSequence* b =
				expect_type<ArcBytecodeSequence>(stack.top());
			Closure* clos =
				new(proc) Closure(THE_ARC_EXECUTOR(arc_executor, b->seq), 0);
			stack[2] = clos;
			stack.restack(2);
		} NEXT_EXECUTOR;
		EXECUTOR(halting_continuation):
			stack.push(stack[1]);
			stack.restack(1);
			return process_dead;
		NEXT_EXECUTOR;
		/* this function implements the ($ ...) dispatcher
		not very efficient, since we construct new closures
		each time; obviously the arc-side support has to
		put them in some sort of enclosed local variable
		(fn (self k s)
		  (case s
		    ...))
		*/
		EXECUTOR(bif_dispatch):
		{	Sym* s = expect_type<Sym>(stack[2],
					"$",
					"built-in dispatcher "
					"expected a symbol");
			/*if-else-if chain*/
			if(s->a == CCCATOM){
				stack[2] =
					new(proc)
					Closure(THE_EXECUTOR(ccc), 0);
			} else if(s->a == BYTECODERATOM){
				stack[2] =
					new(proc)
					Closure(THE_EXECUTOR(compile), 0);
			} else if(s->a == FREEFUNATOM){
				stack[2] =
					new(proc)
					Closure(THE_EXECUTOR(to_free_fun), 0);
			} else if(s->a == HALTATOM){
				stack[2] =
					new(proc)
					Closure(THE_EXECUTOR(halting_continuation), 0);
			} else {
				stack[2] = new(proc) Sym(NILATOM);
			}
			stack.restack(2);
		} NEXT_EXECUTOR;
	}
	return process_dead;
initialize:
	/*atoms*/
	QUOTEATOM = globals->lookup("quote");
	CCCATOM = globals->lookup("ccc");
	BYTECODERATOM = globals->lookup("bytecoder");
	FREEFUNATOM = globals->lookup("bytecode-to-free-fun");
	HALTATOM = globals->lookup("halt");
	/*bytecodes*/
	bytetb[&*globals->lookup("apply")] = THE_BYTECODE_LABEL(apply);
	bytetb[&*globals->lookup("apply-list")] =
		THE_BYTECODE_LABEL(apply_list);
	bytetb[&*globals->lookup("car")] = THE_BYTECODE_LABEL(car);
	bytetb[&*globals->lookup("car-local-push")] =
		THE_BYTECODE_LABEL(car_local_push);
	bytetb[&*globals->lookup("car-clos-push")] =
		THE_BYTECODE_LABEL(car_clos_push);
	bytetb[&*globals->lookup("cdr")] = THE_BYTECODE_LABEL(cdr);
	bytetb[&*globals->lookup("cdr-local-push")] =
		THE_BYTECODE_LABEL(cdr_local_push);
	bytetb[&*globals->lookup("cdr-clos-push")] =
		THE_BYTECODE_LABEL(cdr_clos_push);
	bytetb[&*globals->lookup("check-vars")] =
		THE_BYTECODE_LABEL(check_vars);
	bytetb[&*globals->lookup("closure")] = THE_BYTECODE_LABEL(closure);
	bytetb[&*globals->lookup("cons")] = THE_BYTECODE_LABEL(cons);
	bytetb[&*globals->lookup("continue")] =
		THE_BYTECODE_LABEL(b_continue);
	/*assign bultin global*/
	proc.assign(globals->lookup("$"),
		new(proc) Closure(THE_EXECUTOR(bif_dispatch), 0));
	return process_running;
}

