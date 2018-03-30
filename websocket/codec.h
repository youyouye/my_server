#ifndef CODEC_H_
#define CODEC_H_
#include <unordered_map>
#include "../server/connection.h"

class HeaderHash;
class HeaderEqual;
typedef std::unordered_multimap<std::string, std::string, HeaderHash, HeaderEqual> HttpHeader;
typedef std::function<void(const ConnectionPtr& conn, const std::string& message, Timestamp time)> WebsocketMessageCallback;

enum CodecState
{
	StartConnect,
	InvalidMessage,
	CompleteHandShake,
	ReadMessageLength,
	ReadMessageContent,
	Error,
};


class HeaderHash 
{
public:
	std::size_t operator()(const std::string &str) const noexcept 
	{
		std::size_t h = 0;
		std::hash<int> hash;
		for (auto c : str)
			h ^= hash(tolower(c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}
};

class HeaderEqual 
{
public:
	bool operator()(const std::string &str1, const std::string &str2) const noexcept 
	{
		return str1.size() == str2.size() &&
			std::equal(str1.begin(), str1.end(), str2.begin(), [](char a, char b) {
			return tolower(a) == tolower(b);
		});
	}

};

class RequestMessage 
{
public:
	bool parseHeaders(Buffer* buffer);
public:
	std::string method;
	std::string path;
	std::string query_string;
	std::string version;
	HttpHeader headers;
};	
	
class FragmentMessage
{
public:
	FragmentMessage() = default;
	FragmentMessage(unsigned char opcode, size_t length)
		:opcode_(opcode), length_(length) {}
	void clear() 
	{
		length_ = 0;
		content_.clear();
	}
	unsigned char opcode_;
	size_t length_;
	std::string content_;
};

class ConnectionInfo 
{
public:
	RequestMessage request_message_;
	FragmentMessage fragment_message_;
};

class CodecResult
{	
public:
	CodecResult(CodecState state) 
		: state_(state),read_length_(0)
	{
	}
	CodecResult(CodecState state, size_t read_length,unsigned char opcode) :
		state_(state), read_length_(read_length),opcode_(opcode)
	{
	}
	CodecResult(CodecState state,int error,const std::string& reason)
		: state_(state), read_length_(0),error_code_(error),error_reason_(reason)
	{
	}

	CodecState state_;
	size_t read_length_;
	unsigned char opcode_;
	int error_code_;
	std::string error_reason_;
};	
	
class Codec 
{
public:
	Codec();
	~Codec();
public:
	void AddConnection(const ConnectionPtr& conn);
	void DeleteConnection(const ConnectionPtr& conn);
	void setMessageCallback(const WebsocketMessageCallback& callback) { message_callback_ = callback; }
	CodecResult onStartConnect(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time);
	CodecResult onCompleteHandshake(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time);
	CodecResult onReadMessageLength(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time, const CodecResult& last_result);
	CodecResult onReadMessageContent(const ConnectionPtr& conn,
		Buffer* buffer,
		Timestamp time, const CodecResult& last_result);
private:
	bool peekUntil(Buffer* buffer,const std::string& fragment);
	bool parseRequest(Buffer* buffer,RequestMessage& request);
	void parseResponse(Buffer* buffer,RequestMessage& request);
	bool generateHandshake(const HttpHeader& header, std::string& output);
private:
	std::unordered_map<std::string, std::shared_ptr<ConnectionInfo>> connections_info_;
	WebsocketMessageCallback message_callback_;
};


#endif