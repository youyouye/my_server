#include "accept.h"
#include "socket_op.h"

Acceptor::Acceptor(EventLoop* loop, const std::string& addr, int port)
	:loop_(loop),
	accept_socket_(sockets::createSocket()),
	accept_channel_(loop, accept_socket_.fd())
{
	accept_socket_.bind(addr, port);
	accept_socket_.setReuseAddr(true);
	accept_channel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
	accept_channel_.disableAll();
	accept_channel_.remove();
}

void Acceptor::listen()
{
	accept_socket_.listen();
	accept_channel_.enableReading();
}

void Acceptor::handleRead()
{
	int connfd = accept_socket_.accept();
	if (connfd >= 0)
	{
		if (newconn_callback_) 
		{
			newconn_callback_(connfd);
		}
		else 
		{
			sockets::close(connfd);
		}
	}
	else
	{
		//..
		if (errno == EMFILE)
		{
		}
	}
}




