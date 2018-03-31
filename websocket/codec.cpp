#include "codec.h"
#include "../server/crypto.hpp"
#include "../log/Log.h"
#include <regex>
#include <sstream>

Codec::Codec()
{
}

Codec::~Codec()
{
}

void Codec::AddConnection(const ConnectionPtr& conn)
{
	connections_info_.insert({conn->name(),std::make_shared<ConnectionInfo>()});
}

void Codec::DeleteConnection(const ConnectionPtr& conn)
{
	auto it = connections_info_.find(conn->name());
	connections_info_.erase(it);
}

CodecResult Codec::onStartConnect(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time) 
{
	//TODO:add overtime deal
	if (peekUntil(buffer,"\r\n\r\n"))
	{
		RequestMessage& request = connections_info_[conn->name()]->request_message_;
		if (parseRequest(buffer,request)) 
		{
			//read \r\n
			buffer->readBytes(2);
			std::string hand_shake;
			if (generateHandshake(request.headers, hand_shake))
			{
				conn->send(hand_shake);
				send_callback_(conn,"");
				return CodecResult(CodecState::CompleteHandShake,false);
			}
			else 
			{
				//error message,we should shutdown it;
				conn->shutdown();
				return CodecResult(CodecState::InvalidMessage,false);
			}
		}
		else 
		{
			//error message,we should shutdown it;
			conn->shutdown();
			return CodecResult(CodecState::InvalidMessage,false);
		}
	}
	//FIXME:prevent accept too much unuse message
	return CodecResult(CodecState::StartConnect,false);
}

CodecResult Codec::onCompleteHandshake(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time)
{
	if (buffer->readableBytes() >= 2) 
	{
		auto bytes = buffer->readBytes(2);
		unsigned char fin_rsv_opcode = bytes[0];
		if ((unsigned char)bytes[1] < 128)
		{
			return CodecResult(CodecState::Error,1002,"message from client not masked",false);
		}
		size_t length = ((unsigned char)bytes[1] & 127);
		if (length == 126)
		{
			return CodecResult(CodecState::ReadMessageLength,2,fin_rsv_opcode,buffer->readableBytes()?1:0);
		}
		else if (length == 127) 
		{
			return CodecResult(CodecState::ReadMessageLength, 8, fin_rsv_opcode, buffer->readableBytes() ? 1 : 0);
		}
		else 
		{
			return CodecResult(CodecState::ReadMessageContent,length, fin_rsv_opcode, buffer->readableBytes() ? 1 : 0);
		}
	}
	return CodecResult(CodecState::CompleteHandShake,false);
}

CodecResult Codec::onReadMessageLength(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time,const CodecResult& last_result)
{
	if (last_result.read_length_ == 2 || last_result.read_length_ == 8)
	{
		if (buffer->readableBytes() >= last_result.read_length_)
		{
			size_t length = 0;
			size_t num_bytes = last_result.read_length_;
			auto bytes = buffer->readBytes(num_bytes);
			for (size_t c = 0; c <num_bytes;++c)
			{
				length += static_cast<size_t>(bytes[c]) << (8 * (num_bytes-1-c));
			}
			return CodecResult(CodecState::ReadMessageContent,length,last_result.opcode_, buffer->readableBytes() ? 1 : 0);
		}
		else 
		{
			return CodecResult(CodecState::ReadMessageLength,2,last_result.opcode_,false);
		}
	}
}

