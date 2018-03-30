#ifndef EVENT_LOOP_H_
#define EVENT_LOOP_H_
#include <vector>
#include <memory>
#include <functional>
#include <pthread.h>
#include <mutex>

class Channel;
class Epoll;

class EventLoop
{
public:
	EventLoop();
	~EventLoop();
	
	void loop();
	void quit();	

	void removeChannel(Channel* channel);
	void updateChannel(Channel* channel);

	void runInLoop(std::function<void()>& func);
	bool isInLoopThread() const { return threadId_ == pthread_self(); }

	void queueInLoop(const std::function<void()>& func);
private:
	void doHandleTask();
private:
	const pthread_t threadId_;
	bool loop_;
	bool quit_;
	std::shared_ptr<Epoll> epoller_;
	std::vector<Channel> active_channels_;
	Channel* current_active_channel_;

	std::mutex task_mutex_;
	std::vector<std::function<void()>> handle_tasks_;
};

#endif
