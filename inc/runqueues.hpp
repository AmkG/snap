#ifndef RUNQUEUES_H
#define RUNQUEUES_H

#include<queue>
#include<boost/shared_ptr.hpp>

class ProcessBase;

class RunQueue{
private:
	/*insert mutex for run queue here*/
	std::queue<boost::shared_ptr<ProcessBase> > Q;
public:
	void pop(boost::shared_ptr<ProcessBase>&);
	void pop_push(boost::shared_ptr<ProcessBase>&);
	void push(boost::shared_ptr<ProcessBase> const&);
};

#endif // RUNQUEUES_H


