#include"variables.hpp"
#include"executors.hpp"
#include"atoms.hpp"
#include"processes.hpp"
#include"phandles.hpp"
#include"runsystems.hpp"
#include"bytecodes.hpp"
#include<iostream>
#include<boost/shared_ptr.hpp>
#include<map>

static boost::shared_ptr<Atom> QUOTEATOM;

static std::map<Atom*,_bytecode_label> bytetb;
static std::map<Atom*, boost::shared_ptr<Executor> > biftb;

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

class InitialAssignments {
public:
	InitialAssignments const& operator()(
			char const* s,
			Executor* e) const {
		biftb[globals->lookup(s).get()].reset(e);
		return *this;
	}
	InitialAssignments const& operator()(
			char const* s,
			_bytecode_label l) const{
		bytetb[globals->lookup(s).get()] = l;
		return *this;
	}
	InitialAssignments const& operator()(
			char const* s,
			Process& proc,
			Executor* e) const{
		proc.assign(globals->lookup(s), 
			NewClosure(proc, e, 0));
		return *this;
	}
};

ProcessStatus execute(Process& proc, size_t reductions, bool init){
	/*REMINDER
	All allocations of Generic objects on the
	Process proc *will* invalidate any pointers
	and references to Generic objects in that
	process.
	i.e. new(proc) Anything() will *always*
	invalidate any existing pointers.
	(technically it will only do so if a GC
	is triggered but hey, burned once, burned
	for all eternity)
	(oh and yeah: proc.nilobj() and proc.tobj()
	are both potentially allocating)
	*/
	ProcessStack& stack = proc.stack;
	if(init) goto initialize;
	DISPATCH_EXECUTORS {
		EXECUTOR(arc_executor):
		/*label must be named exactly so, and
		must exist (executor dispatching
		special-cases arc_executor; this will
		hopefully speed up dispatch slightly
		for some processors)
		*/
		arc_executor_top:
		{	DISPATCH_BYTECODES{
				BYTECODE(apply):
				{INTPARAM(N);
					stack.restack(N);
				} /***/ NEXT_EXECUTOR; /***/
				/*Used by a continuation that would
				like to reuse its closure if possible.
				It can't reuse its closure too early
				because the arguments to the next
				function are likely to depend on the
				closure entries, so it must first
				compute the arguments, then compute
				the values for closure, then finally
				construct the closure.  This means
				that the continuation gets pushed
				last; this bytecode inverts the k
				*/
				BYTECODE(apply_invert_k):
				{INTPARAM(N);
					Generic* k = stack.top(); stack.pop();
					stack.top(N-1) = k;
					stack.restack(N);
				} /***/ NEXT_EXECUTOR; /***/
				BYTECODE(apply_list):
					Generic* tmp;
					stack.restack(3);
					/*destructure until stack top is nil*/
					while(stack.top()->istrue()){
						tmp = stack.top();
						bytecode_<&Generic::car>(
							stack);
						// we don't expect car to
						// allocate, so tmp should
						// still be valid
						stack.push(tmp);
						bytecode_<&Generic::cdr>(
							stack);
					}
					stack.pop();
				/***/ NEXT_EXECUTOR; /***/
				BYTECODE(car):
					/*exact implementation is in
					inc/bytecodes.hpp ; we can
					actually change the interpreter
					system.
					*/
					bytecode_<&Generic::car>(stack);
				NEXT_BYTECODE;
				BYTECODE(car_local_push):
				{INTPARAM(N);
					bytecode_local_push_<&Generic::car>(
						stack,N);
				} NEXT_BYTECODE;
				BYTECODE(car_clos_push):
				{INTPARAM(N);CLOSUREREF;
					bytecode_clos_push_<&Generic::car>(
						stack,clos,N);
				} NEXT_BYTECODE;
				BYTECODE(cdr):
					bytecode_<&Generic::cdr>(stack);
				NEXT_BYTECODE;
				BYTECODE(cdr_local_push):
				{INTPARAM(N);
					bytecode_local_push_<&Generic::cdr>(
						stack,N);
				} NEXT_BYTECODE;
				BYTECODE(cdr_clos_push):
				{INTPARAM(N);CLOSUREREF;
					bytecode_clos_push_<&Generic::cdr>(
						stack,clos,N);
				} NEXT_BYTECODE;
				BYTECODE(check_vars):
				{INTPARAM(N);
					bytecode_check_vars(stack, N);
				} NEXT_BYTECODE;
				BYTECODE(closure):
				{INTSEQPARAM(N,S);
					Closure* nclos =
						NewClosure(proc,
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
				BYTECODE(closure_ref):
				{INTPARAM(N);CLOSUREREF;
					bytecode_closure_ref(stack, clos, N);
				} NEXT_BYTECODE;
				BYTECODE(composeo):
				{CLOSUREREF;
					/*destructure closure*/
					stack.push(clos[0]);
					stack[0] = clos[1];
					// clos is now no longer safe to use
					KClosure& kclos =
						*NewKClosure(proc,
		THE_EXECUTOR(composeo_continuation), 2);
					// clos is now invalid
					kclos[0] = stack[1];
					kclos[1] = stack.top(); stack.pop();
					stack[1] = &kclos;
				} /***/ NEXT_EXECUTOR; /***/
				BYTECODE(cons):
					bytecode_cons(proc,stack);
				NEXT_BYTECODE;
				/*implements the common case
				where we just return a value
				to our continuation.
				*/
				/*"continue" might conflict with C++ keyword*/
				BYTECODE(b_continue):
					stack.top(2) = stack[1];
					stack.restack(2);
				/***/ NEXT_EXECUTOR; /***/
				/*implements the case where we
				just return a local variable
				(probably computed by a
				primitive) to our continuation
				*/
				BYTECODE(continue_local):
				{INTPARAM(N);
					stack.push(stack[N]);
					stack.top(2) = stack[1];
					stack.restack(2);
				} /***/ NEXT_EXECUTOR; /***/
				/*handles the case where the
				continuation we want to return
				to is in our own closure.
				*/
				BYTECODE(continue_on_clos):
				{INTPARAM(N);CLOSUREREF;
					Generic* gp = stack.top();
					stack.top() = clos[N];
					stack.push(gp);
					stack.restack(2);
				} /***/ NEXT_EXECUTOR; /***/
				BYTECODE(global):
				{ATOMPARAM(S);
					bytecode_global(proc,stack,S);
				} NEXT_BYTECODE;
				BYTECODE(global_set):
				{ATOMPARAM(S);
					bytecode_global_set(proc,stack,S);
				} NEXT_BYTECODE;
				BYTECODE(halt):
					stack.restack(1);
					return process_dead;
				NEXT_BYTECODE;
				BYTECODE(halt_local_push):
				{INTPARAM(N);
					stack.push(stack[N]);
					stack.restack(1);
					return process_dead;
				} NEXT_BYTECODE;
				BYTECODE(halt_clos_push):
				{INTPARAM(N);CLOSUREREF;
					stack.push(clos[N]);
					stack.restack(1);
					return process_dead;
				} NEXT_BYTECODE;
				BYTECODE(b_if):
				{	Generic* gp = stack.top(); stack.pop();
					if(gp->istrue()){SEQPARAM(S);
						GOTO_SEQUENCE(S);
					}
				} NEXT_BYTECODE;
				/*we expect this to be much more
				common due to CPS conversion; in
				fact we don't expect a plain 'if
				bytecode at all...
				*/
				BYTECODE(if_local):
				{INTSEQPARAM(N,S);
					if(stack[N]->istrue()){
						GOTO_SEQUENCE(S);
					}
				} NEXT_BYTECODE;
				BYTECODE(b_int):
				{INTPARAM(N);
					bytecode_int(proc, stack, N);
				} NEXT_BYTECODE;
				BYTECODE(k_closure):
				{INTSEQPARAM(N,S);
					KClosure& nclos =
						*NewKClosure(proc,
							THE_ARC_EXECUTOR(
								arc_executor,
								S),
							N);
					for(int i = N; i ; --i){
						nclos[i - 1] = stack.top();
						stack.pop();
					}
					stack.push(&nclos);
				} NEXT_BYTECODE;
				/*attempts to reuse the current
				continuation closure.  Falls back
				to allocating a new KClosure if
				the closure isn't a continuation
				(for example due to serialization)
				or if it can't be reused (due to
				'ccc).
				*/
				BYTECODE(k_closure_reuse):
				{INTSEQPARAM(N,S);
					KClosure* nclos =
						dynamic_cast<KClosure*>(
							stack[0]);
					if(!nclos || !nclos->reusable()){
						CLOSUREREF;
						nclos =
						NewKClosure(proc,
							THE_ARC_EXECUTOR(
								arc_executor,
								S),
							//Use the size of the
							//current closure
							clos.size());
						//clos is now invalid
					} else {
						nclos->codereset(
							THE_ARC_EXECUTOR(
								arc_executor,
								S));
					}
					for(int i = N; i ; --i){
						(*nclos)[i - 1] = stack.top();
						stack.pop();
					}
					stack.push(nclos);
				} NEXT_BYTECODE;
				BYTECODE(lit_nil):
					bytecode_lit_nil(proc, stack);
				NEXT_BYTECODE;
				BYTECODE(lit_t):
					bytecode_lit_t(proc, stack);
				NEXT_BYTECODE;
				BYTECODE(local):
				{INTPARAM(N);
					bytecode_local(stack, N);
				} NEXT_BYTECODE;
				/*reducto is a bytecode to *efficiently*
				implement the common reduction functions,
				such as '+.  This bytecode avoids
				allocating new space when called with 2
				Arc arguments or less, and only allocates
				a reusable continuation closure that is
				used an array otherwise
				See also the executor reducto_continuation.
				*/
				BYTECODE(reducto):
				{CLOSUREREF;
					/*determine #params*/
					/*we have two hidden arguments,
					the current closure and the
					current continuation
					This shouldn't be callable without
					at least those two parameters,
					but better paranoid than sorry
					(theoretically possible in a buggy
					bytecode sequence, for example)
					*/
					if(stack.size() < 2){
						throw ArcError("apply",
							"Insufficient number "
							"of parameters to "
							"variadic function");
					}
					size_t params = stack.size() - 2;
					if(params < 3){
						/*simple and quick dispatch
						for common case
						*/
						stack[0] = clos[params];
						/*don't disturb the other
						parameters; the point is
						to be efficient for the
						common case
						*/
					} else {
						stack[0] = clos[2]; // f2
						size_t saved_params =
							params - 2;
						KClosure& kclos =
							*NewKClosure(proc,
			THE_EXECUTOR(reducto_continuation),
			saved_params + 3
							);
						// clos is now invalid
						kclos[0] = stack[0]; // f2
						kclos[1] = stack[1];
						/*** placeholder ***/
						kclos[2] = stack[1];
						for(size_t i = saved_params +
							3;
						    i > 3;
						    --i){
							kclos[i - 1] =
								stack.top();
							stack.pop();
						}
						/*save closure*/
						stack[1] = &kclos;
						Integer* Np =
							new(proc) Integer(3);
						// kclos is now invalid
						KClosure& kkclos =
							*static_cast
							<KClosure*>(stack[1]);
						kkclos[2] = Np;
					}
				} /***/ NEXT_EXECUTOR; /***/
				BYTECODE(rep):
					bytecode_<&Generic::rep>(stack);
				NEXT_BYTECODE;
				BYTECODE(rep_local_push):
				{INTPARAM(N);
					bytecode_local_push_<&Generic::rep>(
						stack,N);
				} NEXT_BYTECODE;
				BYTECODE(rep_clos_push):
				{INTPARAM(N);CLOSUREREF;
					bytecode_clos_push_<&Generic::rep>(
						stack,clos,N);
				} NEXT_BYTECODE;
				BYTECODE(tag):
					bytecode_tag(proc,stack);
				NEXT_BYTECODE;
				BYTECODE(sv):
					bytecode_<&Generic::make_sv>(
						proc, stack);
				NEXT_BYTECODE;
				BYTECODE(sv_local_push):
				{INTPARAM(N);
					bytecode_local_push_
						<&Generic::make_sv>(
						proc, stack, N);
				} NEXT_BYTECODE;
				BYTECODE(sv_clos_push):
				{INTPARAM(N);CLOSUREREF;
					bytecode_clos_push_
						<&Generic::make_sv>(
						proc, stack, clos, N);
				} NEXT_BYTECODE;
				BYTECODE(sv_ref):
					bytecode_<&Generic::sv_ref>(
						stack);
				NEXT_BYTECODE;
				BYTECODE(sv_ref_local_push):
				{INTPARAM(N);
					bytecode_local_push_
						<&Generic::sv_ref>(
						stack, N);
				} NEXT_BYTECODE;
				BYTECODE(sv_ref_clos_push):
				{INTPARAM(N);CLOSUREREF;
					bytecode_clos_push_
						<&Generic::sv_ref>(
						stack, clos, N);
				} NEXT_BYTECODE;
				BYTECODE(sv_set):
					bytecode_sv_set(stack);
				BYTECODE(sym):
				{ATOMPARAM(S);
					bytecode_sym(proc, stack, S);
				} NEXT_BYTECODE;
				BYTECODE(type):
					bytecode_<&Generic::type>(
						proc, stack);
				NEXT_BYTECODE;
				BYTECODE(type_local_push):
				{INTPARAM(N);
					bytecode_local_push_
						<&Generic::type>(
						proc, stack, N);
				} NEXT_BYTECODE;
				BYTECODE(type_clos_push):
				{INTPARAM(N);CLOSUREREF;
					bytecode_clos_push_
						<&Generic::type>(
						proc, stack, clos, N);
				} NEXT_BYTECODE;
				BYTECODE(variadic):
				{INTPARAM(N);
					bytecode_variadic(proc, stack, N);
				} NEXT_BYTECODE;
			}
		} NEXT_EXECUTOR;
		/*
		(fn (k#|1|# f#|2|#)
		  (f k (fn (_ r) (k r))))
		*/
		EXECUTOR(ccc):
		{	Closure* c =
				NewClosure(proc, THE_EXECUTOR(ccc_helper), 1);
			KClosure* k = dynamic_cast<KClosure*>(stack[1]);
			if(k){
				k->banreuse();
			}
			(*c)[0] = stack[1/*k*/];
			stack[0] = stack[2/*f*/];
			stack[2] = c;
		} NEXT_EXECUTOR;
		EXECUTOR(ccc_helper):
		{	Closure* c = static_cast<Closure*>(stack[0]);
			stack[0] = (*c)[0/*k*/];
			stack[1] = stack[2/*r*/];
			stack.pop();
		} NEXT_EXECUTOR;
		/*(fn (self k l)
		    (compile_helper k (%empty-bytecode-sequence) l))
		*/
		EXECUTOR(compile):
		{	Closure* c =
				NewClosure(
					proc, THE_EXECUTOR(compile_helper), 0);
			stack[0] = c;
			ArcBytecodeSequence* bseq = new(proc)
				ArcBytecodeSequence();
			// c is no invalid
			/*just do type checking*/
			if(stack[2]->istrue()){
				Cons* cp = expect_type<Cons>(stack[2],
						"compile",
						"Expected bytecode list");
			}
			stack.push(stack[2]);
			stack[2] = bseq;
		} NEXT_EXECUTOR;
		/*NOTE: Although this violates the letter of the
		coding conventions (don't exceed 80 cols/line),
		it's OK: it's just a transcription of Arc code
		*/
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
							Closure* clos = NewClosure(proc, THE_EXECUTOR(compile_intseq_bytecode), 6);
							// b is now invalid
							for(int i = 0; i < 6; ++i){
								(*clos)[i] = stack[i];
							}
							/*call new continuation*/
							stack[3] = stack[0];
							stack[4] = clos; // have to save this first!
							stack[5] = b = new(proc) ArcBytecodeSequence(); // make sure to reassign b
							//allocated values are now invalid!
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
						Closure* clos = NewClosure(proc, THE_EXECUTOR(compile_seq_bytecode), 5);
						// b is no invalid
						for(int i = 0; i < 5; ++i){
							(*clos)[i] = stack[i];
						}
						stack[1] = clos; // Have to save this first!
						stack[2] = b = new(proc) ArcBytecodeSequence(); // make sure to reassign b
						//variables not in stack are now invalid
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
		{CLOSUREREF;
			ArcBytecodeSequence* subseq =
				static_cast<ArcBytecodeSequence*>(stack[1]);
			/*(let ...)*/
			ArcBytecodeSequence* b =
				static_cast<ArcBytecodeSequence*>(
					clos[2]);
			/*the type of c wasn't checked in the first place*/
			Sym* c = expect_type<Sym>(clos[4],
				"compile",
				"Expected bytecode symbol "
				"in int-seq bytecode form");
			Integer* param =
				static_cast<Integer*>(
					clos[5]);
			b->append(new IntSeqBytecode(bytecodelookup(c->a), param->integer(), subseq->seq));
			/*push the closure elements*/
			for(int i = 0; i < 4; ++i){
				stack.push(clos[i]);
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
		{CLOSUREREF;
			ArcBytecodeSequence* subseq =
				dynamic_cast<ArcBytecodeSequence*>(stack[1]);
			/*(let ...)*/
			ArcBytecodeSequence* b =
				dynamic_cast<ArcBytecodeSequence*>(
					clos[2]);
			/*the type of c wasn't checked in the first place*/
			Sym* c = expect_type<Sym>(clos[4],
				"compile",
				"Expected bytecode symbol "
				"in seq bytecode form");
			b->append(new SeqBytecode(bytecodelookup(c->a), subseq->seq));
			/*push the closure elements*/
			for(int i = 0; i < 4; ++i){
				stack.push(clos[i]);
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
			boost::shared_ptr<BytecodeSequence> pb =
				b->seq;
			Closure* nclos =
				// might invalidate b
				NewClosure(proc,
					THE_ARC_EXECUTOR(arc_executor, pb),
					0);
			// b is now invalid
			stack[2] = nclos;
			stack.restack(2);
		} NEXT_EXECUTOR;
		EXECUTOR(halting_continuation):
			stack.push(stack[1]);
			stack.restack(1);
			return process_dead;
		NEXT_EXECUTOR;
		/*this implements the 3-argument or more case
		of the reducto bytecode.  reducto is used to
		efficiently implement a function that
		performs reduction, and avoids allocating
		cons cells for the arguments.
		*/
		EXECUTOR(reducto_continuation):
		{CLOSUREREF;
			Integer* Np =
				static_cast<Integer*>(clos[2]);
			int N = Np->val;
			int NN = N + 1; // next N
			stack.push(clos[0]);
			if(NN == clos.size()){
				// final iteration
				stack.push(clos[1]);
				stack.push(stack[1]);
				stack.push(clos[N]);
				stack.restack(4);
			} else {
				/*dynamic_cast because it is possible for
				the program to reconstruct a continuation
				using an ordinary, non-continuation
				Closure
				*/
				KClosure* pkclos =
					dynamic_cast<KClosure*>(&clos);
				if(pkclos && pkclos->reusable()){
					// a reusable continuation
					Np->val++; // Np is also reusable
					stack.push(pkclos);
					stack.push(stack[1]);
					stack.push(clos[N]);
					stack.restack(4);
				} else {
					/*TODO: instead create a closure
					with only a reference to this
					closure and an index number, to
					reduce memory allocation.
					*/
					KClosure& nclos =
						*NewKClosure(proc,
			THE_EXECUTOR(reducto_continuation),
			// save only necessary
			clos.size() - NN + 3
						);
					// Np, pkclos and clos are now invalid
					{CLOSUREREF; //revalidate clos
						nclos[0] = clos[0];
						nclos[1] = clos[1];
						/*** placeholder! ***/
						nclos[2] = clos[1];
						for(int j = 0;
						    j < clos.size() - NN;
						    ++j){
							nclos[3 + j] =
								clos[NN + j];
						}
						stack.push(&nclos);
						stack.push(stack[1]);
						stack.push(clos[N]);
						stack.restack(4);
					} // clos is now invalid again
					Integer* Np = new(proc) Integer(3);
					// nclos is now invalid
					KClosure& kkclos =
						*static_cast<KClosure*>(
							stack[1]);
					kkclos[2] = Np;
				}
			}
		} NEXT_EXECUTOR;
		/*
		this executor implements the return from the first
		(inner) function of a composed pair and calls the
		second function in the pair.
		*/
		EXECUTOR(composeo_continuation):
		{CLOSUREREF;
			stack.push(clos[1]);
			stack.push(clos[0]);
			stack.push(stack[1]);
			stack.restack(3);
		} NEXT_EXECUTOR;
		/*
		(fn (self k f) ; f is a (fn (self k))
		  ...)
		*/
		EXECUTOR(spawn):
		{	Closure* hclos = NewClosure(proc,
					THE_EXECUTOR(spawn_helper),
					1);
			(*hclos)[0] = stack[2];
			Generic* gp = hclos;
			/*create a copy*/
			boost::shared_ptr<Semispace> ns =
				proc.to_new_semispace(gp);
			//gp now points within the new Semispace
			boost::shared_ptr<ProcessHandle> nh =
				NewArcProcess(ns,gp);
			Pid* npid = new(proc) Pid(nh);
			// hclos is now invalid
			stack[2] = npid;
			runsystem->schedule(nh);
			stack.restack(2);
		} NEXT_EXECUTOR;
		/*TODO: EXECUTOR(spawn)
		(with (stored_f <...>)
		  (fn (self)
		    (stored_f halting-continuation)))
		this is the only executor that really doesn't have
		a continuation argument.  it is intended to be the
		executor for a newly-spawned process.
		*/
		EXECUTOR(spawn_helper):
		{CLOSUREREF;
			stack[0] = clos[0];
			Closure* nclos =
				NewClosure(proc,
					THE_EXECUTOR(halting_continuation),
					0);
			//clos is now invalid
			stack.push(nclos);
		} NEXT_EXECUTOR;
		/*
		(fn (self k pid o)
		  (send_actual k pid (%to-new-semispace o)))
		*/
		EXECUTOR(send):
		{	Pid* pp = expect_type<Pid>(stack[2],
					"apply",
					"==> expects an object of type 'pid");
			Generic* gp = stack[3];
			boost::shared_ptr<Semispace> ns =
				proc.to_new_semispace(gp);
			// gp is now within ns
			stack[3] = new(proc) SemispacePackage(ns,gp);
			stack[0] = NewClosure(proc,
					THE_EXECUTOR(send_actual),
					0);
		} /*** FALL THROUGH! ***/
		EXECUTOR(send_actual):
		{	Pid* pp = static_cast<Pid*>(stack[2]);
			SemispacePackage* sp =
				static_cast<SemispacePackage*>(stack[3]);
			bool rv = pp->hproc->pproc->receive(sp->ns, sp->gp);
			if(rv){
				stack.push(stack[1]);
				stack.push(proc.nilobj());
			//process not ready, retry
			} else return process_running;
		} NEXT_EXECUTOR;
		/*
		(fn (self k p) (p!probe 0) (k p))
		*/
		EXECUTOR(probe):
			stack[2]->probe(0);
			stack.restack(2);
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
			std::map<Atom*,
				boost::shared_ptr<Executor> >::iterator i =
				biftb.find(&s->atom());
			if(i != biftb.end()){
				stack.top() =
					NewClosure(proc, i->second, 0);
				// s is now invalid
			} else {
				stack.top() = proc.nilobj();
			}
			stack.restack(2);
		} NEXT_EXECUTOR;
	}
	// should be unreachable
	return process_dead;

/*So why isn't this, say, a separate function?  Well,
because I originally had this impression that labels
were inaccessible outside of the function where they
were defined, and now I'm too lazy to move it out of
here
*/
initialize:
	/*atoms*/
	QUOTEATOM = globals->lookup("quote");

	InitialAssignments()
	/*built-in functions accessible via $*/
		("ccc",			THE_EXECUTOR(ccc))
		("bytecoder",		THE_EXECUTOR(compile))
		("bytecode-to-free-fun",THE_EXECUTOR(to_free_fun))
		("halt",		THE_EXECUTOR(halting_continuation))
		("spawn",		THE_EXECUTOR(spawn))
		("probe",		THE_EXECUTOR(probe))
	/*bytecodes*/
		("apply",		THE_BYTECODE_LABEL(apply))
		("apply-invert-k",	THE_BYTECODE_LABEL(apply_invert_k))
		("apply-list",		THE_BYTECODE_LABEL(apply_list))
		("car",			THE_BYTECODE_LABEL(car))
		("car-local-push",	THE_BYTECODE_LABEL(car_local_push))
		("car-clos-push",	THE_BYTECODE_LABEL(car_clos_push))
		("cdr",			THE_BYTECODE_LABEL(cdr))
		("cdr-local-push",	THE_BYTECODE_LABEL(cdr_local_push))
		("cdr-clos-push",	THE_BYTECODE_LABEL(cdr_clos_push))
		("check-vars",		THE_BYTECODE_LABEL(check_vars))
		("closure",		THE_BYTECODE_LABEL(closure))
		("closure-ref",		THE_BYTECODE_LABEL(closure_ref))
		("composeo",		THE_BYTECODE_LABEL(composeo))
		("cons",		THE_BYTECODE_LABEL(cons))
		("continue",		THE_BYTECODE_LABEL(b_continue))
		("continue-local",	THE_BYTECODE_LABEL(continue_local))
		("continue-on-clos",	THE_BYTECODE_LABEL(continue_on_clos))
		("global",		THE_BYTECODE_LABEL(global))
		("global-set",		THE_BYTECODE_LABEL(global_set))
		("halt",		THE_BYTECODE_LABEL(halt))
		("halt-local-push",	THE_BYTECODE_LABEL(halt_local_push))
		("halt-clos-push",	THE_BYTECODE_LABEL(halt_clos_push))
		("if",			THE_BYTECODE_LABEL(b_if))
		("if-local",		THE_BYTECODE_LABEL(if_local))
		("int",			THE_BYTECODE_LABEL(b_int))
		("k-closure",		THE_BYTECODE_LABEL(k_closure))
		("k-closure-reuse",	THE_BYTECODE_LABEL(k_closure_reuse))
		("lit-nil",		THE_BYTECODE_LABEL(lit_nil))
		("lit-t",		THE_BYTECODE_LABEL(lit_t))
		("local",		THE_BYTECODE_LABEL(local))
		("reducto",		THE_BYTECODE_LABEL(reducto))
		("rep",			THE_BYTECODE_LABEL(rep))
		("rep-local-push",	THE_BYTECODE_LABEL(rep_local_push))
		("rep-clos-push",	THE_BYTECODE_LABEL(rep_clos_push))
		("sv",			THE_BYTECODE_LABEL(sv))
		("sv-local-push",	THE_BYTECODE_LABEL(sv_local_push))
		("sv-clos-push",	THE_BYTECODE_LABEL(sv_clos_push))
		("sv-ref",		THE_BYTECODE_LABEL(sv_ref))
		("sv-ref-local-push",	THE_BYTECODE_LABEL(sv_ref_local_push))
		("sv-ref-clos-push",	THE_BYTECODE_LABEL(sv_ref_clos_push))
		("sym",			THE_BYTECODE_LABEL(sym))
		("tag",			THE_BYTECODE_LABEL(tag))
		("type",		THE_BYTECODE_LABEL(type))
		("type-local-push",	THE_BYTECODE_LABEL(type_local_push))
		("type-clos-push",	THE_BYTECODE_LABEL(type_clos_push))
		("variadic",		THE_BYTECODE_LABEL(variadic))

	/*assign bultin global*/
		("$",			proc, THE_EXECUTOR(bif_dispatch))
	/*these globals are used only during initialization.  After
	initialization these globals may be overwritten by user
	program.
	*/
		("<snap>compile</snap>",proc, THE_EXECUTOR(compile))
		("<snap>halt</snap>",	proc,
			THE_EXECUTOR(halting_continuation))
		("<snap>to-free-fun</snap>",
					proc, THE_EXECUTOR(to_free_fun))
	;/*end initializer*/
	return process_running;
}

