#include <cstdio>
#include "server/server.h"
#include "server/client.h"
#include "server/event_loop.h"
#include "websocket/websocket_server.h"
#include "websocket/websocket_client.h"
#include "http/http_server.h"

void start_server(const std::string& address) 
{
	EventLoop loop;
	http::HttpServer server(&loop, address, 1900);
	server.start();
	loop.loop();
}

void start_client(const std::string& address)
{
	EventLoop loop;
	WebsocketClient client(&loop, address, 1935);
	client.connect();
	loop.loop();
}

int main(int argc,char* argv[])
{
    printf("hello from my_server!\n");

	start_server("0.0.0.0");
    
	return 0;
}
