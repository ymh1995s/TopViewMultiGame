#include "pch.h"
#include "Session.h"
#include "SessionManager.h"

void Session::Start(shared_ptr<tcp::socket> sock)
{
	socket = sock;
	Recv();
}

void Session::Recv()
{
	if (!socket || !socket->is_open()) return;

	auto self = shared_from_this(); // 핸들러 내부에서 수명 보장용
	// TODO : tempRecvBuffer는 삭제 예정인데 바로 RecvBuffer로 옮기는 방법은?
	socket->async_read_some(boost::asio::buffer(tempRecvBuffer, sizeof(tempRecvBuffer)),
		[self](const boost::system::error_code& ec, size_t length)
		{
			if (!ec)
			{
				//std::cout << "Session " << self->GetSessionId() << " recv: " << std::string(self->tempRecvBuffer, length) << "\n";

				vector<tempPacket> packets = self->recvBuffer.attachData(self->tempRecvBuffer,length);

				cout << "Session " << self->GetSessionId() << " received " << packets.size() << " packets.\n";
				
				// TODO : 삭제, 생성자 - 소비자 패턴으로 변경
				// 네트워크와 처리 로직은 분리되어야 한다.
				for (auto& pkt : packets)
					self->HandlePacket(pkt);


				self->Recv();
			}
			else
			{
				std::cerr << "Session " << self->GetSessionId() << " recv err: " << ec.message() << "\n";
				self->Close();
				// TODO: SessionManager에 RemoveSession(GetSessionId()) 알림 필요
			}
		});
}

void Session::HandlePacket(const tempPacket& pkt)
{
	// TODO  PacketHandler 클래스 생성하고 조건문 대신 Action 형식으로 변경
	cout << "Session " << GetSessionId() << " HandlePacket body size: " << pkt.GetBody() << '\n';
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
			else cout << "Session " << self->GetSessionId() << " said :" << testMSG << '\n';
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