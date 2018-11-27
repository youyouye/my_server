#ifndef HTTP_CODEC_
#define HTTP_CODEC_
#include <unordered_map>
#include <map>
#include "../server/connection.h"
#include "http_base.h"

namespace http {

	typedef std::function<void(const ConnectionPtr& conn, const std::string& message, Timestamp time)> HttpMessageCallback;
	typedef std::function<void(const ConnectionPtr& conn, const std::string& message)> SendMessageCallback;
	enum CodecState
	{
		StartConnect,
		ReadBody,
		ReadChunkedLength,
		ReadChunkedBody,
		InvalidMessage,
		Error,
	};

	class CodecResult
	{
	public:
		CodecResult(CodecState state, bool continue_flag)
			:state_(state), continue_flag_(continue_flag)
		{
		}
	public:
		CodecState state_;
		bool continue_flag_ = true;
	};

	class HttpCodec
	{
	public:
		HttpCodec();
		~HttpCodec();

		void AddConnection(const ConnectionPtr& conn);
		void DeleteConnection(const ConnectionPtr& conn);
		void setMessageCallback(const HttpMessageCallback& callback) { message_callback_ = callback; }
		void setSendMessageCallback(const SendMessageCallback& callback) { send_callback_ = callback; }

		CodecResult onStartConnect(const ConnectionPtr& conn,
			Buffer* buffer,
			Timestamp time);
		CodecResult onReadBody(const ConnectionPtr& conn,
			Buffer* buffer,
			Timestamp time);
		CodecResult readChunkedTransferLength(const ConnectionPtr& conn,
			Buffer* buffer,
			Timestamp time);
		CodecResult readChunkedTransferBody(const ConnectionPtr& conn,
			Buffer* buffer,
			Timestamp time);
		CodecResult readChunkedFinish(const ConnectionPtr& conn,
			Buffer* buffer,
			Timestamp time, unsigned long long length);
	public:
		std::map<std::string, std::function<void(std::shared_ptr<Response>, std::shared_ptr<Request>)>> default_deal_;
	private:
		bool peekUntil(Buffer* buffer, const std::string& fragment);
		CodecResult dealCallback(std::shared_ptr<Session> session);
		CodecResult Write(std::shared_ptr<Session> session, std::function<void(std::shared_ptr<Response>, std::shared_ptr<Request>)> resource_function);
	private:
		std::unordered_map<std::string, std::shared_ptr<Session>> connections_info_;
		HttpMessageCallback message_callback_;
		SendMessageCallback send_callback_;
	};

	class HttpConn
	{
	public:
		HttpConn(ConnectionPtr conn, CodecResult result) : conn_(conn), codec_result_(result) {}
		ConnectionPtr conn_;
		CodecResult codec_result_;
	};

}

#endif // !HTTP_CODEC_
