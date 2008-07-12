#ifndef PROCESS_H
#define PROCESS_H
#include"heaps.hpp"
#include"errors.hpp"
#include<vector>
#include<stack>
#include<map>
#include<boost/shared_ptr.hpp>

class Process;

/*Use our own stack in order to support restack()*/
class ProcessStack{
private:
	std::vector<Generic*> stack;
public:
	Generic*& top(size_t off=1){
		if(off > stack.size() || off == 0){
			throw ArcError("internal",
					"Process stack underflow in top()");
		}
		return stack[stack.size() - off];
	}
	void pop(size_t num=1){
		if(num > stack.size()){
			throw ArcError( "internal",
					"Process stack underflow in pop()");
		}
		if(num != 0) stack.resize(stack.size() - num);
	}
	void push(Generic* gp){
		stack.push_back(gp);
	}
	/*Used in function calls*/
	void restack(size_t sz){
		if(sz > stack.size()){
			throw ArcError( "internal",
					"Process stack underflow in restack()");
		}
		size_t off = stack.size() - sz;
		/*Not exactly the best way to do it?*/
		if(sz != 0) stack.erase(stack.begin(), stack.begin() + off);
	}
	Generic*& operator[](int i){
		return stack[i];
	}
	size_t size(void) const{ return stack.size(); };
	friend class Process;
};

typedef enum _e_ProcessStatus {
	process_running, process_waiting, process_dead
} ProcessStatus;

class Semispace;

/*base process class*/
/*the base process class includes built-in non-Arc system processes
(e.g. i/o processes)
*/
class ProcessBase {
public:
	virtual void receive(boost::shared_ptr<Semispace>, Generic*) =0;
	virtual ProcessStatus run(void) =0;
	virtual ~ProcessBase(){};
};

class Atom;

class Process : public Heap, public ProcessBase {
private:
	//should also be locked using the other_spaces lock
	std::vector<Generic*> mailbox;
	Generic* queue;//handled by Arc-side code
	std::map<boost::shared_ptr<Atom>, Generic*> global_cache;
	Generic* _tobj;
	Generic* _nilobj;
protected:
	virtual void get_root_set(std::stack<Generic**>&);
public:
	ProcessStack stack;
	void sendto(ProcessBase&, Generic*) const ;
	void assign(boost::shared_ptr<Atom>, Generic*) const ;
	Generic* get(boost::shared_ptr<Atom>);
	virtual ~Process(){};
	Process() : Heap(), queue(NULL), _tobj(NULL), _nilobj(NULL) {};
	Generic* tobj(void);
	Generic* nilobj(void);
	virtual void receive(boost::shared_ptr<Semispace>, Generic*);
	virtual ProcessStatus run(void);
};

#endif //PROCESS_H

