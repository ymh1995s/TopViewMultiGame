#include "pch.h"
#include "Session.h"
#include "SessionManager.h"
#include "PacketHandler.h"
#include "Room.h"
#include "Player.h"

#include <google/protobuf/message.h>
using google::protobuf::Message;

void Session::Start(shared_ptr<tcp::socket> sock)
{
	socket = sock;
	EnterRoom();
	RegisterRecv();
}

void Session::RegisterRecv()
{
	if (!socket || !socket->is_open()) return;

	auto self = shared_from_this(); // 핸들러 내부에서 수명 보장용
	// TODO : tempRecvBuffer는 삭제 예정인데 바로 RecvBuffer로 옮기는 방법은?
	socket->async_read_some(boost::asio::buffer(recvBuffer.GetWritePos(), recvBuffer.GetWritableSize()),
		[self](const boost::system::error_code& ec, size_t length)
		{
			if (!ec)
			{
				self->recvBuffer.SetWritePos(length);
				self->ProcessRecv(length);
				self->RegisterRecv();
			}
			else
			{
				std::cerr << "Session " << self->GetSessionId() << " recv err: " << ec.message() << "\n";
				self->Close();
				// TODO: SessionManager에 RemoveSession(GetSessionId()) 알림 필요
			}
		});
}

void Session::ProcessRecv(size_t length )
{
	// TODO : 삭제, 생성자 - 소비자 패턴으로 변경
	// 네트워크와 처리 로직은 분리되어야 한다.

	if (length == 0)
	{
		cout << "Session " << GetSessionId()<<" recv 0 \n";
		Close();
		return;
	}

	while (true)
	{
		// 1. 헤더는 왔는가?
		if (recvBuffer.GetReadableSize() < sizeof(PacketHeader))
			break; 

		// 2. 헤더 정보 확인
		PacketHeader* header = reinterpret_cast<PacketHeader*>(recvBuffer.GetReadPos());
		unsigned __int16 pktSize = header->size;
		unsigned __int16 pktId = header->id;

		// 3. 패킷 전체는 왔는지 확인
		if (recvBuffer.GetReadableSize() < pktSize)
			break;

		// 4. 패킷 처리
		BYTE* pktData = reinterpret_cast<BYTE*>(recvBuffer.GetReadPos()) + sizeof(PacketHeader);

		if (GPacketHandler[pktId])
		{
			GPacketHandler[pktId](shared_from_this(), pktData, pktSize - sizeof(PacketHeader));
		}

		// TEST
		//GRoom->countPackets.fetch_add(1, std::memory_order_relaxed);

		// 5. Readpos 이동
		recvBuffer.SetReadPos(pktSize);

		// 6. CLEAR
		recvBuffer.Clear();
	}
}

void Session::EnterRoom()
{
	if(player != nullptr) return;

	// TODO : 플레이어에서 Start 함수 따위로 묶기
	player = make_shared<Player>();
	player->SetId();
	player->_type = OBJECT_TYPE_PLAYER;
	player->SetRoom(GRoom);
	player->SetSession(shared_from_this());

	GRoom->EnterObject(player);
}

void Session::Send(const vector<shared_ptr<vector<uint8_t>>>& packetList)
{
	// TODO : 패킷의 헤더 크기 계산이 제대로 됐는지 검증 필요.
	shared_ptr<vector<shared_ptr<vector<uint8_t>>>> lpacketList;
	{
		lock_guard<mutex> guard(sendlock);
		lpacketList = make_shared<vector<shared_ptr<vector<uint8_t>>>>(packetList);
	}

	if (lpacketList->size()==0)
		return;

	size_t totalSize = 0;
	for (auto& payload : *lpacketList)
		totalSize += payload->size() + 4;

	auto sendBuffer = make_shared<vector<uint8_t>>(totalSize);
	uint8_t* writePos = sendBuffer->data();

	for (auto& payload : *lpacketList)
	{
		// TODO : SCHAT 하드코딩 삭제하고 올바른 패킷명으로 교체 
		const uint16_t payloadSize = static_cast<uint16_t>(payload->size());
		const uint16_t packetSize = payloadSize + 4;
		const uint16_t packetId = S_CHAT;

		memcpy(writePos, &packetSize, 2); writePos += 2;
		memcpy(writePos, &packetId, 2); writePos += 2;
		memcpy(writePos, payload->data(), payloadSize); 
		writePos += payloadSize;
	}

	auto self = shared_from_this();

	boost::asio::async_write(
		*socket,
		boost::asio::buffer(sendBuffer->data(), sendBuffer->size()),
		[self, sendBuffer, lpacketList]
		(const boost::system::error_code& ec, size_t /*bytes*/)
		{
			if (ec)
			{
				std::cerr << "send err: " << ec.message() << "\n";
			}
		});
}

void Session::Close()
{
	if (socket && socket->is_open())
	{
		boost::system::error_code ec;
		socket->close(ec);
		if (ec) std::cerr << "close err: " << ec.message() << "\n";
	}
}