CodecResult Codec::onReadMessageContent(const ConnectionPtr& conn,
	Buffer* buffer,
	Timestamp time, const CodecResult& last_result)
{
	//FIXME:length limit and error_code = 1009
	if (buffer->readableBytes() >= 4 + last_result.read_length_)
	{
		auto bytes = buffer->readBytes(4 + last_result.read_length_);
		//mask is first 4 bytes;
		//If fragmented message
		FragmentMessage& fragment_message = connections_info_[conn->name()]->fragment_message_;
		if ((last_result.opcode_ & 0x80) == 0 || (last_result.opcode_ & 0x0f) == 0)
		{
			if (fragment_message.length_ <= 0)
			{
				fragment_message.opcode_ = last_result.opcode_;
				fragment_message.opcode_ |= 0x80;
				fragment_message.length_ = last_result.read_length_;
			}
			else 
			{
				fragment_message.length_ == last_result.read_length_;
			}
		}
		else 
		{
			fragment_message.opcode_ = last_result.opcode_;
			fragment_message.length_ = last_result.read_length_;
		}
		for (size_t c = 0; c < last_result.read_length_;c++)
		{
			fragment_message.content_.push_back((bytes[c + 4] ^ bytes[c % 4]));
		}
		//If connection close
		if ((last_result.opcode_ & 0x0f) == 8)
		{
			int status = 0;
			if (last_result.read_length_ >= 2)
			{
				unsigned char byte1 = fragment_message.content_[0];
				unsigned char byte2 = fragment_message.content_[1];
				status = (static_cast<int>(byte1) << 8) + byte2;
			}
			return CodecResult(CodecState::Error, status, fragment_message.content_,false);
		}
		else if ((last_result.opcode_ & 0x0f) == 9)
		{
			//ping
		}
		else if ((last_result.opcode_ & 0x0f) == 10)
		{
		}
		else if ((last_result.opcode_ & 0x80) == 0) 
		{
			return CodecResult(CodecState::CompleteHandShake,true);
		}
		else 
		{
			message_callback_(conn,fragment_message.content_,time);
			fragment_message.clear();
			return CodecResult(CodecState::CompleteHandShake,true);
		}
	}
	return CodecResult(CodecState::ReadMessageContent, last_result.read_length_, last_result.opcode_,false);
}

bool Codec::peekUntil(Buffer* buffer,const std::string& fragment)
{
	if (buffer->findFragment(fragment) != nullptr)
		return true;
	return false;
}

bool Codec::parseRequest(Buffer* buffer,RequestMessage& request)
{
	std::size_t method_end;
	std::string current_content = buffer->readUntil("\r\n");	//FIXME:also need read \n
	if ((method_end = current_content.find(' ')) != std::string::npos)
	{
		request.method = current_content.substr(0,method_end);
		size_t query_start = std::string::npos,path_and_query_string_end = std::string::npos;
		for (size_t i = method_end+1; i < current_content.size(); ++i)
		{
			if (current_content[i] == '?' && (i + 1) < current_content.size())
				query_start = i + 1;
			else if (current_content[i] == ' ') 
			{
				path_and_query_string_end = i;
				break;
			}
		}
		if (path_and_query_string_end != std::string::npos)
		{
			if (query_start != std::string::npos)
			{
				request.path = current_content.substr(method_end+1,query_start - method_end - 2);
				request.query_string = current_content.substr(query_start,path_and_query_string_end - query_start);
			}
			else 
			{
				request.path = current_content.substr(method_end+1,path_and_query_string_end - method_end - 1);
			}
			size_t protocol_end;
			if ((protocol_end = current_content.find('/',path_and_query_string_end + 1))!=std::string::npos)
			{
				if (current_content.compare(path_and_query_string_end + 1, protocol_end - path_and_query_string_end - 1, "HTTP") != 0)
				{
					return false;
				}
				request.version = current_content.substr(protocol_end + 1,current_content.size() - protocol_end - 1);
			}
			else
				return false;
			request.parseHeaders(buffer);
		}
		else 
			return false;
	}
	else 
		return false;
	return true;
}

void Codec::parseResponse(Buffer* buffer,RequestMessage& request)
{
}

bool Codec::generateHandshake(const HttpHeader& header,std::string& output)
{
	//TODO:can add a http filter
	auto header_it = header.find("Sec-WebSocket-Key");
	if (header_it == header.end())
		return false;
	//test
	std::string key = header_it->second;
	static auto magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	auto sha1 = Crypto::sha1(header_it->second + magic_string);
	std::stringstream ss;
	ss << "HTTP/1.1 101 Web Socket Protocol Handshake\r\n";
	ss << "Upgrade: websocket\r\n";
	ss << "Connection: Upgrade\r\n";
	ss << "Sec-WebSocket-Accept: " << Crypto::base64Encode(sha1) << "\r\n";
	ss << "Sec-WebSocket-Version: 13\r\n";
	ss << "\r\n";
	output = ss.str();
	return true;
}

bool RequestMessage::parseHeaders(Buffer* buffer)
{
	std::string line = buffer->readUntil("\r\n");
	size_t param_end;
	while ((param_end = line.find(':'))!=std::string::npos)
	{
		std::size_t value_start = param_end + 1;
		if (value_start < line.size())
		{
			if (line[value_start] == ' ')
				value_start++;
			if (value_start < line.size())
			{
				headers.emplace(line.substr(0, param_end), line.substr(value_start, line.size() - value_start));
			}
		}
		line = buffer->readUntil("\r\n");
	}
}
