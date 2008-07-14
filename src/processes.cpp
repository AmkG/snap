/**/
#include"heaps.hpp"
#include"processes.hpp"
#include"variables.hpp"
#include"atoms.hpp"
#include<boost/shared_ptr.hpp>
#include"executors.hpp"

/*-----------------------------------------------------------------------------
Process
-----------------------------------------------------------------------------*/
/*
These are Arc processes
*/

void Process::get_root_set(std::stack<Generic**>& s){
	if(queue != NULL) s.push(&queue);
	if(_tobj != NULL) s.push(&_tobj);
	if(_nilobj != NULL) s.push(&_nilobj);
	for(std::vector<Generic*>::iterator i = mailbox.begin();
	    i != mailbox.end();
	    ++i){
		s.push(&*i);
	}
	for(std::vector<Generic*>::iterator i = stack.stack.begin();
	    i != stack.stack.end();
	    ++i){
		s.push(&*i);
	}
	for(std::map<boost::shared_ptr<Atom>, Generic* >::iterator i =
		global_cache.begin();
	    i != global_cache.end();
	    ++i){
		s.push(&(i->second));
	}
}

void Process::sendto(ProcessBase& p, Generic* g) const {
	boost::shared_ptr<Semispace> ns = to_new_semispace(g);
	/*g is modified by to_new_semispace, and now points
	to the copy in the new Semispace
	*/
	/*send message*/
	p.receive(ns, g);
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
	/*assign it (Global::assign will handle locking of the atom)*/
	globals->assign(a, ns, g);
	/*reset ns so that GC will release the old semispace*/
	ns.reset();
	/*TODO: notify all other processes that the global
	has been modified.  including itself
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
		std::pair<boost::shared_ptr<Semispace>, Generic*> nsv;
		boost::shared_ptr<Semispace> ns;
		Generic* src;
		/*globals->get handles locking of the atom for us*/
		nsv = globals->get(a);
		ns = nsv.first;// a new Semispace just for us
		src = nsv.second;
		if(!ns) throw ArcError("eval",
					"Unbound variable");
		{/*insert locking of our own other_spaces*/
			other_spaces.push_back(ns);
		}
		global_cache[a] = src;
		return src;
	}
}

/*used when we need to return a nil or t
Important!  tobj() and nilobj() are allocating
functions, so any data must be saved on the Arc
stack
*/
Generic* Process::tobj(void){
	if(_tobj != NULL)	return _tobj;
	else			return _tobj = new(*this) Sym(TATOM);
}
Generic* Process::nilobj(void){
	if(_nilobj != NULL)	return _nilobj;
	else			return _nilobj = new(*this) Sym(NILATOM);
}

void Process::receive(boost::shared_ptr<Semispace> s, Generic* o){
	/*insert locking of other_spaces here*/
	other_spaces.push_back(s);
	mailbox.push_back(o);
}

ProcessStatus Process::run(void){
	// TODO: number should eventually be related to priority
	return execute(*this, 64);
}

boost::shared_ptr<ProcessBase> NewArcProcess(void){
	boost::shared_ptr<ProcessBase> rv (new Process());
	return rv;
}

/*f is the starting function, ns is a Semispace containing that function*/
boost::shared_ptr<ProcessBase> NewArcProcess(
		boost::shared_ptr<Semispace>ns, Generic* f){
	Process* np = new Process();
	boost::shared_ptr<ProcessBase> rv(np);
	/*no need to lock other_spaces: this is a new process anyway*/
	np->other_spaces.push_back(ns);
	np->stack.push(f);
	return rv;
}


