/**/
#include"heaps.hpp"
#include"processes.hpp"

/*-----------------------------------------------------------------------------
Process
-----------------------------------------------------------------------------*/

void Process::sendto(Process& p, Generic* g){
	Generic* ng = transfer(g, p);
	/*add to mailbox*/
	{/*insert locking of destination's other_spaces here*/
		p.mailbox.push_back(ng);
	}
	/*TODO: if destination process is waiting for a message,
	schedule it
	*/
}
/*
Scheduling:
*/
