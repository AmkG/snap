; quote-lift.arc
; by AmkG
; converts the form:
;   (foo 'bar)
; to:
;   (let quoted 'bar (foo quoted))

(def quote-lift (ast)
  (givens collect
          (fn (ast)
            (let tb (table)
              (trav+ (go ast) ast
                (when (aquote ast)
                  (= tb.ast (new-var 'quoted)))
                (each s ast!subx (go s)))
              tb))
          ctb (collect ast)
          (params arguments)
            (let (params arguments) nil
              (ontable argument param ctb
                (push argument arguments)
                (push param params))
              (list params arguments))
          list-expand
          (fn (ast)
            (if
              (isa ast!val 'cons)
                ((afn (val)
                   (if
                     (isa val 'cons)
                       (make-prim (list (self:car val) (self:cdr val)) '%cons)
                     (isa val 'sym)
                       (make-quote () val)
                       (make-lit () val)))
                 ast!val)
              (isa ast!val 'sym)
                ast
              ; else
                (make-lit () ast!val)))
          arguments (map list-expand arguments)
          transform
          (fn (ast)
            (make-app
              (cons (make-lam
                      (list
                        ((afn (ast)
                           (aif ctb.ast
                                (make-ref () it)
                                (do (zap [map self _] ast!subx)
                                    ast)))
                         ast))
                      params)
                    arguments)))
    (transform ast)))


