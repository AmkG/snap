#ifndef VARIABLES_H
#define VARIABLES_H
#include<boost/scoped_ptr.hpp>
#include<boost/shared_ptr.hpp>

#ifndef NO_EXTERN
#	ifndef EXTERN
#		define	EXTERN extern
#	endif //EXTERN
#endif //NO_EXTERN not defined

#ifdef NO_EXTERN
#	ifndef EXTERN
#		define	EXTERN
#	endif //EXTERN
#endif //NO_EXTERN defined

class Atom;
class Globals;
class Runsystem;
class ProcessHandle;

/*These are constant after initialization*/
EXTERN boost::shared_ptr<Atom> NILATOM;		//'nil
EXTERN boost::shared_ptr<Atom> TATOM;		//'t
EXTERN boost::shared_ptr<Atom> CONSATOM;	//'cons
EXTERN boost::shared_ptr<Atom> SYMATOM;		//'sym
EXTERN boost::shared_ptr<Atom> FNATOM;		//'fn
EXTERN boost::shared_ptr<Atom> INTATOM;		//'int
EXTERN boost::shared_ptr<Atom> PIDATOM;		//'pid
EXTERN boost::shared_ptr<Atom> BINATOM;		//'binary
EXTERN boost::shared_ptr<Atom> INTERNALATOM;	//'internally-used
EXTERN boost::shared_ptr<Atom> WRITEATOM;	//'write
EXTERN boost::shared_ptr<Atom> READ1ATOM;	//'read1
EXTERN boost::shared_ptr<Atom> ERRATOM;		//'err
EXTERN boost::shared_ptr<Atom> EOLATOM;		//'eol

/*Should really be Singleton but let's not bother with that*/
EXTERN boost::scoped_ptr<Globals> globals;
EXTERN boost::scoped_ptr<Runsystem> runsystem;
EXTERN boost::scoped_ptr<ProcessHandle> centralio;

void variables_init(void);

#endif //VARIABLES_H

