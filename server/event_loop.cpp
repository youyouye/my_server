#include "event_loop.h"
#include "channel.h"
#include "epoll.h"

namespace
{
const int kPollTimeMs = 1000;
}

EventLoop::EventLoop()
	:threadId_(pthread_self()),
	loop_(false),
	quit_(false),
	epoller_(std::make_shared<Epoll>()),
	handle_tasks_()
{
}

EventLoop::~EventLoop()
{
}

void EventLoop::loop()
{
	loop_= true;
	quit_ = false;
	
	while(!quit_)
	{
		active_channels_.clear();
		epoller_->poll(kPollTimeMs,&active_channels_);
		
		for (auto it = active_channels_.begin();it != active_channels_.end();++it)
		{
			current_active_channel_ = &(*it);
			current_active_channel_->handleEvent();
		}
		current_active_channel_ = nullptr;
		doHandleTask();
	}
}

void EventLoop::quit()
{
	quit_ = true;
}

void EventLoop::removeChannel(Channel* channel)
{
	epoller_->removeChannel(channel);
}

void EventLoop::updateChannel(Channel* channel)
{
	epoller_->updateChannel(channel);
}

void EventLoop::runInLoop(std::function<void()>& func)
{
	if (isInLoopThread())
	{
		func();
	}
	else 
	{
		queueInLoop(func);
	}
}

void EventLoop::doHandleTask()
{
	std::lock_guard<std::mutex> lock(task_mutex_);
	for (size_t i = 0;i<handle_tasks_.size();++i)
	{
		handle_tasks_[i]();
	}
}

void EventLoop::queueInLoop(const std::function<void()>& func)
{
	std::lock_guard<std::mutex> lock(task_mutex_);
	handle_tasks_.push_back(func);
}
