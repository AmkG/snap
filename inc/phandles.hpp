#ifndef PROCESS_HANDLES_H
#define PROCESS_HANDLES_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class ProcessBase;
class ProcessHandle : public boost::enable_shared_from_this<ProcessHandle>{
public:
	boost::scoped_ptr<ProcessBase> pproc;
	void operator()(void);
	explicit ProcessHandle(ProcessBase*);
	boost::shared_ptr<ProcessHandle> mypid(void){
		return shared_from_this();
	}
};

#endif // PROCESS_HANDLES_H
