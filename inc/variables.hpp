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

/*These are constant after initialization*/
EXTERN boost::shared_ptr<Atom> NILATOM;		//'nil
EXTERN boost::shared_ptr<Atom> TATOM;		//'t
EXTERN boost::shared_ptr<Atom> CONSATOM;	//'cons
EXTERN boost::shared_ptr<Atom> SYMATOM;		//'sym
EXTERN boost::shared_ptr<Atom> FNATOM;		//'fn
EXTERN boost::shared_ptr<Atom> INTATOM;		//'int
EXTERN boost::shared_ptr<Atom> INTERNALATOM;	//'internally-used

/*Should really be Singleton but let's not bother with that*/
EXTERN boost::scoped_ptr<Globals> globals;

void variables_init(void);

#endif //VARIABLES_H

