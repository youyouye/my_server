#ifndef WEBSOCKET_SERVER_H_
#define WEBSOCKET_SERVER_H_
#include "../server/server.h"
#include "../server/callback.hpp"
#include "codec.h"

class EventLoop;

class WebsocketConn 
{
public:
	WebsocketConn(ConnectionPtr conn, CodecResult result) : conn_(conn), codec_result_(result) {}
	ConnectionPtr conn_;
	CodecResult codec_result_;
};

class WebsocketServer : public Server
{
public:
	WebsocketServer(EventLoop* loop,const std::string& addr,int port);
	~WebsocketServer() = default;

	void start();
	void onReceiveMessage(const ConnectionPtr& conn,const std::string& message,Timestamp time);
	void onSendMessage(const ConnectionPtr& conn, const std::string& message);
private:
	void onConnection(const ConnectionPtr& conn);
	void onMessage(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time);
private:
	void sendClose(const ConnectionPtr& conn,int status,const std::string& reason);
	void send(const ConnectionPtr& conn,const std::string& content,unsigned char opcode = 129);
private:
	Server server_;
	Codec codec_;
	std::unordered_map<std::string, WebsocketConn> connections_;
};

#endif
