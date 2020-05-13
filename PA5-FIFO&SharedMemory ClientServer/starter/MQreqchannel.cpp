#include "common.h"
#include "MQreqchannel.h"
#include <mqueue.h>
using namespace std;


// constructor has to construct the base class first
MQRequestChannel::MQRequestChannel(const string _name, const Side _side, int _bs) : RequestChannel(_name, _side){
	
	// my_name does not exist in MQreqchannel but still referrable since it is the protected field in the parent class.
	q1 = "/mq_" + my_name + "1";
	q2 = "/mq_" + my_name + "2";
	
	buffersize = _bs;

	if (_side == SERVER_SIDE){
		wfd = open_q(q1, O_WRONLY);
		rfd = open_q(q2, O_RDONLY);
	}
	else{
		rfd = open_q(q1, O_RDONLY);
		wfd = open_q(q2, O_WRONLY);
		
	}
	
}

MQRequestChannel::~MQRequestChannel(){ 
	mq_close(wfd);
	mq_close(rfd);

	mq_unlink(q1.c_str());
	mq_unlink(q2.c_str());
}

// to create message queue
int MQRequestChannel::open_q(string _q_name, int mode){
	struct mq_attr attr {0, 10, buffersize, 0};
    int fd = mq_open(_q_name.c_str(), O_RDWR | O_CREAT, 0600, &attr);
	if (fd < 0){
		EXITONERROR(_q_name);
	}
	return fd;
}

int MQRequestChannel::cread(void* msgbuf, int bufcapacity){
    // rfd is pointing to different queue depending on whether this is a client/server side channel
	return mq_receive(rfd, (char*) msgbuf, buffersize, 0); 
}

int MQRequestChannel::cwrite(void* msgbuf, int len){
    // the last arg is the priority. 
	return mq_send (wfd, (char*) msgbuf, len, 0);
}

