#ifndef SIGPIPE_H_
#define SIGPIPE_H_
#include <stdio.h>
#include <signal.h>

class IgnoreSigPipe 
{
public:
	IgnoreSigPipe() 
	{
		::signal(SIGPIPE,SIG_IGN);
	}
};

IgnoreSigPipe initObj;

#endif
