#ifndef CHANNEL_H_
#define CHANNEL_H_
#include <functional>
#include "timestamp.h"

class EventLoop;

class Channel
{
public:
	Channel(EventLoop* loop,int fd);
	~Channel();

	int fd() { return fd_;}
	int events() const {return events_;}
	void setRevents(int event) {revents_ = event;} 
	void setEvents(int event) {events_ = event;}
	void enableReading() { events_ |= kReadEvent; update(); }
	void disableReading() { events_ &= ~kReadEvent; update(); }
	void enableWriting() { events_ |= kWriteEvent; update(); }
	void disableWriting() { events_ &= ~kWriteEvent; update(); }
	void disableAll() { events_ = kNoneEvent; update(); }
	bool isWriting() const { return events_ & kWriteEvent; }
	bool isReading() const { return events_ & kReadEvent; }
	bool isNoneEvent() const {return events_ == kNoneEvent;}
	int state() const {return state_;}
	void setState(int state) {state_ = state;}

	void remove();
public:
	void update();
	void handleEvent();
	void setReadCallback(const std::function<void(Timestamp time)>& callback) { read_callback_ = callback; }
	void setWriteCallback(const std::function<void()>& callback) { write_callback_ = callback; }
	void setCloseCallback(const std::function<void()>& callback) { close_callback_ = callback; }
	void setErrorCallback(const std::function<void()>& callback) { error_callback_ = callback; }
private:
	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	EventLoop* loop_;

	const int fd_;
	int events_;
	int revents_;
	int state_;

	std::function<void(Timestamp time)> read_callback_;
	std::function<void()> write_callback_;
	std::function<void()> close_callback_;
	std::function<void()> error_callback_;
};

#endif
