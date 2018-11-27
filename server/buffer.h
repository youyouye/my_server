#ifndef BUFFER_H_
#define BUFFER_H_
#include <vector>
#include <algorithm>
#include <string.h>
#include <assert.h>
#include "endian.h"

//	|		|	readable bytes	|		writable bytes	|
//	0	reader_index_		write_index_		       size
//

class Buffer 
{
public:
	static const size_t kCheapPrepend = 8;
	static const size_t kInitialSize = 1024;
	
	explicit Buffer(size_t initialSize = kInitialSize) 
		: buffer_(kCheapPrepend + initialSize),
		reader_index_(kCheapPrepend),
		writer_index_(kCheapPrepend)
	{
	}

	void swap(Buffer& rhs)
	{
		buffer_.swap(rhs.buffer_);
		std::swap(reader_index_, rhs.reader_index_);
		std::swap(writer_index_, rhs.writer_index_);
	}
	
	size_t readableBytes() const
	{
		return writer_index_ - reader_index_;
	}

	size_t writableBytes() const
	{
		return buffer_.size() - writer_index_;
	}

	size_t prependableBytes() const
	{
		return reader_index_;
	}

	const char* peek() const
	{
		return begin() + reader_index_;
	}
	
	const char* findCRLF() const
	{
		const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 4);
		return crlf == beginWrite() ? NULL : crlf;
	}
	
	const char* findCRLF(const char* start) const
	{
		assert(peek() <= start);
		assert(start <= beginWrite());
		const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 4);
		return crlf == beginWrite() ? NULL : crlf;
	}

	const char* findFragment(std::string fragment) const
	{
		const char* find = std::search(peek(), beginWrite(), fragment.c_str(), fragment.c_str()+fragment.size());
		return find == beginWrite() ? NULL : find;
	}

	const char* findEOL() const
	{
		const void* eol = memchr(peek(), '\n', readableBytes());
		return static_cast<const char*>(eol);
	}

	const char* findEOL(const char* start) const
	{
		assert(peek() <= start);
		assert(start <= beginWrite());
		const void* eol = memchr(start, '\n', beginWrite() - start);
		return static_cast<const char*>(eol);
	}

	void retrieve(size_t len)
	{
		assert(len <= readableBytes());
		if (len < readableBytes())
		{
			reader_index_ += len;
		}
		else
		{
			retrieveAll();
		}
	}

	void retrieveUntil(const char* end)
	{
		assert(peek() <= end);
		assert(end <= beginWrite());
		retrieve(end - peek());
	}

	void retrieveInt64()
	{
		retrieve(sizeof(int64_t));
	}
	void retrieveInt32()
	{
		retrieve(sizeof(int32_t));
	}
	void retrieveInt16()
	{
		retrieve(sizeof(int16_t));
	}
	void retrieveInt8()
	{
		retrieve(sizeof(int8_t));
	}
	void retrieveAll()
	{
		reader_index_ = kCheapPrepend;
		writer_index_ = kCheapPrepend;
	}
	std::string retrieveAllAsString()
	{
		return retrieveAsString(readableBytes());
	}

	std::string retrieveAsString(size_t len)
	{
		assert(len <= readableBytes());
		std::string result(peek(), len);
		retrieve(len);
		return result;
	}

	void append(const char* /*restrict*/ data, size_t len)
	{
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}
	void append(const void* /*restrict*/ data, size_t len)
	{
		append(static_cast<const char*>(data), len);
	}

	void ensureWritableBytes(size_t len)
	{
		if (writableBytes() < len)
		{
			makeSpace(len);
		}
		assert(writableBytes() >= len);
	}

	char* beginWrite()
	{
		return begin() + writer_index_;
	}

	const char* beginWrite() const
	{
		return begin() + writer_index_;
	}

	void hasWritten(size_t len)
	{
		assert(len <= writableBytes());
		writer_index_ += len;
	}

	void unwrite(size_t len)
	{
		assert(len <= readableBytes());
		writer_index_ -= len;
	}

	void appendInt64(int64_t x)
	{
		int64_t be64 = sockets::hostToNetwork64(x);
		append(&be64, sizeof be64);
	}

	void appendInt32(int32_t x)
	{
		int32_t be32 = sockets::hostToNetwork32(x);
		append(&be32, sizeof be32);
	}

	void appendInt16(int16_t x)
	{
		int16_t be16 = sockets::hostToNetwork16(x);
		append(&be16, sizeof be16);
	}

	void appendInt8(int8_t x)
	{
		append(&x, sizeof x);
	}

	int64_t readInt64()
	{
		int64_t result = peekInt64();
		retrieveInt64();
		return result;
	}

	int32_t readInt32()
	{
		int32_t result = peekInt32();
		retrieveInt32();
		return result;
	}
	int16_t readInt16()
	{
		int16_t result = peekInt16();
		retrieveInt16();
		return result;
	}
	int8_t readInt8()
	{
		int8_t result = peekInt8();
		retrieveInt8();
		return result;
	}
	
	std::string readUntil(const std::string& fragment) 
	{
		std::string result(peek(), readableBytes());
		std::string ret;
		size_t found;
		if ((found = result.find(fragment)) != std::string::npos) 
		{
			ret = std::string(peek(),found);
			retrieve(found+fragment.length());
		}
		return ret;
	}
	
	std::vector<char> readBytes(size_t len) 
	{
		if (len <= readableBytes())
		{
			std::vector<char> ret(buffer_.begin()+reader_index_,buffer_.begin()+reader_index_+len);
			reader_index_ += len;
			return ret;
		}
		return std::vector<char>();
	}

	std::string peekAllAsString() const 
	{
		std::string result(peek(), readableBytes());
		return result;
	}

	int64_t peekInt64() const
	{
		assert(readableBytes() >= sizeof(int64_t));
		int64_t be64 = 0;
		::memcpy(&be64, peek(), sizeof be64);
		return sockets::networkToHost64(be64);
	}
	
	int32_t peekInt32() const
	{
		assert(readableBytes() >= sizeof(int32_t));
		int32_t be32 = 0;
		::memcpy(&be32, peek(), sizeof be32);
		return sockets::networkToHost32(be32);
	}

	int16_t peekInt16() const
	{
		assert(readableBytes() >= sizeof(int16_t));
		int16_t be16 = 0;
		::memcpy(&be16, peek(), sizeof be16);
		return sockets::networkToHost16(be16);
	}
	int8_t peekInt8() const
	{
		assert(readableBytes() >= sizeof(int8_t));
		int8_t x = *peek();
		return x;
	}

	void prependInt64(int64_t x)
	{
		int64_t be64 = sockets::hostToNetwork64(x);
		prepend(&be64, sizeof be64);
	}
	void prependInt32(int32_t x)
	{
		int32_t be32 = sockets::hostToNetwork32(x);
		prepend(&be32, sizeof be32);
	}

	void prependInt16(int16_t x)
	{
		int16_t be16 = sockets::hostToNetwork16(x);
		prepend(&be16, sizeof be16);
	}

	void prependInt8(int8_t x)
	{
		prepend(&x, sizeof x);
	}

	void prepend(const void* /*restrict*/ data, size_t len)
	{
		assert(len <= prependableBytes());
		reader_index_ -= len;
		const char* d = static_cast<const char*>(data);
		std::copy(d, d + len, begin() + reader_index_);
	}

	size_t internalCapacity() const
	{
		return buffer_.capacity();
	}

	ssize_t readFd(int fd, int* savedErrno);

private:
	char* begin() 
	{
		return &*buffer_.begin();
	}

	const char* begin() const
	{
		return &*buffer_.begin();
	}

	void makeSpace(size_t len)
	{
		if (writableBytes() + prependableBytes() < len + kCheapPrepend)
		{
			buffer_.resize(writer_index_ + len);
		}
		else
		{
			// move readable data to the front, make space inside buffer
			assert(kCheapPrepend < reader_index_);
			size_t readable = readableBytes();
			std::copy(begin() + reader_index_,
				begin() + writer_index_,
				begin() + kCheapPrepend);
			reader_index_ = kCheapPrepend;
			writer_index_ = reader_index_ + readable;
			assert(readable == readableBytes());
		}
	}

private:
	std::vector<char> buffer_;
	size_t reader_index_;
	size_t writer_index_;

	static const char kCRLF[];
};











#endif
