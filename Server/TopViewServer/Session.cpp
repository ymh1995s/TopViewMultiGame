#include "pch.h"
#include "Session.h"
#include "SessionManager.h"
#include "PacketHandler.h"
#include "Room.h"
#include "Player.h"

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
		GRoom->countPackets.fetch_add(1, std::memory_order_relaxed);

		// 5. Readpos 이동
		recvBuffer.SetReadPos(pktSize);

		// 6. TODO CLEAR
	}
}

void Session::EnterRoom()
{
	if(player != nullptr) return;

	// TODO : 플레이어에서 Start ㅎ마수 따위로 묶기
	player = make_shared<Player>();
	player->SetId();
	player->_type = OBJECT_TYPE_PLAYER;
	player->SetRoom(GRoom);
	player->SetSession(shared_from_this());

	GRoom->EnterObject(player);
}

void Session::Send(const char* msg, int size)
{
	// TODO : SendBuffer는 보낼 때 마다 새로 생성 => 버퍼로 관리
	// .. 아니지 msg를 복사 안하고 바로 보내면 안되나?

	auto self = shared_from_this(); // 핸들러 내부에서 수명 보장용
	string testMSG(msg, size); // TODO :삭제

	socket->async_write_some(boost::asio::buffer(msg, size),
		[self, testMSG](const boost::system::error_code& ec, size_t)
		{
			if (ec) std::cerr << "send err: " << ec.message() << "\n";
			//else cout << "Session " << self->GetSessionId() << " said :" << testMSG << '\n';
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