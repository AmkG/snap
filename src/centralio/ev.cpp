/*
*/

#include"ioprocesses.hpp"

#include<stdexcept>
#include<ev++.h>
#include<unistd.h>
#include<fcntl.h>

#include<string>
#include<boost/scoped_ptr.hpp>
#include<vector>

const size_t READ_SIZE;

class PortData {
public:
	int fd;
	boost::scoped_ptr<ev::io> iop;
	IOAction ioa;
	CentralIOToDo todo;
	PortData(int nfd, int events) : fd(nfd), iop(new ev::io), ioap() {
		iop->set(nfd, events);
	}
	bool check_select(int typ) {
		/*TODO*/
	}
	void do_read(ev::io& w, int revents) {
		/*now attempt the read*/
		/*first use a select() to check: the event loop
		might falsely trigger
		*/
		if((revents & ev::READ) && check_select(ev::READ)) {
			/*now attempt a read*/
			ioa.data.reset(
				new std::vector<unsigned char>(READ_SIZE));
			ssize_t rv = read(fd, io.data->begin(), READ_SIZE);
			if(rv < 0) {
				/*potential errors*/
				switch(errno) {
				/*ignore these*/
				case EAGAIN:
				case EINTR:
					return;
					break;
				case EBADF:
					todo.err(ioa, "not valid, or not "
							"open for reading");
					break;
				case EFAULT:
					todo.err(ioa, "internal error, buffer "
							"misallocated");
					break;
				case EINVAL:
					todo.err(ioa, "unsuitable for "
							"reading");
					break;
				case EIO:
					todo.err(ioa, "low-level i/o error");
					break;
				default:
					todo.err(ioa, "unknown error type");
					break;
				}
			} else if(rv == 0) {
				todo.eof(ioa);
			} else {
				ioa.data->resize(rv);
				todo.respond(ioa);
			}
			w.stop();
		}
	}
	void do_write(ev::io& w, int revents) {
	}
};

boost::shared_ptr<PortData> pd_stdin;
boost::shared_ptr<PortData> pd_stdout;
boost::shared_ptr<PortData> pd_stderr;

class EvCentralIO : public CentralIO {
private:
	void getall(CentralIOToDo todo) {
		while(!todo.empty()) {
			IOAction io = todo.get();
			switch(io.action) {
			case ioaction_stdin:
				if(!pd_stdin) {
					pd_stdin.reset(
						new PortData(STDIN_FILENO,
								ev::READ));
				}
				io.port = pd_stdin;
				todo.respond(io);
				break;
			case ioaction_stdout:
				if(!pd_stdout) {
					pd_stdout.reset(
						new PortData(STDOUT_FILENO,
								ev::WRITE));
				}
				io.port = pd_stdout;
				todo.respond(io);
				break;
			case ioaction_stderr:
				if(!pd_stderr) {
					pd_stderr.reset(
						new PortData(STDERR_FILENO,
								ev::WRITE));
				}
				io.port = pd_stderr;
				todo.respond(io);
				break;
			case ioaction_read:
				PortData& port = *io.port;
				port.ioa = io;
				port.todo = todo;
				port.iop->set<PortData, &PortData::do_read>(
						&port);
				port.iop->start();
				break;
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
	bool empty(void) const {
		/*TODO*/
		return 0;
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

