#include "Log.h"
#include <fstream>
#include <ctime>
#include <thread>
#include <algorithm>
#include <iostream>
#include <pthread.h>

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
	"TRACE ",
	"DEBUG ",
	"INFO  ",
	"WARN  ",
	"ERROR ",
	"FATAL ",
};

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
const char digitsHex[] = "0123456789ABCDEF";

template<typename T>
size_t convert(char buf[], T value)
{
	T i = value;
	char* p = buf;

	do
	{
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = zero[lsd];
	} while (i != 0);

	if (value < 0)
	{
		*p++ = '-';
	}
	*p = '\0';
	std::reverse(buf, p);

	return p - buf;
}

size_t convertHex(char buf[], uintptr_t value)
{
	uintptr_t i = value;
	char* p = buf;

	do
	{
		int lsd = static_cast<int>(i % 16);
		i /= 16;
		*p++ = digitsHex[lsd];
	} while (i != 0);

	*p = '\0';
	std::reverse(buf, p);

	return p - buf;
}

template<typename T>
void LogStream::formatInteger(T v)
{
	if (buffer_.avail() >= kMaxNumericSize)
	{
		size_t len = convert(buffer_.current(), v);
		buffer_.add(len);
	}
}

LogStream& LogStream::operator<<(short v)
{
	*this << static_cast<int>(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
	*this << static_cast<unsigned int>(v);
	return *this;
}

LogStream& LogStream::operator<<(int v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(const void* p)
{
	uintptr_t v = reinterpret_cast<uintptr_t>(p);
	if (buffer_.avail() >= kMaxNumericSize)
	{
		char* buf = buffer_.current();
		buf[0] = '0';
		buf[1] = 'x';
		size_t len = convertHex(buf + 2, v);
		buffer_.add(len + 2);
	}
	return *this;
}

LogStream& LogStream::operator<<(double v)
{
	if (buffer_.avail() >= kMaxNumericSize)
	{
		int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
		buffer_.add(len);
	}
	return *this;
}

void LogStream::flush()
{
	/*
	std::ofstream log_file("..//log//log.txt",std::ios::app);
	log_file << buffer_.toString();
	log_file << '\n';
	log_file.close();
	*/
	std::cout<< buffer_.toString() <<std::endl;
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
	:stream_(),
	level_(level),
	line_(line),
	basename_(file)
{
	time_t t = time(0);
	struct tm *now = localtime(&t);
	stream_ << (now->tm_year + 1900) << '-'
		<< (now->tm_mon + 1) << '-'
		<< now->tm_mday << ' '
		<< now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec << "  ";
	stream_ << pthread_self() << "   ";
	stream_ << LogLevelName[level];
	stream_ << func << ' ';
}
Logger::~Logger()
{
	flush();
}

void Logger::flush()
{
	stream_ << "_" << basename_.data_ << ":" << line_ << '\n';
}
