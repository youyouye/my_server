#ifndef SOCKET_H_
#define SOCKET_H_
#include <string>
class Socket
{
public:
	Socket(int sockfd):
			sockfd_(sockfd)
	{}
	virtual ~Socket();

	int fd() const {return sockfd_;}
	void bind(std::string localaddr,int port);
	void listen();
	int accept();
	void shutdownWrite();
	void setReuseAddr(bool on);	
	void setReusePort(bool on);
private:
	const int sockfd_;
};

#endif
