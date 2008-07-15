#ifndef RUNSYSTEMS_H
#define RUNSYSTEMS_H

#include<boost/shared_ptr.hpp>

class ProcessHandle;
class Runsystem;
Runsystem* NewRunsystem(void);

class Runsystem {
public:
	//adds the process to scheduling
	virtual void schedule(boost::shared_ptr<ProcessHandle> const&) =0;
	//runs all scheduled processes, then returns when all
	//have completed
	virtual void run(void) =0;
	virtual ~Runsystem(){}
};

#endif // RUNSYSTEMS_H

