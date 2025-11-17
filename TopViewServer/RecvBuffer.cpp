#include "pch.h"
#include "RecvBuffer.h"

vector<tempPacket> RecvBuffer::attachData(char* data, size_t size)
{
	vector<tempPacket> packets;

	if (writePos + size > sizeof(buffer))
	{
		cout << "RecvBuffer overflow\n";
		return packets;
	}

	// TODO : 복사 없이 할 수 있는 방법 없나?
	memcpy(buffer + writePos, data, size);
	writePos += size;

	// 1. uint32_t가 tempPacket의 패킷 헤더이므로, 최소한 헤더는 도착했는지 확인
	while (writePos - readPos >= sizeof(uint32_t))
	{
		uint32_t bodySize = 0; // 패킷의 바디 크기 
		uint32_t packetSize = 0; // 패킷 전체 크기
		memcpy(&bodySize, buffer + readPos, sizeof(uint32_t));
		packetSize = sizeof(uint32_t) + bodySize;

		// 2. 완전한 패킷이  도착했는지 확인
		if (writePos - readPos >= packetSize)
		{
			// 완전한 패킷 도착
			// TODO : 복사 없이 할 수 있는 방법 없나?
			tempPacket pkt;
			pkt.SetBody(buffer + readPos + sizeof(uint32_t), bodySize);

			packets.emplace_back(pkt);
			readPos += packetSize;
		}
		else
		{
			// 아직 완전한 패킷이 아님
			break;
		}
	}
	if(readPos == writePos)
	{
		Clear();
	}
	else
	{
		// 처리되지 않은 데이터가 있는 경우, 앞으로 당기기
		Clear(writePos - readPos);
	}

	return packets;
}

void RecvBuffer::Clear()
{
	readPos = writePos = 0;
}

void RecvBuffer::Clear(size_t remainedDataSize)
{
	memcpy(buffer, buffer + readPos, remainedDataSize);
	readPos = 0;
	writePos = remainedDataSize;
}