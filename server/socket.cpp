#include "socket.h"
#include "socket_op.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdio.h>

Socket::~Socket()
{
	sockets::close(sockfd_);
}

void Socket::bind(std::string localaddr,int port)
{
	sockets::bind(sockfd_,localaddr,port);
}

void Socket::listen()
{
	sockets::listen(sockfd_);
}

int Socket::accept()
{
	struct sockaddr_in6 addr;
	bzero(&addr,sizeof addr);
	int connfd = sockets::accept(sockfd_,&addr);
	return connfd;
}

void Socket::shutdownWrite()
{
	sockets::shutdownWrite(sockfd_);
}

void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
		&optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on)
{
}

