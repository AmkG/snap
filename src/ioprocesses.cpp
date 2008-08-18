/*
*/
#include"ioprocesses.hpp"
#include"variables.hpp"
#include"phandles.hpp"
#include"runsystems.hpp"

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

