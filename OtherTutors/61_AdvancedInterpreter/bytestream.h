
#ifndef _BYTESTREAM_H_
#define _BYTESTREAM_H_

#include <cstdint>

class ByteStream
{
private:
	char* mydata;
	size_t mysize;
	size_t mycap;

public:
	ByteStream();
	ByteStream(const ByteStream& other);
	~ByteStream();

	void Clear();
	void Reserve(size_t newcap);
	void Replace(void* what, void* with, size_t size);

	ByteStream& operator <<(uint8_t value);
	ByteStream& operator <<(int64_t value);
	ByteStream& operator <<(const ByteStream& other);

	ByteStream& operator =(const ByteStream& other);

	inline size_t Size() const			{ return mysize; }
	inline char* Data()					{ return mydata; }
	inline char* Seek_Set(size_t off)	{ return (mydata + off); }
	inline char* Seek_End(size_t off)	{ return (mydata + (mysize - off)); }
};

#endif
