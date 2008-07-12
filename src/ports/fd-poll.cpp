/*
ports implementation using POSIX open() and poll().
*/

#include"ports.hpp"
#include"errors.hpp"

#include<sys/unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<poll.h>
#include<stdlib.h>
#include<errno.h>

#include<string>
#include<boost/scoped_ptr.hpp>
#include<map>
#include<vector>

typedef int FD;
enum port_dir{
	input_direction, output_direction
};

class OpenFileBase : public AsyncPort {
public:
	FD fd;
	boost::scoped_ptr<std::string> zombie;
	OpenFileBase() : fd(), zombie(){}
	OpenFileBase(FD nfd) : fd(nfd), zombie(){}
	virtual bool dezombify(void)=0;
};

template<int oflags, enum port_dir dir, bool autoclose=1>
class OpenFile : public OpenFileBase{
public:

	void try_open(std::string& s){
		char* sb = (char*)malloc(s.size()+1);
		s.copy(sb, s.size());
		sb[s.size()] = 0;
		FD nfd = open(sb, oflags);
		free(sb);
		if(nfd < 0){
			switch(errno){
			case ENOMEM:
			case EMFILE:
			case ENFILE:
				zombie.reset(&s);
				break;
			default:
				throw ArcError("i/o",
					"failed to open file");
			}
		} else {
			fd = nfd;
			zombie.reset();
		}
	}
	bool dezombify(void){
		if(zombie){
			try_open(*zombie);
			if(zombie)	return 0;
			else		return 1;
		} else return 1;
	}
	/*throw if we're not an input*/
	void expect_input(void){
		if(dir != input_direction) {
			throw ArcError("i/o",
				"attempt to read from port that is not an "
				"input port");
		}
	}
	/*throw if we're not an output*/
	void expect_output(void){
		if(dir != output_direction) {
			throw ArcError("i/o",
				"attempt to write from port that is not an "
				"output port");
		}
	}
	explicit OpenFile(FD nfd)
		: OpenFileBase(nfd){}
	explicit OpenFile(std::string s){
		try_open(s);
	}
	~OpenFile(){
		if(autoclose && !zombie) close(fd);
		// TODO: how to handle failed close?
	}
	bool do_poll(int events) {
		if(dezombify()){
			/*construct poll*/
			struct pollfd pfd[1];
			pfd[0].fd = fd;
			pfd[0].events = events;
			int rv = poll(pfd,1,0);
			if(rv == 0)		return 0;
			else if(rv == 1)	return 1;
			else throw ArcError("i/o",
				"error checking port status");
		} else return 0;
	}
	bool input_available(void){
		expect_input();
		return do_poll(POLLIN);
	}
	/*preconditions: exactly one previous input_available() call
	returned 1
	*/
	void input(std::vector<unsigned char>& dat){
		expect_input();
		/*couldn't have been zombified, since if it's a zombie
		then input_available() would not return true
		*/
		unsigned char* dp = &dat[0];
		ssize_t len = read(fd,dp,dat.size());
		if(len < 0){
			throw ArcError("i/o",
				"error reading from port");
		} else {
			dat.resize((size_t) len);
		}
	}
	bool output_available(void){
		expect_output();
		return do_poll(POLLOUT);
	}
	size_t output(std::vector<unsigned char> const & dat,
			size_t off){
		expect_output();
		ssize_t len = write(fd, &dat[0], dat.size() - off);
		if(len < 0){
			throw ArcError("i/o",
				"error writing to port");
		} else {
			return (size_t) len;
		}
	}
	bool is_input(void) const {
		return dir == input_direction;
	}
	bool is_output(void) const {
		return dir == output_direction;
	}
};

boost::shared_ptr<AsyncPort> AsyncSTDIN(void){
	return boost::shared_ptr<AsyncPort>(
		new OpenFile<O_RDONLY, input_direction>(STDIN_FILENO));
}

boost::shared_ptr<AsyncPort> AsyncSTDOUT(void){
	return boost::shared_ptr<AsyncPort>(
		new OpenFile<O_WRONLY, output_direction>(STDOUT_FILENO));
}

boost::shared_ptr<AsyncPort> AsyncSTDERR(void){
	return boost::shared_ptr<AsyncPort>(
		new OpenFile<O_WRONLY, output_direction>(STDERR_FILENO));
}


