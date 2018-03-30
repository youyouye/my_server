#ifndef CALLBACK_H_
#define CALLBACK_H_
#include <functional>
#include <memory>
#include "timestamp.h"

class Connection;
class Buffer;

typedef std::shared_ptr<Connection> ConnectionPtr;
typedef std::function<void(const ConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const ConnectionPtr&)> CloseCallback;
typedef std::function<void(const ConnectionPtr&)> WriteCompleteCallback;

typedef std::function<void(const ConnectionPtr&,
		Buffer*,
		Timestamp)> MessageCallback;

#endif
