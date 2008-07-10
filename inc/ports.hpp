#ifndef PORTS_H
#define PORTS_H
#include<vector>
#include<boost/scoped_ptr.hpp>
#include<boost/shared_ptr.hpp>
#include<string>

/*abstract class*/
class AsyncPort{
public:
	// returns true if input is available on the port
	virtual bool input_available(void) const =0;
	// size of vector denotes desired number of characters
	//  (call vector::resize() before calling this function)
	// upon return, vector now contains the actual read
	//  number of bytes
	virtual void input(std::vector<unsigned char>&) =0;
	virtual bool output_available(void) const =0;
	virtual size_t output(std::vector<unsigned char> const&,
				size_t off=0) =0;
	virtual bool is_input(void) const =0;
	virtual bool is_output(void) const =0;
	virtual ~AsyncPort(){};
};

boost::shared_ptr<AsyncPort> AsyncSTDIN(void);
boost::shared_ptr<AsyncPort> AsyncSTDOUT(void);
boost::shared_ptr<AsyncPort> AsyncSTDERR(void);

#define DECLARE_ASYNC_PORT(T)\
	virtual bool input_available(void) const;\
	virtual void input(std::vector<unsigned char>&);\
	virtual bool output_available(void) const;\
	virtual size_t output(std::vector<unsigned char> const&,
				size_t off=0 );\
	virtual bool is_input(void) const;\
	virtual bool is_output(void) const;\
	virtual ~T();\
	friend boost::shared_ptr<AsyncPort> AsyncSTDIN(void);\
	friend boost::shared_ptr<AsyncPort> AsyncSTDOUT(void);\
	friend boost::shared_ptr<AsyncPort> AsyncSTDERR(void);
/*include above as friends, we can't be too sure that
those functions might, in some random implementation,
actually construct a derived concrete class.
*/

/*---------------------------------------------------------concrete classes*/

class InputFileImplementation;

class InputFilePort : public AsyncPort{
private:
	//Pointer to implementation
	boost::scoped_ptr<InputFileImplementation> p;
	InputFilePort();
public:
	DECLARE_ASYNC_PORT(InputFilePort)
	explicit InputFilePort(std::string const&);
};

class OutputFileImplementation;

class OutputFilePort : public AsyncPort{
private:
	//Pointer to implementation
	boost::scoped_ptr<OutputFileImplementation> p;
	OutputFilePort();
public:
	DECLARE_ASYNC_PORT(OutputFilePort)
	explicit OutputFilePort(std::string const&);
};

/*question: can we do terminal-based i/o that way or no?*/

/*-------------------------------------------sets of AsyncPorts for waiting*/

class AsyncPortSetImplementation;

class AsyncPortSet{
private:
	boost::scoped_ptr<AsyncPortSetImplementation> p;
public:
	void add(boost::shared_ptr<AsyncPort>);
	void remove(boost::shared_ptr<AsyncPort>);
	//nonblocking
	// returns all asynchronous ports that have data
	// waiting.  those ports are automatically removed
	// from this set.
	std::vector<boost::shared_ptr<AsyncPort> > check(void);
	//blocking (if we detect that we're the only running
	//process)
	std::vector<boost::shared_ptr<AsyncPort> > wait(void);
	AsyncPortSet();
	~AsyncPortSet();
};

#endif //PORTS_H

