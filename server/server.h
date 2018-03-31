#ifndef SERVER_H_
#define SERVER_H_
#include <string>
#include <memory>
#include <map>
#include "connection.h"
#include "callback.hpp"

class Acceptor;
class Server
{
public:
	Server()
		:port_(0),next_connid_(0),index_(0)
	{
	}
	Server(EventLoop* loop,const std::string& addr,int port);
	~Server();

	void start();
	void setConnectionCallback(const ConnectionCallback& callback) { connection_callback_ = callback; }
	void setMessageCallback(const MessageCallback& callback) { message_callback_ = callback; }
	void setCompleteCallback(const WriteCompleteCallback& callback) {write_complete_callback_ = callback;}
private:
	void newConnection(int sockfd);
	void removeConnection(const ConnectionPtr& conn);
	void removeConnectionInLoop(const ConnectionPtr& conn);
	
	void defaultConnectionCallback(const ConnectionPtr& conn);
	void defaultMessageCallback(const ConnectionPtr&,
		Buffer*,
		Timestamp);
private:
	EventLoop* loop_;
	const std::string addr_;
	const std::string name_;
	const int port_;

	std::shared_ptr<Acceptor> acceptor_;
	ConnectionCallback connection_callback_;
	MessageCallback message_callback_;
	WriteCompleteCallback write_complete_callback_;

	int next_connid_;
	std::map<std::string,ConnectionPtr> connections_;
	int index_;
};







#endif
