#include "pch.h"
#include "Session.h"

void Session::Start(shared_ptr<tcp::socket> sock)
{
	socket = sock;
	Recv();
}

void Session::Recv()
{
	if (!socket || !socket->is_open()) return;

	auto self = shared_from_this(); // 핸들러 내부에서 수명 보장용
	socket->async_read_some(boost::asio::buffer(tempRecvBuffer, sizeof(tempRecvBuffer)),
		[self](const boost::system::error_code& ec, size_t length)
		{
			if (!ec)
			{
				std::cout << "Session " << self->GetSessionId() << " recv: " << std::string(self->tempRecvBuffer, length) << "\n";

				// 예시: 에코
				boost::asio::async_write(*self->socket, boost::asio::buffer(self->tempRecvBuffer, length),
					[self](const boost::system::error_code& ec, size_t)
					{
						if (ec) std::cerr << "send err: " << ec.message() << "\n";
					});

				// 다음 읽기 예약
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

void Session::Close()
{
	if (socket && socket->is_open())
	{
		boost::system::error_code ec;
		socket->close(ec);
		if (ec) std::cerr << "close err: " << ec.message() << "\n";
	}
}