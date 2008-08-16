#ifndef IOPROCESSES_H
#define IOPROCESSES_H
#include<boost/shared_ptr.hpp>
#include<boost/scoped_ptr.hpp>
#include<vector>
#include<utility>
#include<set>

#include"processes.hpp"

class PortData; /*abstract port data class*/

class CentralIOToDo;

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
	/*gets various standard I/O ports*/
	virtual boost::shared_ptr<PortData> get_stdin(void) =0;
	virtual boost::shared_ptr<PortData> get_stdout(void) =0;
	virtual boost::shared_ptr<PortData> get_stderr(void) =0;
	virtual ~CentralIO();
}

CentralIO* NewCentralIO(void);

/*PImpl*/
class CentralIOProcess : public ProcessBase {
private:
	boost::scoped_ptr<CentralIO> impl;

	bool waiting;

	typedef std::pair<boost::shared_ptr<Semispace>, Generic* > message;

	/*received queue*/
	std::vector<message> rcv_queue;//protect with lock
	/*to-do list*/
	std::set<IOAction> todo;
	/*for commission*/
public:
	virtual bool receive(boost::shared_ptr<Semispace>, Generic*);
	virtual ProcessStatus run(void);
	virtual ~CentralIOProcess(){}

	CentralIOProcess() : impl(NewCentralIO()), waiting(0) {}

	friend class CentralIOToDo;
};

CentralIOProcess* NewCentralIOProcess(void);

class CentralIOToDo {
private:
	CentralIOProcess* proc;
public:
	explicit CentralIOToDo(CentralIOProcess* p) : proc(p){}
};

#endif //IOPROCESSES_H
