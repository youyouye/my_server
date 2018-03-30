#ifndef ACCEPTOR_H_
#define ACCEPTOR_H_
#include <functional>
#include "socket.h"
#include "channel.h"

class EventLoop;

class Acceptor 
{
public:
	Acceptor(EventLoop* loop,const std::string& addr,int port);
	~Acceptor();

	void listen();

	void setNewConnCallback(const std::function<void(int fd)>& callback)
	{
		newconn_callback_ = callback;
	}

private:
	void handleRead();

	EventLoop* loop_;
	Socket accept_socket_;	
	Channel accept_channel_;

	std::function<void(int sockfd)> newconn_callback_;
};

#endif
