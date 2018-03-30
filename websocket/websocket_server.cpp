#include "websocket_server.h"
#include <sstream>
#include "../log/Log.h"

WebsocketServer::WebsocketServer(EventLoop* loop, const std::string& addr, int port)
	:server_(loop,addr,port)
{
	server_.setConnectionCallback(std::bind(&WebsocketServer::onConnection,this,std::placeholders::_1));
	server_.setMessageCallback(std::bind(&WebsocketServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	codec_.setMessageCallback(std::bind(&WebsocketServer::onReceiveMessage,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void WebsocketServer::start()
{
	server_.start();
}

void WebsocketServer::onReceiveMessage(const ConnectionPtr& conn, const std::string& message, Timestamp time)
{
	LOG_INFO << message <<LOG_END;
}

//FIXME:好像server忘记写conn管理了;
void WebsocketServer::onConnection(const ConnectionPtr& conn)
{
	LOG_INFO << "WebsocketServer - "<<conn->name() << " -> "
		<<"is" << ((conn->connected() ? "UP":"DOWN")) << LOG_END;
	if (conn->connected())
	{
		connections_.insert({conn->name(),WebsocketConn(conn,CodecState::StartConnect)});
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

void WebsocketServer::onMessage(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time) 
{
	auto websocket_conn = connections_.find(conn->name());
	switch (websocket_conn->second.codec_result_.state_)
	{
	case CodecState::StartConnect:
		websocket_conn->second.codec_result_ = codec_.onStartConnect(conn, buffer, time);
		break;
	case CodecState::CompleteHandShake:
		websocket_conn->second.codec_result_ = codec_.onCompleteHandshake(conn, buffer, time);
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

void WebsocketServer::sendClose(const ConnectionPtr& conn,int status, const std::string& reason)
{
	std::stringstream ss;
	ss << (status >> 8);
	ss << (status % 256);
	ss << reason;
	send(conn,ss.str(), 136);
}
//129=one fragment,text;130=one fragment,binary;136=close connection.
void WebsocketServer::send(const ConnectionPtr& conn,const std::string& content, unsigned char opcode)
{
	size_t length = content.size();
	std::stringstream header_ss;
	header_ss << opcode;
	if (length >= 126)
	{
		std::size_t num_bytes;
		if (length > 0xffff)
		{
			num_bytes = 8;
			header_ss << 127;
		}
		else 
		{
			num_bytes = 2;
			header_ss << 126;
		}
		for (size_t c = num_bytes - 1; c != static_cast<std::size_t>(-1);c--)
		{
			header_ss << ((static_cast<unsigned long long>(length) >> (8 * c)) % 256);
		}
	}
	else 
	{
		header_ss << static_cast<char>(length);
	}
	conn->send(header_ss.str());
	conn->send(content);
}
