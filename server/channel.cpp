#include "channel.h"
#include <sys/epoll.h>
#include "event_loop.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop,int fd)
	:loop_(loop),
	fd_(fd),
	events_(0),
	revents_(0),
	state_(-1)
{

}

Channel::~Channel()
{
}

void Channel::update()
{
	loop_->updateChannel(this);
}

void Channel::handleEvent()
{
	if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
	{
		if (close_callback_)
			close_callback_();
	}

//	if (revents_ & EPOLLNVAL)
//	{
//	}
	
	if (revents_ & EPOLLERR)
	{
		if (error_callback_)
			error_callback_();
	}
	
	if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
	{
		if (read_callback_)
			read_callback_(Timestamp::now());
	}
	
	if (revents_ & EPOLLOUT)
	{
		if (write_callback_)
			write_callback_();
	}
}

void Channel::remove()
{
	loop_->removeChannel(this);
}










