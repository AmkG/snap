/*
*/
#include"types.hpp"
#include"heaps.hpp"

/*O(N), where N is the number of objects involved!*/
/*necessary for determining the size of the semispace
to be allocated when sending messages.
*/
size_t Generic::total_size(void){
	ToPointerLock toptrs;
	std::stack<Generic**> s;
	Generic** gpp;
	Generic* to;
	size_t rv = get_size();
	/*Use the to-pointers as expensive marks*/
	toptrs.pointto(this, this);
	get_refs(s);
	while(!s.empty()){
		gpp = s.top(); s.pop();
		to = toptrs.to(*gpp);
		if(to == NULL){
			toptrs.pointto(*gpp, *gpp);
			rv += (*gpp)->get_size();
			(*gpp)->get_refs(s);
		}
	}
	/*toptrs' dtor should clear the to-pointers of the objects*/
	return rv;
}

