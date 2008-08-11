#ifndef IOPROCESSES_H
#define IOPROCESSES_H

/*derived by the actual implementation*/
class CentralIOProcess : public ProcessBase {
public:
	/*NOTE destructor needs to be declared; it
	is used for cleanup
	*/
	virtual ~CentralIOProcess(){}
};
CentralIOProcess* NewCentralIOProcess(void);

#endif //IOPROCESSES_H
