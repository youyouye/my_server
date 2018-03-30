#include "connector.h"

#include <functional>
#include <assert.h>
#include "event_loop.h"
#include "../log/Log.h"
#include "socket_op.h"
#include "channel.h"

Connector::Connector(EventLoop* loop, const std::string& addr, int port)
	:loop_(loop),
	addr_(addr),
	port_(port),
	connected_(false),
	state_(kDisconnected)
{
}

Connector::~Connector()
{
}

void Connector::start()
{
	connected_ = true;
	std::function<void()> func = std::bind(&Connector::startInLoop,this);
	loop_->runInLoop(func);
}

void Connector::restart()
{
	connected_ = true;

}

void Connector::stop()
{
	connected_ = false;
	std::function<void()> func = std::bind(&Connector::stopInLoop, this);
	loop_->runInLoop(func);
}

void Connector::connect()
{
	int sockfd = sockets::createSocket();
	int ret = sockets::connect(sockfd, addr_, port_);
	int saved_error = (ret == 0) ? 0 : errno;
	switch (saved_error)
	{
	case 0:
	case EINPROGRESS:
	case EINTR:
	case EISCONN:
		connecting(sockfd);
		break;
	case EAGAIN:
	case EADDRINUSE:
	case EADDRNOTAVAIL:
	case ECONNREFUSED:
	case ENETUNREACH:
		retry(sockfd);
		break;
	case EACCES:
	case EPERM:
	case EAFNOSUPPORT:
	case EALREADY:
	case EBADF:
	case EFAULT:
	case ENOTSOCK:
		LOG_ERROR << "connect error in Connector::startInLoop " << saved_error << LOG_END;
		sockets::close(sockfd);
		break;
	default:
		LOG_ERROR << "Unexpected error in Connector::startInLoop" << saved_error << LOG_END;
		sockets::close(sockfd);
		break;
	}
}

void Connector::connecting(int sockfd)
{
	setState(kConnecting);
	assert(!channel_);
	channel_.reset(new Channel(loop_,sockfd));
	channel_->setWriteCallback(std::bind(&Connector::handleWrite,this));
	channel_->setErrorCallback(std::bind(&Connector::handleError,this));
	channel_->enableWriting();
}

void Connector::retry(int sockfd)
{
	sockets::close(sockfd);
	setState(kDisconnected);
	if (connected_)
	{

	}
	else 
	{
		LOG_DEBUG << "do not connect!" << LOG_END;
	}
}

void Connector::startInLoop()
{
	assert(state_ == kDisconnected);
	if (connected_)
	{
		connect();
	}
	else 
	{
		LOG_DEBUG << "not connect!" << LOG_END;
	}
}

void Connector::stopInLoop()
{
	if (state_ == kConnecting)
	{
		setState(kDisconnected);
		int sockfd = removeAndResetChannel();
		//why retry?
	}
}

int Connector::removeAndResetChannel()
{
	channel_->disableAll();
	channel_->remove();
	int sockfd = channel_->fd();
	std::function<void()> func = std::bind(&Connector::resetChannel, this);
	loop_->queueInLoop(func);
	return sockfd;
}

void Connector::resetChannel()
{
	channel_.reset();
}

void Connector::handleWrite()
{
	LOG_DEBUG << "Connector::handleWrite state=" <<state_ << LOG_END;
	if (state_ == kConnecting)
	{
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		if (err)
		{
			LOG_ERROR << "Connector::handleWrite - SO_ERROR ="<<strerror(err) << " " << LOG_END;
		}
//		else if () 
//		{
//			//is-self-connect	
//		}
		else 
		{
			setState(kConnected);
			if (connected_)
			{
				new_connection_callback_(sockfd);
			}
			else 
			{
				sockets::close(sockfd);
			}
		}
	}
	else 
	{
		assert(state_ == kDisconnected);
	}
}

void Connector::handleError()
{
	LOG_ERROR << "Connector::handleError state=" <<state_ << LOG_END;
	if (state_ == kConnecting)
	{
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		LOG_ERROR << "SO_ERROR ="<<strerror(err)<<"  " << LOG_END;
	}
}
