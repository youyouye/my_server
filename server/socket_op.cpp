#include "socket_op.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../log/Log.h"

namespace sockets
{
void setScoketNonBlock(int sockfd)
{
	int flags = ::fcntl(sockfd,F_GETFL,0);
	flags |= O_NONBLOCK;
	int ret = ::fcntl(sockfd,F_SETFL,flags);
	if (ret < 0)
	{
		LOG_ERROR <<"set socket nonblock fail!"<<LOG_END;
	}
	// close-on-exec
	flags = ::fcntl(sockfd, F_GETFD, 0);
	flags |= FD_CLOEXEC;
	ret = ::fcntl(sockfd, F_SETFD, flags);      
	(void)ret;
}

int createSocket()
{
	int sockfd = ::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (sockfd < 0)
	{
		LOG_ERROR <<"create socket error"<< LOG_END;
	}
	setScoketNonBlock(sockfd);
	return sockfd;
}

void bind(int sockfd,std::string addr,int port)
{
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = inet_addr(addr.c_str());
	int ret = ::bind(sockfd,(struct sockaddr *)&sock_addr,static_cast<socklen_t>(sizeof(struct sockaddr)));
	if (ret < 0)
	{
		LOG_ERROR <<"bind error!"<<strerror(errno)<< LOG_END;
	}
}

void listen(int sockfd)
{
	int ret = ::listen(sockfd,SOMAXCONN);
	if(ret < 0)
	{
		LOG_ERROR <<"listen error!"<< LOG_END;
	}
}

int accept(int sockfd,struct sockaddr_in6* addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
	int connfd = ::accept(sockfd,sockaddr_cast(addr),&addrlen);
	setScoketNonBlock(connfd);
	if (connfd < 0)
	{
		int savedErrno = errno;
		switch(savedErrno)
		{
		case EAGAIN:
		case ECONNABORTED:
		case EINTR:
		case EPROTO:
		case EPERM:
		case EMFILE:
			errno = savedErrno;
			break;
		case EBADF:
		case EFAULT:
		case EINVAL:
		case ENFILE:
		case ENOBUFS:
		case ENOMEM:
		case ENOTSOCK:
		case EOPNOTSUPP:
			break;
		default:
			break;
		}
	}
	return connfd;
}

int connect(int sockfd, std::string addr, int port) 
{
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = inet_addr(addr.c_str());
	return ::connect(sockfd, (struct sockaddr *)&sock_addr, static_cast<socklen_t>(sizeof(struct sockaddr)));
}


ssize_t read(int sockfd,void *buf,size_t count)
{
	return ::read(sockfd,buf,count);
}

ssize_t readv(int sockfd,const struct iovec *iov,int iovcnt)
{
	return ::readv(sockfd,iov,iovcnt);
}

ssize_t write(int sockfd,void *buf,size_t count)
{
	return ::write(sockfd,buf,count);
}
void close(int sockfd)
{
	if(::close(sockfd) < 0)
	{
		LOG_INFO <<"close error!"<< LOG_END;
	}
}

void shutdownWrite(int sockfd)
{
	if(::shutdown(sockfd,SHUT_WR) < 0)
	{
		LOG_INFO <<"shutwrite error!"<< LOG_END;
	}
}

int getSocketError(int sockfd)
{
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof optval);

	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
	{
		return errno;
	}
	else
	{
		return optval;
	}
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) 
{
	return static_cast<const struct sockaddr*>((const void*)addr);
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr) 
{
	return static_cast<const struct sockaddr*>((const void*)(addr));
}

struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr) 
{
	return static_cast<struct sockaddr*>((void*)(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) 
{
	return static_cast<const struct sockaddr_in*>((const void*)(addr));
}

const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr) 
{
	return static_cast<const struct sockaddr_in6*>((const void*)(addr));
}

}


