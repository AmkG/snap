/*
*/

#include"phandles.hpp"
#include"processes.hpp"
#include"runsystems.hpp"

#include<boost/shared_ptr.hpp>
#include<boost/scoped_ptr.hpp>
#include<boost/weak_ptr.hpp>
#include<boost/enable_shared_from_this.hpp>
#include<iostream>
#include<stdexcept>

void ProcessHandle::operator()(void){
	if(pproc){
		ProcessStatus rv;
		try{
			try{
				rv = pproc->run();
			} catch(std::runtime_error& e) {
				std::cerr << "Process@" << std::hex <<
					((size_t) pproc.get()) << std::endl <<
					"Died to runtime error: " <<
					e.what() << std::endl;
				rv = process_dead;
			} catch(std::bad_alloc& e) {
				std::cerr << "Process@" << std::hex <<
					((size_t) pproc.get()) << std::endl <<
					"Died to memory allocation error" <<
					std::endl <<
					"(probably out of memory)" <<
					std::endl;
				rv = process_dead;
			}
		} catch(...) {
			std::cerr << "Process@" << std::hex <<
				((size_t) pproc.get()) << std::endl <<
				"Died to unspecified error" << std::endl;
			rv = process_dead;
		}
		switch(rv){
			case process_dead:
				pproc.reset();
				break;
			case process_running:
				runsystem->schedule(shared_from_this());
				break;
		}
	}
}

ProcessHandle::ProcessHandle(ProcessBase* npb)
		: pproc(npb){
	npb->handle = this;
}

