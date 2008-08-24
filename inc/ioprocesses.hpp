#ifndef IOPROCESSES_H
#define IOPROCESSES_H
#include<boost/shared_ptr.hpp>
#include<boost/scoped_ptr.hpp>
#include<vector>
#include<utility>
#include<stack>
#include<string>

#include"processes.hpp"

class PortData; /*abstract port data class*/

class CentralIOToDo;

enum IOActionType {
	ioaction_read,		/*read some bytes*/
	ioaction_write,		/*write some bytes*/
	ioaction_stdin,		/*get PortData for stdin*/
	ioaction_stdout,	/*get PortData for stdout*/
	ioaction_stderr,	/*get PortData for stderr*/
	ioaction_open,		/*open a filesystem file*/
	ioaction_close,		/*close a PortData*/
	ioaction_server,	/*create a server at port*/
	ioaction_listen,	/*listen on a server PortData*/
	ioaction_sleep,		/*wait a particular amount of time*/
	ioaction_system		/*start another OS process and wait*/
};

class IOAction {
private:
	boost::shared_ptr<ProcessHandle> requester;
public:
	IOActionType action;
	boost::shared_ptr<PortData> port;
	boost::shared_ptr<std::vector<unsigned char> > data;
	int num;
	std::string str;
	friend class CentralIOToDo;
};

/*abstract base*/
class CentralIO {
public:
	/*performs members of the to-do lists that
	can be done now, and returns as soon as those
	are handled
	*/
	virtual void poll(CentralIOToDo) =0;
	/*performs members of the to-do lists
	that can be done now.  If there aren't
	any, waits until at least one can be
	done.
	*/
	virtual void wait(CentralIOToDo) =0;
	/*determine if there are any pending request: returns
	true if no pending requests
	*/
	virtual bool empty(void) =0;
	virtual ~CentralIO(){}
};

CentralIO* NewCentralIO(void);

class CentralIOProcess;

CentralIOProcess* NewCentralIOProcess(void);

/*PImpl*/
class CentralIOProcess : public ProcessBase {
private:
	boost::scoped_ptr<CentralIO> impl;

	bool waiting;

	typedef std::pair<boost::shared_ptr<Semispace>, Generic* > message;
	typedef std::pair<boost::shared_ptr<ProcessHandle>, message> response;

	/*received queue*/
	std::vector<message> rcv_queue;//protect with lock
	/*to-do list*/
	std::vector<IOAction> todo;
	/*for commission*/
	std::vector<response> snd_queue;

	/*ctor*/
	CentralIOProcess() : impl(NewCentralIO()), waiting(1) {}

public:
	virtual bool receive(boost::shared_ptr<Semispace>, Generic*);
	virtual ProcessStatus run(void);
	virtual ~CentralIOProcess(){}

	friend class CentralIOToDo;
	friend CentralIOProcess* NewCentralIOProcess(void);
};

/*TODO: think of best way to do this*/
class CentralIOToDo {
private:
	CentralIOProcess* proc;
public:
	explicit CentralIOToDo(CentralIOProcess* p) : proc(p){}
	bool empty(void){
		return proc->todo.empty();
	}
	/*returns an IOAction.  Once an IOAction is released
	using this, it is the responsibility of the CentralIO
	object.
	*/
	IOAction get(void){
		IOAction tmp = proc->todo.back();
		proc->todo.pop_back();
		return tmp;
	}

	/*responses to an IOAction*/
	void respond(IOAction);
	void error(IOAction, std::string);
	void eof(IOAction);
};

#endif //IOPROCESSES_H
