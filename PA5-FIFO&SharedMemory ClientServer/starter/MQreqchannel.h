#ifndef MQreqchannel_H
#define MQreqchannel_H

#include "common.h"
#include "Reqchannel.h"


class MQRequestChannel: public RequestChannel{

private:
    // we need two file discriptors
    int wfd;
    int rfd;

    // they are message queue names.
    string q1, q2;
    int buffersize;
    int open_q(string _q_name, int mode);

public:
    MQRequestChannel(const string _name, const Side _side, int buffersize);

    ~MQRequestChannel();

    int cread(void* msgbuf, int bufcapacity);

    int cwrite(void *msgbuf, int msglen);

};

#endif