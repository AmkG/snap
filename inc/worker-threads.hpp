#ifndef WORKER_THREADS_H
#define WORKER_THREADS_H

/*A functor class
boost::thread accepts any functor object with a
void foo(void) signature.
*/
class WorkerThread {
public:
	void operator()(void) const ;
};

#endif // WORKER_THREADS_H
