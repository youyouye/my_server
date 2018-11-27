#ifndef HTTP_BASE_
#define HTTP_BASE_
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <ostream>
#include <istream>
#include "../server/connection.h"
#include "http_status_code.h"

namespace http {

	bool case_insensitive_equal(const std::string &str1, const std::string &str2);

	class CaseInsensitiveEqual
	{
	public:
		bool operator()(const std::string &str1, const std::string &str2) const noexcept {
			return case_insensitive_equal(str1, str2);
		}
	};

	class CaseInsensitiveHash
	{
	public:
		std::size_t operator()(const std::string &str) const noexcept {
			std::size_t h = 0;
			std::hash<int> hash;
			for (auto c : str)
				h ^= hash(tolower(c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};

	using CaseInsensitiveMultimap = std::unordered_multimap<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;

	class Request;
	class Response;
	class Session;

	class PercentTool
	{
	public:
		static std::string Encode(const std::string &value) noexcept;
		static std::string Decode(const std::string &value) noexcept;
	};

	class QueryString
	{
	public:
		static std::string Create(const CaseInsensitiveMultimap &fields) noexcept;
		static CaseInsensitiveMultimap Parse(const std::string &query_string) noexcept;
	};

	class HttpHeader
	{
	public:
		static CaseInsensitiveMultimap Parse(const std::string& str) noexcept;
	};

	class RequestMessage
	{
	public:
		static bool Parse(std::shared_ptr<Request> request) noexcept;
	};

	class ResponseMessage
	{
	public:
		static bool Parse(const std::string& str, std::string &version, std::string &status_code, CaseInsensitiveMultimap &header) noexcept;
	};

	class Request
	{
	public:
		CaseInsensitiveMultimap ParseQueryString();
	public:
		std::string method_;
		std::string path_;
		std::string query_string_;
		std::string http_version_;
		std::string content_;
		CaseInsensitiveMultimap header_;
		std::string body_;
		std::chrono::system_clock::time_point header_read_time_;
	};

	class Response : public std::enable_shared_from_this<Response>, public std::ostream
	{
	public:
		Response(std::shared_ptr<Session> session)
			:session_(session)
		{
		}

		template<typename size_type>
		void WriteHeader(const CaseInsensitiveMultimap &header, size_type size)
		{
			bool content_length_written = false;
			bool chunked_transfer_encoding = false;
			for (auto &field : header)
			{
				if (!content_length_written && case_insensitive_equal(field.first, "content-length"))
					content_length_written = true;
				else if (!chunked_transfer_encoding && case_insensitive_equal(field.first, "transfer-encoding")
					&& case_insensitive_equal(field.second, "chunked"))
					chunked_transfer_encoding = true;
				*this << field.first << ": " << field.second << "\r\n";
			}
			if (!content_length_written && !chunked_transfer_encoding && !close_connection_after_response)
				*this << "Content-Length: " << size << "\r\n\r\n";
			else
				*this << "\r\n";
		}

		void Send();

		void Write(const char_type *ptr, std::streamsize n);

		void Write(http::StatusCode status_code = http::StatusCode::success_ok, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap());

		void Write(http::StatusCode status_code, const std::string &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap());

		void Write(http::StatusCode status_code, std::istream &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap());

		void Write(const std::string &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap());

		void Write(std::istream &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap());

		void Write(const CaseInsensitiveMultimap &header);
	public:
		std::shared_ptr<Session> session_;
		bool close_connection_after_response = false;
	};

	class Session
	{
	public:
		Session(ConnectionPtr conn)
			:conn_(conn)
		{
			request_ = std::make_shared<Request>();
		}

		ConnectionPtr conn_;
		std::shared_ptr<Request> request_;
		unsigned long long content_length_ = -1;
		unsigned long long trunked_length_ = -1;
	};

}
#endif // !HTTP_BASE_
