
(require "lib/proto-table.arc")

(def bytecodegen (ast (o env (table 'top 0)) (o cont-clos))
  ; (ppr-sexp:source ast)
  (withs (is-closure-ref-self
          (fn (ast)
            (and (aprim ast) (is ast!op '%closure-ref)
                 (let (s-var i) ast!subx
                   (and (aref s-var) (is 0 (env s-var!var))))))
          is-closure-ref-0
          (fn (ast)
            (and (aprim ast)
                 (is ast!op '%closure-ref)
                 (is ((cadr ast!subx) 'val) 0)))
          is-cont-type
          (fn (asts)
            (if (is (len asts) 2)
                (let (f r) asts
                  (and (aref f) (is 1 (env f!var))))))
          is-cont-on-closure-type
          (fn (asts)
            (if (is (len asts) 2)
                (let (f r) asts
                  (is-closure-ref-self f))))
          ; ensure that a tail call apply really is just a tail-call apply
          ; (i.e. does not nest any if's or lets)
          apply-pushall
          (fn (params)
            (mappend
              (fn (p)
                (if ((orf aprim aref aset aquote alit) p)
                    (bytecodegen p env)
                    (do (prn "----unexpected in tail-call apply")
                        (ppr-sexp:source p)
                        nil)))
              params))
          ; generates an environment for a function
          f-environment
          (fn (params)
            (let new-env (table)
              (on p params (= new-env.p index))
              (= new-env!top (len params))
              new-env))
          ; uses the specified bytecode's -local-push
          ; version if possible
          w/local-push
          (fn (param op)
            ; check if it's a local variable reference
            (aif
              (and (aref param) (~aglobal param!var) (env param!var))
                 `( (,(sym:string op '-local-push) ,it))
              (is-closure-ref-self param)
                 ; closure values in arc2c are numbered from 1->N, but
                 ; in snap are numbered 0->N-1
                 `( (,(sym:string op '-clos-push)
                     ,(- ((cadr param!subx) 'val) 1)))
              ; else
                 `( ,@(bytecodegen param env)
                    ,op)))
          direct-compile
          (fn (params op)
            `( ,@(mappend [bytecodegen _ env] params)
               ,op))
          is-continuation
          (fn (ast)
            (if (and (aprim ast) (is '%closure ast!op))
               (let f (car ast!subx)
                  (and (alam f) f!continuation)))))
    (if
      (anapp ast)
        ; check if applied-to parameter is a function
        (let (f . params) ast!subx
          (if (alam f)
              ; let form
              ; associate parameters with variables
              ; Note! does not handle cases:
              ; ((fn (a) ...) 1 2 3)
              ; (shouldn't happen anyway, probably flag
              ; this as an error before reaching this stage)
              (withs (assoclist
                      ((afn (vars subxs)
                         (if
                           (acons vars)
                             (cons
                               (cons (car vars) (car subxs))
                               (self (cdr vars) (cdr subxs)))
                           ; rest parameter
                           ; (shouldn't normally happen...)
                           vars
                             (cons
                               (cons vars
                                 ((afn (subxs)
                                    (if subxs
                                        (make-prim
                                          `(,(car subxs) ,(self:cdr subxs))
                                          '%cons)
                                        (make-lit () nil)))
                                  subxs))
                               nil)))
                       f!params params)
                       ; create a new environment
                       ; (don't overwrite old one - we might
                       ; be on one branch of an 'if)
                       new-env (proto-table env)
                       ; push each parameter on the stack
                       pushlist
                       (mappend
                         (fn ((var . ast))
                           ; just rename it if reference to an existing
                           ; local variable
                           (aif (and
                                  (aref ast)
                                  (~aglobal ast!var)
                                  (env ast!var))
                                ; existing local
                                (do
                                  (= (new-env var) it)
                                  ; emit no code
                                  ())
                                ; emit code for it
                                (do
                                  (= (new-env var) new-env!top)
                                  (++ new-env!top)
                                  (bytecodegen ast env))))
                         assoclist))
                `(,@pushlist
                  ,@(bytecodegen (car f!subx) new-env cont-clos)))
              ; ordinary apply
              ; ignore f: the base system will handle this for us
              (if (is-closure-ref-0 f)
                  (if
                    ; check if continuation-type where continuation is a
                    ; closure-ref (first param is ref to self[N], and two
                    ; params only)
                    (is-cont-on-closure-type params)
                      (let (k rv) params
                        `( ,@(bytecodegen rv env)
                           ; arc2c counts the function code as closure.0
                           ; snap does not, so convert closure.N to
                           ; (closure (- N 1))
                           (continue-on-clos ,(- ((cadr k!subx) 'val) 1))))
                    ; check if current function is a continuation and
                    ; if we need to create a continuation for this
                    ; call
                    (and cont-clos (is-continuation:cadr params))
                      (withs ((f k . params) params
                              ; k-fn = the lambda for the continuation
                              ; k-closed = the closed variables of the
                              ;   continuation
                              (k-fn . k-closed) k!subx)
                        `( ,@(bytecodegen f env)
                           (local 0) ; placeholder for k
                           ,@(apply-pushall params)
                           ,@(mappend [bytecodegen _ env] k-closed)
                           (,(if (>= cont-clos
                                     (- (len k!subx) 1))
                               'k-closure-reuse
                               'k-closure-recreate)
                             ,(len k-closed)
                             ,@(bytecodegen
                                 (car k-fn!subx)
                                 (f-environment k-fn!params)
                                 (max cont-clos (len k-closed))))
                           (apply-invert-k ,(+ 2 (len params)))))
                    ; check if continuation anyway - continuations
                    ; that don't reuse or recreate their closures
                    ; should use apply-k-release
                    cont-clos
                      `( ,@(apply-pushall params)
                         (apply-k-release ,(len params)))
                    ; check if return-type call (first param is ref
                    ; to local[1], and two params (continuation and
                    ; return-value) only)
                    (is-cont-type params)
                      ; now check if return value is a variable reference
                      (let (k rv) params
                        (if (and (aref rv) (~aglobal rv!var))
                            `( (continue-local ,(env rv!var)))
                            `( ,@(bytecodegen rv env)
                               continue)))
                    ; ordinary apply
                       ; NOTE! assumes that subexpressions here cannot
                       ; be 'let forms
                      `( ,@(apply-pushall params)
                         (apply ,(len params))))
                  (do (prn "----expected (%closure-ref _ 0) on apply")
                      (ppr-sexp:source ast)
                      nil))))
      (aprim ast)
        (given op      ast!op
               params  ast!subx
          (case op
            %closure-ref
              ; make sure we are referring to the
              ; self variable
              (let (s-var paramnum) params
                (if (and
                      (aref s-var)
                      (~aglobal s-var!var)
                      (is 0 (env s-var!var)))
                    ; closure numberings in arc2c count the function code
                    ; as closure.0
                    ; snap does not, so convert closure.N to (closure (- N 1))
                    `((closure-ref ,(- paramnum!val 1)))
                    (do (prn "----expected (%closure-ref self _)")
                        (ppr-sexp:source ast)
                        nil)))
            %closure
              ; make sure first parameter is a
              ; function
              (let (f . closed) params
                (if
                  (~alam f)
                    ; not a function
                    (do (prn "---- expected (%closure (fn ...) ...)")
                        (ppr-sexp:source ast)
                        nil)
                    ; create the closure
                    `( ,@(mappend [bytecodegen _ env] closed)
                       (,(if f!continuation 'k-closure 'closure)
                                ,(len closed)
                                ; add check for correct arguments here
                                ,(if (~dotted f!params)
                                     `(check-vars ,(len f!params))
                                     ; the variadic bytecode counts only
                                     ; the proper arguments
                                     `(variadic
                                        ,(- (len:properify f!params) 1)))
                                ,@(bytecodegen
                                    (car f!subx)
                                    (f-environment:properify f!params)
                                    (if f!continuation (len closed)))))))
            %car
              (w/local-push (car ast!subx) 'car)
            %cdr
              (w/local-push (car ast!subx) 'cdr)
            %cons
              (direct-compile ast!subx 'cons)
            %halt
              (w/local-push (car ast!subx) 'halt)
            %type
              (w/local-push (car ast!subx) 'type)
            %rep
              (w/local-push (car ast!subx) 'rep)
            %annotate
              (direct-compile ast!subx 'tag)
            %apply
              (direct-compile ast!subx 'apply-list)
            %sharedvar
              (w/local-push (car ast!subx) 'sv)
            %sharedvar-read
              (w/local-push (car ast!subx) 'sv-ref)
            %sharedvar-write
              (direct-compile ast!subx 'sv-set)
            %symeval
              (direct-compile ast!subx 'symeval)
            ; default primitive
              (do (prn "----unsupported primitive")
                  (ppr-sexp:source ast)
                  nil)))
      (acnd ast)
        (let (c then else) ast!subx
          ; check for common case of local variable reference
          (if (and (aref c) (~aglobal c!var))
              `( (if-local ,(env c!var) ,@(bytecodegen then env))
                 ,@(bytecodegen else env))
              ; bytecode 'if pops stack top, so no need to update environment
              `( ,@(bytecodegen c env)
                 (if ,@(bytecodegen then env))
                 ,@(bytecodegen else env))))
      (alit ast)
        (let val ast!val
          (if
            (isa val 'int)
              `( (int ,val))
            (is val t)
              `( lit-t )
            (no val)
              `( lit-nil )
            ; else
              (do (prn "----unsupported literal type")
                  (ppr-sexp:source ast)
                  nil)))
      (aquote ast)
        (let val ast!val
          (if (isa val 'sym)
              `( (sym ',val))
              (do (prn "----unexpected type in quote form")
                  (ppr-sexp:source ast)
                  nil)))
      (aset ast)
        (given var   ast!var
               (val) ast!subx
          (if (aglobal var)
              `( ,@(bytecodegen val env)
                 (global-set ',var!id))
              (do (prn "----unexpected set of non-global")
                  (ppr-sexp:source ast)
                  nil)))
      (aref ast)
        (let var ast!var
          (if (aglobal var)
              `( (global ',var!id))
              `( (local ,(env var)))))
      ; shouldn't occur after closure conversion
      (alam ast)
        (do (prn "----unexpected naked function")
            (ppr-sexp:source ast)
            nil)
      ; shouldn't occur after cps conversion
      (aseq ast)
        (do (prn "----unexpected sequence")
            (ppr-sexp:source ast)
            nil)
      ; else
        (do (prn "----unsupported ast")
            (ppr-sexp:source ast)
            nil))))

(def bytecodegen-top (ast)
  (if
    ; what we expect
    (alam ast)
      `(
        ,(if (~dotted ast!params)
             `(check-vars ,(len ast!params))
             `(variadic ,(- (len:properify ast!params) 1)))
        ,@(bytecodegen (car ast!subx)
                       (with (rv (table)
                              params (properify ast!params))
                         (on e params (= rv.e index))
                         (= rv!top (len params))
                         rv)))
    ; else
      (do (prn "----expected naked function")
          (ppr-sexp:source ast)
          (err "Expected naked function"))))

(def source (ast)
  (if
    (alit ast)
       ast!val
    (aref ast)
      ((ast 'var) 'uid)
    (aset ast)
      (list 'set ((ast 'var) 'uid) (source (car ast!subx)))
    (acnd ast)
      (cons 'if (map source ast!subx))
    (aprim ast)
      (cons ast!op (map source ast!subx))
    (anapp ast)
      (if (alam (car ast!subx))
        ; actually, shouldn't properify the params, maybe okay since this is debug code, but...
        (list 'let (map (fn (p a) (list p!uid (source a))) (properify ((car ast!subx) 'params)) (cdr ast!subx)) (source (car ((car ast!subx) 'subx))))
        (map source ast!subx))
    (alam ast)
      `(fn ,(map-improper [_ 'uid] ast!params)
           ,@(if ast!continuation '((t arc2b continuation)))
           ,(source (car ast!subx)))
    (aseq ast)
      (cons 'do (map source ast!subx))
    (aquote ast)
      (list 'quote ast!val)
      ; if unknown AST, then probably still in list form
      ; (cref driver)
      ast))

; abstract away avoiding ' and the quoted parts of `
; body *must* return the value in 'var if it won't
; change anything
(mac code-walk (var code . body)
  (w/uniq usercode
    `(let ,usercode (fn (,var) ,@body)
       (*code-walk-internal ,usercode ,code))))

(def *code-walk-internal (usercode code)
  (let pass-to-user
       (fn (c)
         (let new-code (usercode c)
           (if (isnt new-code c)
               ; if change, rewalk the new code
               ; (don't move on unless code is stable)
               (*code-walk-internal usercode new-code)
               (if (acons new-code)
                   (map-improper [*code-walk-internal usercode _] new-code)
                   new-code))))
    (if (acons code)
        (if
          (is (car code) 'quote)
            code
          (is (car code) 'quasiquote)
            (list 'quasiquote
               ((rfn quasiwalk (code)
                   (if (acons code)
                       (if
                         (is (car code) 'unquote)
                           (list 'unquote (*code-walk-internal usercode
                                                               (cadr code)))
                         (is (car code) 'unquote-splicing)
                           (list 'unquote (*code-walk-internal usercode
                                                               (cadr code)))
                         ; else
                           (map-improper quasiwalk code))
                       code))
                 (cadr code)))
          ; else
            (pass-to-user code))
        (pass-to-user code))))

(def to-3-if (l)
  (code-walk code l
    (if
      (and (caris code 'if) (len> code 4))
        (let (_ c v . rest) code
          `(if ,c ,v (if ,@rest)))
      ; else
        code)))

(def symbol-syntax (l)
  (code-walk code l
    (remove-ssyntaxes code)))

; When self-compiling, probably need to define
; ssyntax and ssexpand lib functions
(def remove-ssyntaxes (code)
  (if
    (and (acons code) (ssyntax:car code))
      (let (fun . args) code
        (zap ssexpand fun)
        (if (caris fun 'compose)
            ((afn ((fun . rest))
               (if rest
                   `(,fun ,(self rest))
                   `(,fun ,@args)))
             (cdr fun))
            `(,fun ,@args)))
    (ssyntax code)
      (ssexpand code)
    code))

; compiles an expression to bytecode
(def compile-to-bytecode (e)
  (givens xe [xe _ ()]
          top-k-var (new-var 'k)
          cps-convert [cps-convert _ top-k-var]
          closure-convert [closure-convert _ top-k-var]
    ; this is stupid.... I like it!
    (bytecodegen-top:closure-convert:cps-convert:quote-lift:sharedvars-convert-assert:xe:to-3-if:symbol-syntax e)))

