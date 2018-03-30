#include "connection.h"
#include "socket.h"
#include "socket_op.h"
#include "channel.h"
#include "event_loop.h"
#include "../log/Log.h"

Connection::Connection(EventLoop* loop, std::string name,int sockfd)
	:loop_(loop),
	socket_(new Socket(sockfd)),
	channel_(new Channel(loop,sockfd)),
	name_(name)
{
	channel_->setReadCallback(std::bind(&Connection::handleRead,this,std::placeholders::_1));
	channel_->setWriteCallback(std::bind(&Connection::handleWrite,this));
	channel_->setCloseCallback(std::bind(&Connection::handleClose,this));
	channel_->setErrorCallback(std::bind(&Connection::handleError,this));
}
	
void Connection::send(const std::string& message)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(message);
		}
		else 
		{
			std::function<void()> func = std::bind(&Connection::sendInLoop, this, message);
			loop_->runInLoop(func);
		}
	}
}

void Connection::send(Buffer* message)
{
}

void Connection::startRead()
{
	std::function<void()> func = std::bind(&Connection::startReadInLoop, this);
	loop_->runInLoop(func);
}

void Connection::stopRead()
{
	std::function<void()> func = std::bind(&Connection::stopReadInLoop, this);
	loop_->runInLoop(func);
}

void Connection::stopReadInLoop()
{
	if (channel_->isReading())
	{
		channel_->disableReading();
	}
}

void Connection::startReadInLoop()
{
	if (!channel_->isReading())
	{
		channel_->enableReading();
	}
}

void Connection::handleRead(Timestamp receive_time)
{
	int savedErrno = 0;
	ssize_t n = input_buffer_.readFd(channel_->fd(),&savedErrno);
	if (n > 0)
	{
		message_callback_(shared_from_this(),&input_buffer_,receive_time);
	}
	else if (n == 0) 
	{
		handleClose();
	}
	else 
	{
		errno = savedErrno;
		handleError();
	}
}

void Connection::handleWrite()
{
	if (channel_->isWriting())
	{
		ssize_t n = sockets::write(channel_->fd(),
			(void*)output_buffer_.peek(),output_buffer_.readableBytes());
		if (n > 0)
		{
			output_buffer_.retrieve(n);
			if (output_buffer_.readableBytes() == 0)
			{
				channel_->disableWriting();
				if (write_complete_callback_)
				{
					std::function<void()> func = std::bind(write_complete_callback_, shared_from_this());
					loop_->queueInLoop(func);
				}
				if (state_ == kDisconnecting)
				{
					shutdownInLoop();
				}
			}
		}
		else 
		{
			//error
			LOG_ERROR << "TcpConnection::handleWrite" << LOG_END;
		}
	}
	else 
	{
		LOG_TRACE << "Connection fd = " << channel_->fd()
			<< " is down, no more writing";
	}
}

void Connection::handleClose()
{
	setState(kDisconnected);
	channel_->disableAll();
	ConnectionPtr guardThis(shared_from_this());
	connection_callback_(guardThis);
	close_callback_(guardThis);
}

void Connection::handleError()
{
	int err = sockets::getSocketError(channel_->fd());
	LOG_ERROR << "TcpConnection::handleError [" << name_
		<< "] - SO_ERROR = " << err << " " << LOG_END;
}

void Connection::sendInLoop(const std::string& message)
{
	ssize_t nwrote = 0;
	ssize_t remaining = message.size();
	bool faultError = false;
	if (state_ == kDisconnected)
	{
		LOG_WARN << "disconnected, give up writing" << LOG_END;
		return;
	}
	if (!channel_->isWriting() && output_buffer_.readableBytes() == 0)
	{
		nwrote = sockets::write(channel_->fd(), (void*)message.c_str(), message.size());
		if (nwrote >= 0)
		{
			remaining = message.size() - nwrote;
			if (remaining == 0 && write_complete_callback_)
			{
				std::function<void()> func = std::bind(write_complete_callback_, shared_from_this());
				loop_->queueInLoop(func);
			}
		}
		else 
		{
			nwrote = 0;
			if (errno != EWOULDBLOCK)
			{
				if (errno == EPIPE || errno == ECONNRESET)	//TODO:?
				{
					faultError = true;
				}
			}
		}
	}

	if (!faultError && remaining > 0)
	{
		size_t oldLen = output_buffer_.readableBytes();
		//TODO:highWater callback
		output_buffer_.append(static_cast<const char*>(message.c_str()) + nwrote, remaining);
		if (!channel_->isWriting())
		{
			channel_->enableWriting();
		}
	}

}

void Connection::shutdown()
{
	if (state_ == kConnected)
	{
		setState(kDisconnecting);
		std::function<void()> func = std::bind(&Connection::shutdownInLoop, this);
		loop_->runInLoop(func);
	}
}

void Connection::shutdownInLoop()
{
	if (!channel_->isWriting())
	{
		socket_->shutdownWrite();
	}
}

void Connection::forceClose()
{
	if (state_ == kConnected || state_ == kDisconnecting)
	{
		setState(kDisconnecting);
		std::function<void()> func = std::bind(&Connection::forceCloseInLoop, shared_from_this());
		loop_->queueInLoop(func);
	}
}

void Connection::forceCloseWithDelay(double seconds)
{
}

void Connection::forceCloseInLoop()
{
	if (state_ == kConnected || state_ == kDisconnecting)
	{
		handleClose();
	}
}

void Connection::connectEstablished()
{
	setState(kConnected);
	channel_->enableReading();
	connection_callback_(shared_from_this());
}

void Connection::connectDestroyed()
{
	if (state_ == kConnected)
	{
		setState(kDisconnected);
		channel_->disableAll();
		connection_callback_(shared_from_this());
	}
	channel_->remove();
}
