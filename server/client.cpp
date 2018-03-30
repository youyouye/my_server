#include "client.h"
#include <thread>
#include <unistd.h>
#include "buffer.h"
#include "connector.h"
#include "connection.h"
#include "../log/Log.h"

Client::Client(EventLoop * loop, const std::string & addr, int port)
	:loop_(loop), addr_(addr),
	name_("client"), port_(port),
	connector_(new Connector(loop_,addr,port)),
	connect_(true),
	next_connid_(1),
	index_(1)
{
	connection_callback_ = std::bind(&Client::defaultConnectionCallback, this,std::placeholders::_1);
	message_callback_ = std::bind(&Client::defaultMessageCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	connector_->setNewConnectionCallback(std::bind(&Client::newConnection,this,std::placeholders::_1));
}

Client::~Client()
{
	LOG_INFO << "TcpClient::~TcpClient[" << name_ << "] - connector " <<LOG_END;
	if (connection_)
	{
		//later..
	}
}

void Client::connect()
{
	connect_ = true;
	connector_->start();
}

void Client::disconnect()
{
	connect_ = false;
	if (connection_)
	{
		connection_->shutdown();
	}
}

void Client::stop()
{
	connect_ = false;
	connector_->stop();
}

void Client::setConnectionCallback(ConnectionCallback callback)
{
	connection_callback_ = std::move(callback);
}

void Client::newConnection(int sockfd)
{
	char buf[32];
	snprintf(buf, sizeof buf, ":%d#%d", port_, next_connid_);
	++next_connid_;
	std::string conn_name = name_ + buf;
	auto conn = std::make_shared<Connection>(loop_,conn_name,sockfd);
	conn->setConnectionCallback(connection_callback_);
	conn->setMessageCallback(message_callback_);
	conn->setWriteCompleteCallback(write_complete_callback_);
	conn->setCloseCallback(std::bind(&Client::removeConnection,this,std::placeholders::_1));
	connection_ = conn;
	conn->connectEstablished();
}

void Client::removeConnection(const ConnectionPtr& conn)
{
	connector_.reset();
}

void Client::defaultConnectionCallback(const ConnectionPtr& conn)
{
	LOG_INFO << "client conn" << LOG_END;
	connection_->send(("help me ! " + std::to_string(index_++)));
}

void Client::defaultMessageCallback(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time)
{
	LOG_INFO << buffer->retrieveAllAsString() << LOG_END;
	connection_->send(("help me ! " + std::to_string(index_++)));
}
