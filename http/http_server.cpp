#include "http_server.h"
#include "../log/Log.h"

HttpServer::HttpServer(EventLoop* loop, const std::string &addr, int port)
	:server_(loop, addr, port)
{
	server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
	server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	codec_.setMessageCallback(std::bind(&HttpServer::onReceiveMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	codec_.setSendMessageCallback(std::bind(&HttpServer::onSendMessage, this, std::placeholders::_1, std::placeholders::_2));
		
	codec_.default_deal_["GET"] = std::bind(&HttpServer::onGetHttpRequest,this, std::placeholders::_1,std::placeholders::_2 );
	codec_.default_deal_["POST"] = std::bind(&HttpServer::onPostHttpRequest, this, std::placeholders::_1, std::placeholders::_2);
}

void HttpServer::start()
{
	server_.start();
}

void HttpServer::onReceiveMessage(const ConnectionPtr& conn, const std::string& message, Timestamp time)
{
}

void HttpServer::onSendMessage(const ConnectionPtr& conn, const std::string& message)
{

}

void HttpServer::onConnection(const ConnectionPtr& conn)
{
	LOG_INFO << "HttpServer - " << conn->name() << " -> "
		<< "is" << ((conn->connected() ? "UP" : "DOWN")) << LOG_END;
	if (conn->connected())
	{
		connections_.insert({ conn->name(),HttpConn(conn,CodecResult(CodecState::StartConnect,true)) });
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

void HttpServer::onMessage(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time) 
{
	auto http_conn = connections_.find(conn->name());
	while (http_conn->second.codec_result_.continue_flag_)
	{
		switch (http_conn->second.codec_result_.state_)
		{
		case CodecState::StartConnect:
			http_conn->second.codec_result_ = codec_.onStartConnect(conn, buffer, time);
			break;
		case CodecState::ReadBody:
			http_conn->second.codec_result_ = codec_.onReadBody(conn, buffer, time);
			break;
		case CodecState::ReadChunkedLength:
			http_conn->second.codec_result_ = codec_.readChunkedTransferLength(conn,buffer,time);
			break;
		case CodecState::ReadChunkedBody:
			http_conn->second.codec_result_ = codec_.readChunkedTransferBody(conn,buffer,time);
		case CodecState::Error:
			break;
		case CodecState::InvalidMessage:
			break;
		}
	}
	http_conn->second.codec_result_.continue_flag_ = true;
}

void HttpServer::onGetHttpRequest(std::shared_ptr<Response> response, std::shared_ptr<Request> request)
{
}

void HttpServer::onPostHttpRequest(std::shared_ptr<Response> response, std::shared_ptr<Request> request)
{

}