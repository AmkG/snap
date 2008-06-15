/**/
#include"heaps.hpp"
#include"processes.hpp"
#include"variables.hpp"
#include"atoms.hpp"
#include<boost/shared_ptr.hpp>

/*-----------------------------------------------------------------------------
Process
-----------------------------------------------------------------------------*/

void Process::sendto(Process& p, Generic* g) const {
	Generic* ng = transfer(g, p);
	/*!!!!WARNING!!!!
	Possibly unsafe: ng might become invalidated
	by another worker thread if the destination
	process performs a GC between the transfer and
	the new locking
	*/
	/*add to mailbox*/
	{/*insert locking of destination's other_spaces here*/
		p.mailbox.push_back(ng);
	}
	/*TODO: if destination process is waiting for a message,
	schedule it
	*/
}
/*
Scheduling:
*/

/*assigns the generic to the global variable*/
void Process::assign(boost::shared_ptr<Atom> a, Generic* g) const {
	Generic *ng = transfer(g, *globals);
	{/*insert acquisition of global lock*/
		globals->assign(a, ng);
	}
	globals->GC();
	/*TODO: notify all other processes that the global
	has been modified
	*/
}

