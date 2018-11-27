#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_
#include "../server/server.h"
#include "../server/callback.hpp"
#include "http_codec.h"

namespace http {

	class HttpServer : public Server
	{
	public:
		HttpServer(EventLoop* loop, const std::string &addr, int port);
		~HttpServer() = default;

		void start();
		void onReceiveMessage(const ConnectionPtr& conn, const std::string& message, Timestamp time);
		void onSendMessage(const ConnectionPtr& conn, const std::string& message);
	private:
		void onConnection(const ConnectionPtr& conn);
		void onMessage(const ConnectionPtr& conn,
			Buffer* buffer,
			Timestamp time);
		void onGetHttpRequest(std::shared_ptr<Response> response, std::shared_ptr<Request> request);
		void onPostHttpRequest(std::shared_ptr<Response> response, std::shared_ptr<Request> request);
	private:
		Server server_;
		HttpCodec codec_;
		std::unordered_map<std::string, HttpConn> connections_;
	};

}
#endif // !HTTP_SERVER_H_
