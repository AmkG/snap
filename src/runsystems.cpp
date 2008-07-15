/*
*/
#include"runsystems.hpp"
#include"phandles.hpp"

#include<boost/shared_ptr.hpp>
#include<queue>

/*single worker thread implementation*/

/*
Probably use *unofficial* boost::thread::threadpool on
http://threadpool.sourceforge.net/ for multiple
worker threads
*/

class SingleWorkerRunsystem : public Runsystem {
private:
	std::queue<boost::shared_ptr<ProcessHandle> > Q;
public:
	virtual void schedule(boost::shared_ptr<ProcessHandle> const& s){
		Q.push(s);
	}
	virtual void run(void){
		while(!Q.empty()){
			ProcessHandle& hproc = *Q.front();
			hproc();
			Q.pop();
		}
	}
	virtual ~SingleWorkerRunsystem(){}
};

boost::shared_ptr<Runsystem> NewRunsystem(void){
	boost::shared_ptr<Runsystem> rv(new SingleWorkerRunsystem());
	return rv;
}


