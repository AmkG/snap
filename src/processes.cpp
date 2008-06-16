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
	boost::shared_ptr<Semispace> ns = to_new_semispace(g);
	/*g is modified by to_new_semispace, and now points
	to the copy in the new Semispace
	*/
	/*add to mailbox*/
	{/*insert locking of destination's other_spaces here*/
		p.other_spaces.push_back(ns);
		p.mailbox.push_back(g);
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
	boost::shared_ptr<Semispace> ns = to_new_semispace(g);
	{/*insert acquisition of global lock*/
		globals->assign(a, ns, g);
	}
	/*reset ns so that GC will release the new semispace*/
	ns.reset();
	/*This process has responsibility for doing a GC*/
	globals->GC();
	//note: ns and g will be invalid by now
	/*TODO: notify all other processes that the global
	has been modified
	*/
	/*Yes, assigning to a global variable is expensive!*/
}

