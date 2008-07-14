/*
*/

#include"runqueues.hpp"

#include<boost/shared_ptr.hpp>
#include<queue>

/*NOTE!  Better to use lock-free implementation if available!*/
void pop(boost::shared_ptr<ProcessBase>& rv){
	/*insert locking of runqueue here*/
	if(Q.empty()){
		rv.reset(); return;
	}
	rv = Q.front();
	Q.pop();
}

void pop_push(boost::shared_ptr<ProcessBase>& rv){
	/*insert locking of runqueue here*/
	if(Q.empty()) return; // don't bother pushing if empty
	Q.push(rv);
	rv = Q.front();
	Q.pop();
}

void push(boost::shared_ptr<ProcessBase> const& v){
	/*insert locking of runqueue here*/
	Q.push(v);
}

