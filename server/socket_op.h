#ifndef SOCKET_OP_H_
#define SOCKET_OP_H
#include <string>
#include <sys/uio.h>  
#include <arpa/inet.h>

namespace sockets
{
	int createSocket();
	void bind(int sockfd,std::string addr,int port);
	void listen(int sockfd);
	int accept(int sockfd, struct sockaddr_in6* addr);
	int connect(int sockfd, std::string addr, int port);
	ssize_t read(int sockfd,void *buf,size_t count);
	ssize_t readv(int sockfd,const struct iovec *iov,int iovcnt);
	ssize_t write(int sockfd,void *buf,size_t count);
	void close(int sockfd);
	void shutdownWrite(int sockfd);
	int getSocketError(int sockfd);

	const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
	const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
	struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);
	const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
	const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);
}

#endif
