#ifndef PORTS_H
#define PORTS_H
#include<vector>
#include<boost/scoped_ptr.hpp>
#include<boost/shared_ptr.hpp>
#include<string>

class AsyncPortSet;

/*abstract class*/
class AsyncPort{
public:
	// returns true if input is available on the port
	virtual bool input_available(void) =0;
	// size of vector denotes desired number of characters
	//  (call vector::resize() before calling this function)
	// upon return, vector now contains the actual read
	//  number of bytes
	virtual void input(std::vector<unsigned char>&) =0;
	virtual bool output_available(void) =0;
	virtual size_t output(std::vector<unsigned char> const&,
				size_t off=0) =0;
	virtual bool is_input(void) const =0;
	virtual bool is_output(void) const =0;
	virtual ~AsyncPort(){};

	friend class AsyncPortSet;
};

boost::shared_ptr<AsyncPort> AsyncSTDIN(void);
boost::shared_ptr<AsyncPort> AsyncSTDOUT(void);
boost::shared_ptr<AsyncPort> AsyncSTDERR(void);

boost::shared_ptr<AsyncPort> InputFile(std::string);
boost::shared_ptr<AsyncPort> OutputFile(std::string);

/*for the convenience of the implementation*/
#define DECLARE_ASYNC_PORT(T)\
	virtual bool input_available(void);\
	virtual void input(std::vector<unsigned char>&);\
	virtual bool output_available(void);\
	virtual size_t output(std::vector<unsigned char> const&,
				size_t off=0 );\
	virtual bool is_input(void) const;\
	virtual bool is_output(void) const;\
	virtual ~T();\
	friend boost::shared_ptr<AsyncPort> AsyncSTDIN(void);\
	friend boost::shared_ptr<AsyncPort> AsyncSTDOUT(void);\
	friend boost::shared_ptr<AsyncPort> AsyncSTDERR(void);\
	friend class AsyncPortSet;
/*include above as friends, we can't be too sure that
those functions might, in some random implementation,
actually construct a derived concrete class.
*/

/*-------------------------------------------sets of AsyncPorts for waiting*/

class AsyncPortSet{
public:
	virtual void add(boost::shared_ptr<AsyncPort>) =0;
	virtual void remove(boost::shared_ptr<AsyncPort>) =0;
	//nonblocking
	// returns all asynchronous ports that have data
	// waiting.  those ports are automatically removed
	// from this set.
	virtual std::vector<boost::shared_ptr<AsyncPort> > check(void) =0;
	//blocking (if we detect that we're the only running
	//process)
	virtual std::vector<boost::shared_ptr<AsyncPort> > wait(void) =0;
	AsyncPortSet();
	virtual ~AsyncPortSet();
};

#endif //PORTS_H

