; to-cpp.arc
; by AmkG
; Converts a list to C++ code that uses
; the SNAP VM environment to reconstruct
; that list

(def to-cpp (ob (o fnam "construct_test") (o s (stdout)))
  (w/stdout s
    (prn "#include\"variables.hpp\"")
    (prn "#include\"processes.hpp\"")
    (prn "#include\"types.hpp\"")
    (prn "#include\"atoms.hpp\"")
    (prn "#include\"bytecodes.hpp\"\n")
    (prn "void " fnam "(Process& proc){")
    (prn "\tProcessStack& stack = proc.stack;")
    ((afn (ob)
       (if
         (acons ob)
           (do (self:car ob)
               (self:cdr ob)
               (prn "\tbytecode_cons(proc,stack);"))
         ; special case nil
         (no ob)
           (prn "\tbytecode_sym(proc,stack,NILATOM);")
         (isa ob 'sym)
           (prn "\tbytecode_sym(proc,stack,\n\t\tglobals->lookup("
		(tostring:write:string ob)
		"));")
         (isa ob 'int)
           (prn "\tbytecode_int(proc,stack," ob ");")))
     ob)
    (prn "}")))

