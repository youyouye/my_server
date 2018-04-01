#include "websocket_client.h"
#include "../log/Log.h"

WebsocketClient::WebsocketClient(EventLoop* loop, const std::string& addr, int port)
	:addr_(addr),port_(port),client_(loop,addr,port)
{
	client_.setConnectionCallback(std::bind(&WebsocketClient::onConnection, this, std::placeholders::_1));
	client_.setMessageCallback(std::bind(&WebsocketClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	codec_.setMessageCallback(std::bind(&WebsocketClient::onReceiveMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	codec_.setSendMessageCallback(std::bind(&WebsocketClient::onSendMessage, this, std::placeholders::_1, std::placeholders::_2));
}

void WebsocketClient::connect()
{
	client_.connect();
}

void WebsocketClient::onReceiveMessage(const ConnectionPtr& conn, const std::string& message, Timestamp time)
{
}

void WebsocketClient::onSendMessage(const ConnectionPtr& conn, const std::string& message)
{
}

void WebsocketClient::onConnection(const ConnectionPtr& conn)
{
	LOG_INFO << "WebsocketClient - " << conn->name() << " -> "
		<< "is" << ((conn->connected() ? "UP" : "DOWN")) << LOG_END;
	if (conn->connected())
	{
		connections_.insert({ conn->name(),WebsocketConn(conn,CodecResult(CodecState::StartConnect,true)) });
		codec_.AddConnection(conn);
	}
	else
	{
		auto it = connections_.begin();
		if ((it = connections_.find(conn->name())) != connections_.end())
		{
			connections_.erase(it);
		}
		codec_.DeleteConnection(conn);
	}
}

void WebsocketClient::onMessage(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time) 
{
	auto websocket_conn = connections_.find(conn->name());
	while (websocket_conn->second.codec_result_.continue_flag_)
	{
		switch (websocket_conn->second.codec_result_.continue_flag_)
		{
		case CodecState::StartConnect:
			websocket_conn->second.codec_result_ = codec_.generateClientHandshake(conn,"/",port_);
			break;
		case CodecState::SendHandShake:
			websocket_conn->second.codec_result_ = codec_.onClientReceiveHandshake(conn, buffer, time);
			break;
		case CodecState::ReadMessageLength:
			websocket_conn->second.codec_result_ = codec_.onReadMessageLength(conn, buffer, time, websocket_conn->second.codec_result_);
			break;
		case CodecState::ReadMessageContent:
			websocket_conn->second.codec_result_ = codec_.onReadMessageContent(conn, buffer, time, websocket_conn->second.codec_result_);
			break;
		case CodecState::Error:
			break;
		case CodecState::InvalidMessage:
			break;
		}
	}
	websocket_conn->second.codec_result_.continue_flag_ = true;
}
