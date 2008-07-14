/*
*/
#include"worker-threads.hpp"
#include"processes.hpp"
#include"runqueues.hpp"

#include<boost/shared_ptr.hpp>
#include<iostream>
#include<stdexcept>

void WorkerThread::operator()(void) const {
	boost::shared_ptr<ProcessBase> curp;
	ProcessStatus rv;
	runqueue->pop(curp);
	while(curp){
		try{
			try{
				rv = curp->run();
			} catch(std::exception& e) {
				std::cerr << "Process@" << std::hex <<
					((size_t) curp.get()) <<
					" died to error:" << std::endl;
				std::cerr << e.what() << std::endl;
				rv = process_dead;
			}
		} catch(...) {
			std::cerr << "Process@" << std::hex <<
				((size_t) curp.get()) <<
				" died to unknown exception" << std::endl;
			rv = process_dead;
		}
		if(rv == process_running){
			runqueue->pop_push(curp);
		} else	runqueue->pop(curp);
	}
}

