#pragma once

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

	void Clear()
	{
		if(readPos == writePos)
		{
			readPos = writePos = 0;
			return;
		}
		else
		{
			int remainedDataSize = writePos - readPos;
			memcpy(buffer, buffer + readPos, remainedDataSize);
			readPos = 0;
			writePos = remainedDataSize;
		}
	}

private:
	char buffer[40960];
	size_t readPos = 0;
	size_t writePos = 0;
};

