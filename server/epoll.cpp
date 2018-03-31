#include "epoll.h"
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string.h>
#include "channel.h"

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

Epoll::Epoll()
	:epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
	events_(16)
{
	if (epollfd_ < 0)
	{
		std::cout<<"error epoll_create!"<<strerror(errno)<<std::endl;
	}
}

Epoll::~Epoll()
{
	::close(epollfd_);
}

void Epoll::poll(int timeoutMs,std::vector<Channel>* active_channels)
{
	int num_events = ::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
	int saved_error = errno;
	if (num_events > 0)
	{
		activeChannels(num_events,active_channels);
		if (static_cast<size_t>(num_events) == events_.size())
		{
			events_.resize(events_.size()*2);
		}		
	}
	else if (num_events == 0)
	{
//		std::cout <<"nothing happend"<<std::endl;
	}
	else
	{
		if (saved_error != EINTR)
		{
			errno = saved_error;
			std::cout <<"Epoll::poll()"<<strerror(errno)<<std::endl;
		}	
	}
}	

void Epoll::update(int operation,Channel* channel)
{
	struct epoll_event event;
	bzero(&event,sizeof event);
	event.events = channel->events();
	event.data.ptr = channel;
	int fd = channel->fd();
	if (::epoll_ctl(epollfd_,operation,fd,&event) < 0)
	{
		std::cout <<"epoll_ctl error:"<< strerror(errno)<<std::endl;
	}
}

void Epoll::updateChannel(Channel* channel)
{
	int state = channel->state();
	if (state == kNew || state == kDeleted)
	{
		int fd = channel->fd();
		if (state == kNew)
		{
			channels_[fd] = channel;
		}
		else
		{
		}
		channel->setState(kAdded);
		update(EPOLL_CTL_ADD,channel);
	}
	else
	{
		if (channel->isNoneEvent())
		{
			update(EPOLL_CTL_DEL,channel);
			channel->setState(kDeleted);
		}
		else
		{
			update(EPOLL_CTL_MOD,channel);
		}	
	}

}

void Epoll::removeChannel(Channel* channel)
{
	int fd = channel->fd();
	int state = channel->state();
	channels_.erase(fd);
	if (state == kAdded)
	{
		update(EPOLL_CTL_DEL,channel);
	}
	channel->setState(kNew);
}

void Epoll::activeChannels(int num_events,std::vector<Channel>* active_channels)
{
	for (int i = 0;i <num_events;++i)
	{
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		channel->setRevents(events_[i].events);
		active_channels->push_back(*channel);	//FIXME:
	}
}
