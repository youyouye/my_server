#ifndef EPOLL_H_
#define EPOLL_H_
#include <vector>
#include <map>
class Channel;
class Epoll
{
public:
	Epoll();
	~Epoll();
	void poll(int timeoutMs,std::vector<Channel>* active_channels);
	void updateChannel(Channel *channel);
	void removeChannel(Channel *channel);
	void activeChannels(int num,std::vector<Channel>* active_channels);		
	void update(int operation,Channel* channel);
private:
	int epollfd_;
	std::vector<struct epoll_event> events_;
	std::map<int,Channel*> channels_;
};

#endif
