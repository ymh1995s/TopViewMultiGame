#pragma once
#include "tempPacket.h"

using namespace std;

class RecvBuffer
{
public:
	vector<tempPacket> attachData(char* data, size_t size);

private:
	void Clear();
	void Clear(size_t remainedDataSize);

private:
	vector<char> buffer; // TODO : 삭제하고 아래 고정 버퍼 사용
	// char buffer[40960];
	size_t readPos = 0;
	size_t writePos = 0;
};

