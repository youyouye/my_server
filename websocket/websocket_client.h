#ifndef WEBSOCKET_CLIENT_H_
#define WEBSOCKET_CLIENT_H_
#include <unordered_map>
#include "codec.h"
#include "../server/client.h"

class WebsocketClient 
{
public:
	WebsocketClient(EventLoop* loop, const std::string& addr, int port);
	~WebsocketClient() = default;

	void connect();
	void onReceiveMessage(const ConnectionPtr& conn, const std::string& message, Timestamp time);
	void onSendMessage(const ConnectionPtr& conn, const std::string& message);
private:
	void onConnection(const ConnectionPtr& conn);
	void onMessage(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time);
private:
	std::string addr_;
	int port_;
	Client client_;
	Codec codec_;
	std::unordered_map<std::string, WebsocketConn> connections_;
};



#endif
