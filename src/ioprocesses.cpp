/*
*/
#include"ioprocesses.hpp"
#include"variables.hpp"

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
2. detect if we are the only process running.  If we are call
   the CentralIO::wait(), otherwise just CentralIO::poll()
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

}

