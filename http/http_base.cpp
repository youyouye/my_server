#include "http_base.h"
#include <sstream>

namespace http {

	bool case_insensitive_equal(const std::string &str1, const std::string &str2)
	{
		std::equal(str1.begin(), str1.end(), str2.begin(), [](char a, char b) {
			return tolower(a) == tolower(b);
		});
	}


	std::string PercentTool::Encode(const std::string &value) noexcept
	{
		static auto hex_chars = "0123456789ABCDEF";

		std::string result;
		result.reserve(value.size()); // Minimum size of result

		for (auto &chr : value) {
			if (!((chr >= '0' && chr <= '9') || (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') || chr == '-' || chr == '.' || chr == '_' || chr == '~'))
				result += std::string("%") + hex_chars[static_cast<unsigned char>(chr) >> 4] + hex_chars[static_cast<unsigned char>(chr) & 15];
			else
				result += chr;
		}
		return result;
	}

	std::string PercentTool::Decode(const std::string &value) noexcept
	{
		std::string result;
		result.reserve(value.size() / 3 + (value.size() % 3)); // Minimum size of result

		for (std::size_t i = 0; i < value.size(); ++i) {
			auto &chr = value[i];
			if (chr == '%' && i + 2 < value.size()) {
				auto hex = value.substr(i + 1, 2);
				auto decoded_chr = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
				result += decoded_chr;
				i += 2;
			}
			else if (chr == '+')
				result += ' ';
			else
				result += chr;
		}

		return result;
	}

	std::string QueryString::Create(const CaseInsensitiveMultimap &fields) noexcept
	{
		std::string result;

		bool first = true;
		for (auto &field : fields) {
			result += (!first ? "&" : "") + field.first + '=' + PercentTool::Encode(field.second);
			first = false;
		}

		return result;
	}

	CaseInsensitiveMultimap QueryString::Parse(const std::string &query_string) noexcept
	{
		CaseInsensitiveMultimap result;

		if (query_string.empty())
			return result;

		std::size_t name_pos = 0;
		auto name_end_pos = std::string::npos;
		auto value_pos = std::string::npos;
		for (std::size_t c = 0; c < query_string.size(); ++c) {
			if (query_string[c] == '&') {
				auto name = query_string.substr(name_pos, (name_end_pos == std::string::npos ? c : name_end_pos) - name_pos);
				if (!name.empty()) {
					auto value = value_pos == std::string::npos ? std::string() : query_string.substr(value_pos, c - value_pos);
					result.emplace(std::move(name), PercentTool::Decode(value));
				}
				name_pos = c + 1;
				name_end_pos = std::string::npos;
				value_pos = std::string::npos;
			}
			else if (query_string[c] == '=') {
				name_end_pos = c;
				value_pos = c + 1;
			}
		}
		if (name_pos < query_string.size()) {
			auto name = query_string.substr(name_pos, name_end_pos - name_pos);
			if (!name.empty()) {
				auto value = value_pos >= query_string.size() ? std::string() : query_string.substr(value_pos);
				result.emplace(std::move(name), PercentTool::Decode(value));
			}
		}

		return result;
	}

	CaseInsensitiveMultimap HttpHeader::Parse(const std::string& str) noexcept
	{
		CaseInsensitiveMultimap result;
		auto iter = str.find_first_of("\n");
		auto line = str.substr(0, iter);
		auto content = str.substr(iter + 1, str.size());
		std::size_t param_end;
		while ((param_end = line.find(':')) != std::string::npos)
		{
			std::size_t value_start = param_end + 1;
			while (value_start + 1 < line.size() && line[value_start] == ' ')
				++value_start;
			if (value_start < line.size())
				result.emplace(line.substr(0, param_end), line.substr(value_start, line.size() - value_start - 1));
			iter = content.find_first_of("\n");
			line = content.substr(0, iter);
			content = content.substr(iter + 1, content.size());
		}
		return result;
	}

	bool RequestMessage::Parse(std::shared_ptr<Request> request) noexcept
	{
		request->header_.clear();
		auto content = request->content_;
		auto iter = content.find_first_of("\n");
		auto line = content.substr(0, iter);
		if (auto method_end = line.find(' ') != std::string::npos)
		{
			request->method_ = line.substr(0, method_end);

			std::size_t query_start = std::string::npos;
			std::size_t path_and_query_end = std::string::npos;

			for (size_t i = method_end + 1; i < line.size(); i++)
			{
				if (line[i] == '?' && (i + 1) < line.size())
					query_start = i + 1;
				else if (line[i] == ' ')
				{
					path_and_query_end = i;
					break;
				}
			}
			if (path_and_query_end != std::string::npos)
			{
				if (query_start != std::string::npos)
				{
					request->path_ = line.substr(method_end + 1, query_start - method_end - 2);
					request->query_string_ = line.substr(query_start, path_and_query_end - query_start);
				}
				else
					request->path_ = line.substr(method_end + 1, path_and_query_end - method_end - 1);
				std::size_t protocol_end;
				if ((protocol_end = line.find('/', path_and_query_end + 1)) != std::string::npos)
				{
					if (line.compare(path_and_query_end + 1, protocol_end - path_and_query_end - 1, "HTTP") != 0)
						return false;
					request->http_version_ = line.substr(protocol_end + 1, line.size() - protocol_end - 2);
				}
				else
					return false;
				request->header_ = HttpHeader::Parse(content.substr(iter + 1, content.size()));
			}
			else
				return false;
		}
		else
			return false;
		return false;
	}

	bool ResponseMessage::Parse(const std::string& str, std::string &version, std::string &status_code, CaseInsensitiveMultimap &header) noexcept
	{

	}

	CaseInsensitiveMultimap Request::ParseQueryString()
	{

	}

	void Response::Send()
	{
		std::stringstream ss;
		ss << this->rdbuf();
		session_->conn_->send(ss.str());
	}

	void Response::Write(const char_type *ptr, std::streamsize n)
	{
		std::ostream::write(ptr, n);
	}

	void Response::Write(http::StatusCode status_code, const CaseInsensitiveMultimap &header)
	{
		*this << "HTTP/1.1 " << http::status_code(status_code) << "\r\n";
		WriteHeader(header, 0);
	}

	void Response::Write(http::StatusCode status_code, const std::string &content, const CaseInsensitiveMultimap &header)
	{
		*this << "HTTP/1.1 " << http::status_code(status_code) << "\r\n";
		WriteHeader(header, content.size());
	}

	void Response::Write(http::StatusCode status_code, std::istream &content, const CaseInsensitiveMultimap &header)
	{
		*this << "HTTP/1.1 " << http::status_code(status_code) << "\r\n";
		content.seekg(0, std::ios::end);
		auto size = content.tellg();
		content.seekg(0, std::ios::beg);
		WriteHeader(header, size);
		if (size)
			*this << content.rdbuf();
	}

	void Response::Write(const std::string &content, const CaseInsensitiveMultimap &header)
	{
		Write(http::StatusCode::success_ok, content, header);
	}

	void Response::Write(std::istream &content, const CaseInsensitiveMultimap &header)
	{
		Write(http::StatusCode::success_ok, content, header);
	}

	void Response::Write(const CaseInsensitiveMultimap &header)
	{
		Write(http::StatusCode::success_ok, std::string(), header);
	}

}