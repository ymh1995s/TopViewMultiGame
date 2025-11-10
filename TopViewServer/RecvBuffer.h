#pragma once
class RecvBuffer
{
	RecvBuffer(int size = 4096) : bufferSize(size), dataSize(0)
	{
		buffer = new char[bufferSize];
	}
private:
	char* buffer;
	int bufferSize;
	int dataSize = 0;
};

