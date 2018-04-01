#include <cstdio>
#include "server/server.h"
#include "server/client.h"
#include "server/event_loop.h"
#include "websocket/websocket_server.h"
#include "websocket/websocket_client.h"

void start_server(const std::string& address) 
{
	EventLoop loop;
	WebsocketServer server(&loop, address, 1935);
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

	std::string type = "2";
	std::string address = "";
	if (argc >= 3)
	{
		type = argv[1];
		address = argv[2];
	}
	if (type == "1")
	{
		start_server(address);
	}
	else 
	{
		start_client(address);
	}
    return 0;
}
