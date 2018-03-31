#ifndef CLIENT_H_
#define CLIENT_H_
#include <functional>
#include <string>
#include "callback.hpp"
#include "connector.h"

class EventLoop;
class Buffer;
class Client 
{
public:
	Client(EventLoop* loop, const std::string& addr, int port);
	virtual ~Client();

	void connect();
	void disconnect();
	void stop();

	void setConnectionCallback(ConnectionCallback callback);
private:
	void newConnection(int sockfd);
	void removeConnection(const ConnectionPtr& conn);
	void defaultConnectionCallback(const ConnectionPtr& conn);
	void defaultMessageCallback(const ConnectionPtr&,
		Buffer*,
		Timestamp);
private:
	EventLoop* loop_;
	const std::string addr_;
	const std::string name_;
	const int port_;

	std::shared_ptr<Connector> connector_;

	ConnectionCallback connection_callback_;
	MessageCallback message_callback_;
	WriteCompleteCallback write_complete_callback_;

	bool connect_;
	int next_connid_;
	ConnectionPtr connection_;
	int index_;
};

#endif
