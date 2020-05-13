
#ifndef _Reqchannel_H_
#define _Reqchannel_H_

#include "common.h"

class RequestChannel
{
public:
	enum Side {SERVER_SIDE, CLIENT_SIDE};

// each parent object should have its name and side.
// protected means the fields can be accessed by its child instances but not outside the class
protected:
	string my_name;
	Side my_side;
	
public:
	RequestChannel(const string _name, const Side _side)
	{
		my_name = _name;
		my_side = _side;
	}

	// it is virtual since we expect it to run the destructor of the derived classes.
	virtual ~RequestChannel(){}

	// the signatures of cread() and cwrite() should be same for all child classes
	virtual int cread(void* msgbuf, int bufcapacity) = 0;
	virtual int cwrite(void *msgbuf , int msglen) = 0;
	 
	string name() { return my_name; }
};

#endif
