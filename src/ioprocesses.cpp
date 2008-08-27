/*
*/
#include"ioprocesses.hpp"
#include"variables.hpp"
#include"phandles.hpp"
#include"runsystems.hpp"
#include"types.hpp"

/*
static variables
*/

static boost::shared_ptr<Atom> responseatom;
static boost::shared_ptr<Atom> ioatom;
static boost::shared_ptr<Atom> erratom;
static boost::shared_ptr<Atom> eofatom;

/*-----------------------------------------------------------------------------
CentralIOProcess
-----------------------------------------------------------------------------*/

bool CentralIOProcess::receive(
		boost::shared_ptr<Semispace> sp, Generic* gp) {
	/*insert trylock here, if trylock fails, return false*/
	rcv_queue.push_back(message(sp, gp));
	if(waiting) {
		runsystem->schedule(handle->mypid());
		waiting = 0;
	}
}

/*
1. parse each entry in the receive queue
2. detect if we are the only process running.  If we are, call
   CentralIO::wait(), otherwise just CentralIO::poll()
   2.a. CentralIO should then create responses for requests
        that were processed (e.g. have data, or have some
        error event)
3. collect responses and send to pending processes
   3.a. if the process isn't ready, then keep the response
4. If no responses are pending, check the receive queue.
   If the receive queue is still empty, set our waiting flag
   and return process_waiting
*/
ProcessStatus CentralIOProcess::run(void) {
	/*1*/
	{/*insert trylock here*/
		/*if (trylock.succeeded())*/ {
			/*TODO: parse*/
		}
	}
	/*2*/
	if(runsystem->singleprocess()){
		impl->wait(CentralIOToDo(this));
	} else	impl->poll(CentralIOToDo(this));
	/*3*/
	/*try sending to each process*/
	std::vector<response>::iterator i, to;
	for( i = to = snd_queue.begin(); i != snd_queue.end(); ++i){
		response& curr = *i;
		message& msg = curr.second;
		if(curr.first->pproc->receive(msg.first, msg.second)){
			/*successful send; will get deleted, do nothing*/
		} else {
			/*unsucessful send; move*/
			*to = *i;
			++to;
		}
	}
	snd_queue.erase(to, i);
	/*4*/
	/*check that everything is empty*/
	if(impl->empty() && todo.empty() && snd_queue.empty()) {
		/*insert trylock, if trylock fails, return process_running*/
		if(rcv_queue.empty()) {
			/*now return process_waiting*/
			waiting = 1;
			return process_waiting;
		}
	}
	return process_running;
}

CentralIOProcess* NewCentralIOProcess(void) {
	responseatom = globals->lookup("response");
	ioatom = globals->lookup("i/o");
	erratom = globals->lookup("err");
	eofatom = globals->lookup("eof");
	return new CentralIOProcess();
}

/*-----------------------------------------------------------------------------
CentralIOToDo
-----------------------------------------------------------------------------*/

void CentralIOToDo::send(boost::shared_ptr<ProcessHandle> const& pid,
		boost::shared_ptr<Semispace> const& ns, Generic* gp) {
	proc->snd_queue.push_back(
		CentralIOProcess::response(
			pid,
			CentralIOProcess::message(ns, gp)
		)
	);
}

/*responses*/

void CentralIOToDo::respond(IOAction io) {
	/*figure out size of response for new semispace*/
	/*3 cons cells, 3 symbols, and one variable data
	(cons (sym respond) (cons (sym ,tag) (cons data (sym nil))))
	*/
	size_t sz = sizeof(Cons) * 3 + sizeof(Sym) * 3;

	/*data response depends on specific action*/
	switch(io.action){
	case ioaction_read:
	case ioaction_write:
		sz += sizeof(BinaryBlob);
		break;
	case ioaction_stdin:
	case ioaction_stdout:
	case ioaction_stderr:
	case ioaction_open:
	case ioaction_server:
	case ioaction_listen:
		sz += sizeof(ArcPortData);
		break;
	case ioaction_system:
		sz += sizeof(Integer);
		break;
	/*these have nil as data*/
	case ioaction_sleep:
	case ioaction_close:
		break;
	}
	boost::shared_ptr<Semispace> ns(new Semispace(sz));
	Semispace& sp = *ns;

	/*build the response*/
	Cons* cp1 = new(sp) Cons();
	cp1->a = new(sp) Sym(responseatom);
	Cons* cp2 = new(sp) Cons();
	cp1->d = cp2;
	cp2->a = new(sp) Sym(io.tag);
	Cons* cp3 = new(sp) Cons();
	cp2->d = cp3;
	cp3->d = new(sp) Sym(NILATOM);

	/*figure out the appropriate data field*/
	switch(io.action){
	case ioaction_read:
	case ioaction_write:
	{	BinaryBlob* bp = new(sp) BinaryBlob();
		cp3->a = bp;
		bp->pdat = io.data;
	} break;
	case ioaction_stdin:
	case ioaction_stdout:
	case ioaction_stderr:
	case ioaction_open:
	case ioaction_server:
	case ioaction_listen:
		cp3->a = new(sp) ArcPortData(io.port);
		break;
	case ioaction_system:
		cp3->a = new(sp) Integer(io.num);
		break;
	/*these have nil as data*/
	case ioaction_sleep:
	case ioaction_close:
		cp3->a = cp3->d;
		break;
	}

	/*now push it onto the send queue*/
	send(io.requester,  ns, cp1);
}

void CentralIOToDo::error(IOAction io, std::string err) {
	/*for now, simply use a symbol with the message*/
	/*TODO: in the future, the error message should be
	a string, not a symbol
	*/
	/*
	(cons (sym err)
	      (cons (sym tag)
	            (cons (annotate (sym 'i/o) data)
	                  (sym nil))))
	*/
	size_t sz = sizeof(Cons) * 3 + sizeof(Tagged) + sizeof(Sym) * 4
			+ sizeof(Sym);// error object

	boost::shared_ptr<Semispace> ns(new Semispace(sz));
	Semispace& sp = *ns;

	Cons* cp1 = new(sp) Cons();
	cp1->a = new(sp) Sym(erratom);
	Cons* cp2 = new(sp) Cons();
	cp1->d = cp2;
	cp2->a = new(sp) Sym(io.tag);
	Cons* cp3 = new(sp) Cons();
	cp2->d = cp3;
	Tagged* tp = new(sp) Tagged();
	tp->type_o = new(sp) Sym(ioatom);
	tp->rep_o = new(sp) Sym(globals->lookup(err)); // replace in the future
	cp3->a = tp;
	cp3->d = new(sp) Sym(NILATOM);

	/*now push it onto the send queue*/
	send(io.requester,  ns, cp1);
}

void CentralIOToDo::eof(IOAction io) {
	/*(cons (sym 'eof) (cons (sym tag) (sym nil)))*/
	size_t sz = sizeof(Cons) * 2 + sizeof(Sym) * 3;

	boost::shared_ptr<Semispace> ns(new Semispace(sz));
	Semispace& sp = *ns;

	Cons* cp1 = new(sp) Cons();
	cp1->a = new(sp) Sym(eofatom);
	Cons* cp2 = new(sp) Cons();
	cp1->d = cp2;
	cp2->a = new(sp) Sym(io.tag);
	cp2->d = new(sp) Sym(NILATOM);

	/*now push it onto the send queue*/
	send(io.requester,  ns, cp1);
}

