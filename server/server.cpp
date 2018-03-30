#include "server.h"
#include "accept.h"
#include "event_loop.h"
#include "../log/Log.h"

Server::Server(EventLoop* loop,const std::string& addr,int port)
	:loop_(loop),
	addr_(addr),
	port_(port),
	acceptor_(new Acceptor(loop,addr,port)),
	index_(1)
{
	acceptor_->setNewConnCallback(std::bind(&Server::newConnection,this,std::placeholders::_1));
	connection_callback_ = std::bind(&Server::defaultConnectionCallback,this, std::placeholders::_1);
	message_callback_ = std::bind(&Server::defaultMessageCallback,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

Server::~Server()
{
	for (auto& item: connections_)
	{
		ConnectionPtr conn(item.second);
		item.second.reset();
		std::function<void()> func = std::bind(&Connection::connectDestroyed, conn);
		conn->getLoop()->runInLoop(func);
	}
}

void Server::start()
{
	std::function<void()> func = std::bind(&Acceptor::listen,acceptor_);
	loop_->runInLoop(func);
}

void Server::newConnection(int sockfd)
{
	char buf[64];
	snprintf(buf,sizeof buf,"-%d#%d",port_,next_connid_);
	std::string conn_name = "server"; conn_name += buf;
	++next_connid_;
	auto conn = std::make_shared<Connection>(loop_,conn_name,sockfd);
	connections_[conn_name] = conn;
	conn->setConnectionCallback(connection_callback_);
	conn->setMessageCallback(message_callback_);
	conn->setWriteCompleteCallback(write_complete_callback_);
	conn->setCloseCallback(std::bind(&Server::removeConnection,this, std::placeholders::_1));
	std::function<void()> func = std::bind(&Connection::connectEstablished, conn);
	loop_->runInLoop(func);
}

void Server::removeConnection(const ConnectionPtr& conn)
{
	std::function<void()> func = std::bind(&Server::removeConnectionInLoop, this, conn);
	loop_->runInLoop(func);
}

void Server::removeConnectionInLoop(const ConnectionPtr& conn)
{
	LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
		<< "] - connection " << conn->name();
	size_t n = connections_.erase(conn->name());
	EventLoop* io_loop = conn->getLoop();
	std::function<void()> func = std::bind(&Connection::connectDestroyed, conn.get());
	io_loop->queueInLoop(func);
}

void Server::defaultConnectionCallback(const ConnectionPtr& conn)
{
	LOG_DEBUG << conn->name() << " -> "
		<< " is "
		<< (conn->connected() ? "UP" : "DOWN") << LOG_END;
}

void Server::defaultMessageCallback(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time)
{
	LOG_INFO << buffer->retrieveAllAsString() << LOG_END;
	conn->send(("No! " + std::to_string(index_++)));
}
