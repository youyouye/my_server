#include <cstdio>
#include "server/server.h"
#include "server/client.h"
#include "server/event_loop.h"
#include "websocket/websocket_server.h"

void start_server() 
{
	EventLoop loop;
	WebsocketServer server(&loop, "127.0.0.1", 2017);
	server.start();
	loop.loop();
}

void start_client() 
{
	EventLoop loop;
	Client client(&loop, "127.0.0.1", 2017);
	client.connect();
	loop.loop();
}

int main(int argc,char* argv[])
{
    printf("hello from my_server!\n");

	std::string type = "2";
	if (argc >= 2)
	{
		type = argv[1];
	}
	if (type == "1")
	{
		start_server();
	}
	else 
	{
		start_client();
	}
    return 0;
}