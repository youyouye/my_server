#include "http_codec.h"

namespace http {

	HttpCodec::HttpCodec()
	{
	}

	HttpCodec::~HttpCodec()
	{

	}

	void HttpCodec::AddConnection(const ConnectionPtr& conn)
	{
		connections_info_.insert({ conn->name(),std::make_shared<Session>(conn) });
	}

	void HttpCodec::DeleteConnection(const ConnectionPtr& conn)
	{
		auto it = connections_info_.find(conn->name());
		connections_info_.erase(it);
	}

	CodecResult HttpCodec::onStartConnect(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time)
	{
		auto session = connections_info_[conn->name()];
		if (peekUntil(buffer, "\r\n\r\n"))
		{
			session->request_->header_read_time_ = std::chrono::system_clock::now();
			session->request_->content_ = buffer->readUntil("\r\n\r\n");

			std::size_t num_additional_bytes = buffer->readableBytes() - session->request_->content_.size() - 2;

			if (!RequestMessage::Parse(session->request_))
			{
				//error message,we should shutdown it;
				conn->shutdown();
				return CodecResult(CodecState::InvalidMessage, false);
			}
			buffer->readBytes(4);
			auto header_it = session->request_->header_.find("Content-Length");
			if (header_it != session->request_->header_.end())
			{
				unsigned long long content_length = 0;
				try {
					std::stoul(header_it->second);
				}
				catch (const std::exception &) {
					conn->shutdown();
					return CodecResult(CodecState::InvalidMessage, false);
				}
				if (content_length > num_additional_bytes)
				{
					session->content_length_ = content_length;
					return CodecResult(CodecState::ReadBody, false);
				}
				else
				{
					auto body = buffer->readBytes(content_length);
					session->request_->body_ = std::string(body.begin(), body.end());
					return dealCallback(session);
				}
			}
			else if ((header_it = session->request_->header_.find("Transfer-Encoding")) != session->request_->header_.end() && header_it->second == "chunked")
			{
				return readChunkedTransferLength(conn, buffer, time);
			}
			else
			{
				return dealCallback(session);
			}
		}
		return CodecResult(CodecState::StartConnect, false);
	}

	CodecResult HttpCodec::onReadBody(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time)
	{
		auto session = connections_info_[conn->name()];
		if (buffer->readableBytes() >= session->content_length_)
		{
			auto body = buffer->readBytes(session->content_length_);
			session->request_->body_ = std::string(body.begin(), body.end());
			return dealCallback(session);
		}
		return CodecResult(CodecState::ReadBody, false);
	}

	CodecResult HttpCodec::readChunkedTransferLength(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time)
	{
		auto session = connections_info_[conn->name()];
		if (peekUntil(buffer, "\r\n"))
		{
			auto line = buffer->readUntil("\n");
			buffer->readBytes(1);
			unsigned long length = 0;
			try {
				length = stoul(line, 0, 16);
			}
			catch (...) {
				conn->shutdown();
				return CodecResult(CodecState::InvalidMessage, false);;
			}
			if ((length + 2) > buffer->readableBytes())
			{
				session->trunked_length_ = length + 2;
				return CodecResult(CodecState::ReadChunkedBody, false);
			}
			else
			{
				return readChunkedFinish(conn, buffer, time, length);
			}
		}
		return CodecResult(CodecState::ReadChunkedLength, false);
	}

	CodecResult HttpCodec::readChunkedTransferBody(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time)
	{
		auto session = connections_info_[conn->name()];
		if (buffer->readableBytes() >= session->trunked_length_)
		{
			return readChunkedFinish(conn, buffer, time, session->trunked_length_ - 2);
		}
		return CodecResult(CodecState::ReadChunkedBody, false);
	}

	CodecResult HttpCodec::readChunkedFinish(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time,
		unsigned long long length)
	{
		auto session = connections_info_[conn->name()];
		buffer->readBytes(2);
		if (length > 0)
		{
			auto body = buffer->readBytes(length);
			session->request_->body_ += std::string(body.begin(), body.end());
			return readChunkedTransferLength(conn, buffer, time);
		}
		else
		{
			return dealCallback(session);
		}
	}

	bool HttpCodec::peekUntil(Buffer* buffer, const std::string& fragment)
	{
		if (buffer->findFragment(fragment) != nullptr)
			return true;
		return false;
	}

	CodecResult HttpCodec::dealCallback(std::shared_ptr<Session> session)
	{
		auto it = default_deal_.find(session->request_->method_);
		if (it != default_deal_.end())
			return Write(session, it->second);
		session->conn_->shutdown();
		return CodecResult(CodecState::StartConnect, false);
	}

	CodecResult HttpCodec::Write(std::shared_ptr<Session> session, std::function<void(std::shared_ptr<Response>, std::shared_ptr<Request>)> resource_function)
	{
		auto response = std::make_shared<Response>(session);
		try {
			resource_function(response, session->request_);
		}
		catch (const std::exception &) {
			session->conn_->shutdown();
			return CodecResult(CodecState::Error, false);
		}

		response->Send();

		auto range = response->session_->request_->header_.equal_range("Connection");
		for (auto it = range.first; it != range.second; it++)
		{
			if (case_insensitive_equal(it->second, "close"))
				response->session_->conn_->shutdown();
			else if (case_insensitive_equal(it->second, "keep-alive"))
			{
				auto new_session = std::make_shared<Session>(response->session_->conn_);
				connections_info_[response->session_->conn_->name()] = new_session;
				return CodecResult(CodecState::StartConnect, true);
			}
		}
		if (response->session_->request_->http_version_ >= "1.1")
		{
			auto new_session = std::make_shared<Session>(response->session_->conn_);
			connections_info_[response->session_->conn_->name()] = new_session;
			return CodecResult(CodecState::StartConnect, true);
		}

		response->session_->conn_->shutdown();
		return CodecResult(CodecState::StartConnect, false);
	}
}