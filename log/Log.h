#pragma once
#include <string>
#include <stdio.h>
#include <string.h>

template<int SIZE>
class LogBuffer 
{
public:
	LogBuffer() 
		:cur_(data_)
	{
	}

	void append(const char* buf, size_t len) 
	{
		if (static_cast<size_t>(avail()) > len)
		{
			memcpy(cur_,buf,len);
			cur_ += len;
		}
	}
	const char* data() const { return data_; }
	int length() const { return static_cast<int>(cur_ - data_); }

	char* current() { return cur_; }
	int avail() const { return static_cast<int>(end() - cur_); }
	void add(size_t len) { cur_ += len; }

	void reset() { cur_ = data_; }


	std::string toString() const { return std::string(data_, length()); }
private:
	const char* end() const { return data_ + sizeof(data_); }
	char data_[SIZE];
	char* cur_;
};

class LogStream
{
public:
	enum Tag {
		FLUSH
	};
	typedef LogStream self;
	typedef LogBuffer<4000> Buffer;
	
	self& operator<<(bool v) 
	{
		buffer_.append(v ? "1" : "0", 1);
		return *this;
	}

	self& operator<<(short);
	self& operator<<(unsigned short);
	self& operator<<(int);
	self& operator<<(unsigned int);
	self& operator<<(long);
	self& operator<<(unsigned long);
	self& operator<<(long long);
	self& operator<<(unsigned long long);

	self& operator<<(const void*);

	self& operator<<(float v)
	{
		*this << static_cast<double>(v);
		return *this;
	}
	self& operator<<(double);

	self& operator<<(char v)
	{
		buffer_.append(&v, 1);
		return *this;
	}
	self& operator<<(const char* str)
	{
		if (str)
		{
			buffer_.append(str, strlen(str));
		}
		else
		{
			buffer_.append("(null)", 6);
		}
		return *this;
	}

	self& operator<<(const unsigned char* str)
	{
		return operator<<(reinterpret_cast<const char*>(str));
	}

	self& operator<<(const std::string& v)
	{
		buffer_.append(v.c_str(), v.size());
		return *this;
	}
	void operator<<(Tag tag) 
	{
		if (tag == Tag::FLUSH)
		{
			this->flush();
		}
	}
	void append(const char* data, int len) { buffer_.append(data, len); }
	const Buffer& buffer() const { return buffer_; }
	void resetBuffer() { buffer_.reset(); }
	void flush();

private:
	template<typename T>
	void formatInteger(T);

	Buffer buffer_;
	static const int kMaxNumericSize = 32;
};

class Logger 
{
public:
	enum LogLevel
	{
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL,
		NUM_LOG_LEVELS,
	};
	class SourceFile 
	{
	public:
		SourceFile() {}
		template<int N>
		inline SourceFile(const char(&arr)[N])
			:data_(arr), size_(N - 1) 
		{
			const char* slash = strrchr(data_, '/'); 
			if (slash)
			{
				data_ = slash + 1;
				size_ -= static_cast<int>(data_ - arr);
			}
		}
		explicit SourceFile(const char* filename)
			: data_(filename)
		{
			const char* slash = strrchr(filename, '/');
			if (slash)
			{
				data_ = slash + 1;
			}
			size_ = static_cast<int>(strlen(data_));
		}
		const char* data_;
		int size_;
	};
	Logger() = default;
	Logger(SourceFile file, int line, LogLevel level, const char* func);
	~Logger();
	
	LogStream& stream() { return stream_; }
private:
	void flush();
	LogStream stream_;
	LogLevel level_;
	int line_;
	SourceFile basename_;
};

#define LOG_TRACE Logger(__FILE__,__LINE__,Logger::TRACE,__FUNCTION__).stream()
#define LOG_DEBUG Logger(__FILE__,__LINE__,Logger::DEBUG,__FUNCTION__).stream()
#define LOG_INFO Logger(__FILE__,__LINE__,Logger::INFO,__FUNCTION__).stream()
#define LOG_WARN Logger(__FILE__,__LINE__,Logger::WARN,__FUNCTION__).stream()
#define LOG_ERROR Logger(__FILE__,__LINE__,Logger::ERROR,__FUNCTION__).stream()
#define LOG_FATAL Logger(__FILE__,__LINE__,Logger::FATAL,__FUNCTION__).stream()
#define LOG_END LogStream::Tag::FLUSH