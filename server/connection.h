#ifndef CONNECTION_H_
#define CONNECTION_H_
#include <memory>
#include "buffer.h"
#include "timestamp.h"
#include "callback.hpp"

class Socket;
class Channel;
class EventLoop;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	Connection() = default;
	Connection(EventLoop* loop,std::string name,int sockfd);
	~Connection() = default;
	
	EventLoop* getLoop() const { return loop_; }
	const std::string& name() { return name_; }
	bool connected() const { return state_ == kConnected; }
	bool disconnected() const { return state_ == kDisconnected; }

	void send(const std::string& message);
	void send(Buffer* message);
	void shutdown();
	void forceClose();
	void forceCloseWithDelay(double seconds);

	void startRead();
	void stopRead();

	void setConnectionCallback(const ConnectionCallback& callback) { connection_callback_ = callback; }
	void setMessageCallback(const MessageCallback& callback) { message_callback_ = callback; }
	void setWriteCompleteCallback(const WriteCompleteCallback& callback) { write_complete_callback_ = callback; }
	void setCloseCallback(const CloseCallback& callback) { close_callback_ = callback; }
	
	Buffer* inputBuffer() { return &input_buffer_; }
	Buffer* outputBuffer() { return &output_buffer_; }

	void connectEstablished();
	void connectDestroyed();

private:
	enum StateE {kDisconnected,kConnectiong,kConnected,kDisconnecting};
	void handleRead(Timestamp receive_time);
	void handleWrite();
	void handleClose();
	void handleError();
	
	void sendInLoop(const std::string& message);
	void shutdownInLoop();
	void forceCloseInLoop();
	void setState(StateE s) { state_ = s; }
	void startReadInLoop();
	void stopReadInLoop();
private:
	EventLoop* loop_;
	
	std::shared_ptr<Socket> socket_;
	std::shared_ptr<Channel> channel_;
	
	std::string name_;
	StateE state_;

	ConnectionCallback connection_callback_;
	MessageCallback message_callback_;
	WriteCompleteCallback write_complete_callback_;
	CloseCallback close_callback_;

	Buffer input_buffer_;
	Buffer output_buffer_;
};	

#endif