boost::shared_ptr<AsyncPort> InputFile(std::string s){
	return boost::shared_ptr<AsyncPort>(
		new OpenFile<O_RDONLY, input_direction>(s));
}
boost::shared_ptr<AsyncPort> OutputFile(std::string s){
	return boost::shared_ptr<AsyncPort>(
		new OpenFile<O_WRONLY | O_TRUNC | O_CREAT, output_direction>(
			s));
};

/*AsyncPortSet*/

class AsyncPortSetImpl : public AsyncPortSet {
public:
	std::map<FD, boost::shared_ptr<AsyncPort> > live;
	std::map<FD, boost::shared_ptr<AsyncPort> > zombies;
	void dezombify(void){
		if(zombies.size() == 0) return;
		std::map<FD, boost::shared_ptr<AsyncPort> > nzombies;
		std::map<FD, boost::shared_ptr<AsyncPort> >::iterator i;
		OpenFileBase* fp;
		for(i = zombies.begin(); i != zombies.end(); ++i){
			fp = dynamic_cast<OpenFileBase*>(i->second.get());
			if(fp == NULL){
				 throw std::runtime_error(
						"unexpected type of port "
						"in port set");
			} else if(fp->dezombify()){
				live[i->first] = i->second;
			} else {
				nzombies[i->first] = i->second;
			}
		}
		zombies.swap(nzombies);
	}
	AsyncPortSetImpl(){}
	~AsyncPortSetImpl(){}
	void add(boost::shared_ptr<AsyncPort> np){
		OpenFileBase* fp = dynamic_cast<OpenFileBase*>(np.get());
		if(fp == NULL){
			throw std::runtime_error("unexpected type of port "
						"added to port set");
		} else if(fp->zombie){
			zombies[fp->fd] = np;
		} else {
			live[fp->fd] = np;
		}
	}
	void remove(boost::shared_ptr<AsyncPort> np){
		OpenFileBase* fp = dynamic_cast<OpenFileBase*>(np.get());
		if(fp == NULL){
			throw std::runtime_error("unexpected type of port "
						"removed from port set");
		} else {
			// don't check zombie status, since the port might
			// have changed zombification state in-between.
			// instead just look at both sets
			FD fd = fp->fd;
			std::map<FD, boost::shared_ptr<AsyncPort> >::iterator
				i;
			i = live.find(fd);
			if(i != live.end()){
				live.erase(i);
				return;
			}
			i = zombies.find(fd);
			if(i != zombies.end()){
				zombies.erase(i);
			}
		}
	}
	std::vector<boost::shared_ptr<AsyncPort> > do_poll(int tm){
		dezombify();
		std::vector<struct pollfd> pfds (live.size());
		size_t ii;
		std::map<FD, boost::shared_ptr<AsyncPort> >::iterator i;
		for(i = live.begin(), ii = 0; i != live.end(); ++i, ++ii){
			pfds[ii].fd = i->first;
			pfds[ii].events =
				(i->second->is_input() ? POLLIN : 0) |
				(i->second->is_output() ? POLLOUT : 0) ;
		}
		int rv = poll(&pfds[0], live.size(), tm);
		std::vector<boost::shared_ptr<AsyncPort> > to_return;
		if(rv < 0){
			switch(errno){
			//ignore these errors
			case ENOMEM:
				return to_return;
			default:
				throw ArcError("i/o",
					"file status error");
			}
		} else {
			std::vector<struct pollfd>::iterator pi;
			for(pi = pfds.begin(); pi != pfds.end(); ++pi){
				if(pi->revents != 0){
					i = live.find(pi->fd);
					to_return.push_back(i->second);
					live.erase(i);
				}
			}
			return to_return;
		}
	}
	std::vector<boost::shared_ptr<AsyncPort> > check(void){
		return do_poll(0);
	}
	std::vector<boost::shared_ptr<AsyncPort> > wait(void){
		return do_poll(-1);
	}
};

boost::shared_ptr<AsyncPortSet> NewAsyncPortSet(void){
	return boost::shared_ptr<AsyncPortSet>(
		new AsyncPortSetImpl());
}

