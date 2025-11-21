#pragma once
#include "Protocol.pb.h"

using namespace std;

class RecvBuffer
{
public:
	int attachData(char* data, size_t size);

	void* GetWritePos()
	{
		return &buffer[writePos];
	}

	void* GetReadPos()
	{
		return &buffer[readPos];
	}

	void SetWritePos(size_t pos)
	{
		if (writePos + pos > sizeof(buffer))
		{
			cout << "RecvBuffer overflow\n";
			return;
		}
		writePos += pos;
	}

	void SetReadPos(size_t pos)
	{
		if (readPos + pos > writePos)
		{
			cout << "RecvBuffer underflow\n";
			return;
		}
		readPos += pos;
	}

	int GetWritableSize()
	{
		return sizeof(buffer) - writePos;
	}

	int GetReadableSize()
	{
		return writePos - readPos;
	}

private:
	void Clear()
	{
		readPos = writePos = 0;
	}
	void Clear(size_t remainedDataSize)
	{
		memcpy(buffer, buffer + readPos, remainedDataSize);
		readPos = 0;
		writePos = remainedDataSize;
	}

private:
	// TODO 재사용 말고 매번 할당으로 너프시켜보기 
	char buffer[40960];
	size_t readPos = 0;
	size_t writePos = 0;
};

