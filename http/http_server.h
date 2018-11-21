#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_
#include "../server/server.h"
#include "../server/callback.hpp"

class HttpServer : public Server 
{
public:
	HttpServer(EventLoop* loop, const std::string &addr, int port);
	~HttpServer() = default;

private:
private:
	Server server_;
};












#endif // !HTTP_SERVER_H_

