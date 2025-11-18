#include "pch.h"
#include "RecvBuffer.h"


// TODO : 삭제하고 아래 attachData() 사용
vector<Protocol::C_Chat> RecvBuffer::attachData(char* data, size_t size)
{
	vector<Protocol::C_Chat> messages;

	// 이전에 남아있던 데이터 + 새로 들어온 데이터 결합 (매 호출 새 버퍼 생성)
	vector<char> combined;
	combined.reserve(buffer.size() + size);
	if (!buffer.empty())
		combined.insert(combined.end(), buffer.begin(), buffer.end());
	if (size > 0)
		combined.insert(combined.end(), data, data + size);

	size_t read = 0;

	// 패킷 파싱
	while (combined.size() - read >= sizeof(uint32_t))
	{
		uint32_t netBodySize = 0;
		memcpy(&netBodySize, combined.data() + read, sizeof(uint32_t));
		uint32_t bodySize = ntohl(netBodySize);

		uint32_t packetSize = static_cast<uint32_t>(sizeof(uint32_t)) + bodySize;

		if (combined.size() - read >= packetSize)
		{
			Protocol::C_Chat chat;
			const char* bodyPtr = combined.data() + read + sizeof(uint32_t);

			if (chat.ParseFromArray(bodyPtr, static_cast<int>(bodySize)))
			{
				messages.emplace_back(std::move(chat));
			}
			else
			{
				std::cerr << "RecvBuffer: C_Chat ParseFromArray failed\n";
				// 파싱 실패 시 정책: 여기서는 루프 종료(추가 조치 필요)
				break;
			}

			read += packetSize;
		}
		else
		{
			break;
		}
	}

	// 처리되지 않은 데이터만 멤버 버퍼로 보관 (최적화 함수 없음)
	if (read < combined.size())
	{
		buffer.assign(combined.begin() + read, combined.end());
	}
	else
	{
		buffer.clear();
	}

	return messages;
}

/*
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
*/

void RecvBuffer::Clear()
{
	readPos = writePos = 0;
}

//void RecvBuffer::Clear(size_t remainedDataSize)
//{
//	memcpy(buffer, buffer + readPos, remainedDataSize);
//	readPos = 0;
//	writePos = remainedDataSize;
//}