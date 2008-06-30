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
		/*NOTE!  The GC will acquire the Heap-based lock.
		However we can't release the global lock between the
		assign below and the GC, because the GC mutates some
		Heap member variables.  We are thus constrained to
		use *another* lock. We might try using a recursive lock
		instead, but a recursive lock is unnecessary for a
		true Process Heap.
		Since global assignment is expected to be rare anyway...
		*/
		globals->assign(a, ns, g);
		/*reset ns so that GC will release the new semispace*/
		ns.reset();
		/*This process has responsibility for doing a GC*/
		globals->GC();
	}
	//note: ns and g will be invalid by now
	/*TODO: notify all other processes that the global
	has been modified
	*/
	/*Yes, assigning to a global variable is expensive!*/
}


Generic* Process::get(boost::shared_ptr<Atom> a){
	/*TODO: check notification that global has been modified*/
	std::map<boost::shared_ptr<Atom>, Generic*>::iterator i =
		global_cache.find(a);
	if(i != global_cache.end()){
		return i->second;
	} else {
		/*look it up, then*/
		Generic* src;
		boost::shared_ptr<Semispace> ns;
		{/*insert locking of global here*/
			/*need to lock globals because assigning to globals might
			force a copy
			*/
			src = globals->get(a);
			if(src == NULL) throw ArcError("eval",
							"Unbound variable");
			ns = to_new_semispace(src);
		}/*release before locking something else*/
		{/*insert locking of our own other_spaces*/
			other_spaces.push_back(ns);
		}
		return src;
	}
}


