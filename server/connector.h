#ifndef CONNECTOR_H_
#define CONNECTOR_H_
#include <string>
#include <functional>
#include <memory>

class EventLoop;
class Channel;
class Connector 
{
public:
	enum States { kDisconnected, kConnecting, kConnected };

	Connector(EventLoop* loop,const std::string& addr,int port);
	~Connector();
	
	void start();
	void restart();
	void stop();
	void setNewConnectionCallback(const std::function<void(int sockfd)>& callback) { new_connection_callback_ = callback; }
private:
	void setState(States s) { state_ = s; }
	void connect();
	void connecting(int sockfd);
	void retry(int sockfd);
	void startInLoop();
	void stopInLoop();
	int removeAndResetChannel();
	void resetChannel();

	void handleWrite();
	void handleError();
private:
	EventLoop* loop_;
	std::string addr_;
	int port_;
	bool connected_;
	States state_;
	std::shared_ptr<Channel> channel_;
	std::function<void(int sockfd)> new_connection_callback_;
};


#endif // !CONNECTOR_H_
