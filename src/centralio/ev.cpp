/*
*/

#include"ioprocesses.hpp"

#include<stdexcept>
#include<ev++.h>
#include<unistd.h>
#include<fcntl.h>
#include<string>

class PortData {
public:
	int fd;
	PortData(int nfd) : fd(nfd) {}
};

class EvCentralIO : public CentralIO {
private:
	void getall(CentralIOToDo todo) {
		while(!todo.empty()) {
			IOAction io = todo.get();
			switch(io.action) {
			case ioaction_stdin:
				io.port.reset(new PortData(STDIN_FILENO));
				todo.respond(io);
				break;
			case ioaction_stdout:
				io.port.reset(new PortData(STDOUT_FILENO));
				todo.respond(io);
				break;
			case ioaction_stderr:
				io.port.reset(new PortData(STDERR_FILENO));
				todo.respond(io);
				break;
			case ioaction_read:
			case ioaction_write:
			default:
				todo.error(io, std::string("not implemented"));
			}
		}
	}
public:
	void poll(CentralIOToDo todo) {
		getall(todo);
		ev_loop(ev_default_loop(0), EVLOOP_NONBLOCK);
	}
	void wait(CentralIOToDo todo) {
		getall(todo);
		ev_loop(ev_default_loop(0), EVLOOP_ONESHOT);
	}
	virtual ~EvCentralIO(){}
};

CentralIO* NewCentralIO(void) {
	struct ev_loop* evloop = ev_default_loop(EVFLAG_AUTO);
	if(!evloop) {
		throw std::runtime_error("Unable to initialize libev");
	}
	return new EvCentralIO();
}

