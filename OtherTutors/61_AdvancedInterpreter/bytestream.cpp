
#include "bytestream.h"
#include <cstring>

ByteStream::ByteStream()
{
	mysize	= 0;
	mycap	= 0;
	mydata	= nullptr;
}

ByteStream::ByteStream(const ByteStream& other)
{
	mysize	= 0;
	mycap	= 0;
	mydata	= nullptr;

	operator =(other);
}

ByteStream::~ByteStream()
{
	delete[] mydata;
	mydata = nullptr;
}

void ByteStream::Clear()
{
	mysize = 0;
}

void ByteStream::Reserve(size_t newcap)
{
	if (mycap >= newcap)
		return;

	char* newdata = new char[newcap];

	if (mydata != nullptr && mysize > 0)
		memcpy(newdata, mydata, mysize);

	delete[] mydata;

	mydata = newdata;
	mycap = newcap;
}

void ByteStream::Replace(void* what, void* with, size_t size)
{
	size_t start = 0;

	// NOTE: brute force
	while ((start + size) <= mysize) {
		if (memcmp(mydata + start, what, size) == 0) {
			memcpy(mydata + start, with, size);
			start += size;
		} else {
			++start;
		}
	}
}

ByteStream& ByteStream::operator <<(uint8_t value)
{
	if ((mysize + sizeof(uint8_t)) > mycap)
		Reserve(mysize + 1024);

	*((uint8_t*)(mydata + mysize)) = value;
	mysize += sizeof(uint8_t);

	return *this;
}

ByteStream& ByteStream::operator <<(int64_t value)
{
	if ((mysize + sizeof(int64_t)) > mycap)
		Reserve(mysize + 1024);

	*((int64_t*)(mydata + mysize)) = value;
	mysize += sizeof(int64_t);

	return *this;
}

ByteStream& ByteStream::operator <<(const ByteStream& other)
{
	if (other.mydata != nullptr && other.mysize > 0) {
		if ((mysize + other.mysize) > mycap)
			Reserve(mysize + other.mysize + 512);

		memcpy(mydata + mysize, other.mydata, other.mysize);
		mysize += other.mysize;
	}

	return *this;
}

ByteStream& ByteStream::operator =(const ByteStream& other)
{
	if (&other != this) {
		if (mycap < other.mycap)
			Reserve(other.mycap);

		memcpy(mydata, other.mydata, other.mysize);
		mysize = other.mysize;
	}

	return *this;
}
