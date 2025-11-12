#include "pch.h"
#include "ASIO.h"

// ASIO.cpp

void ASIO::Start()
{
	DoAccept();
}

void ASIO::DoAccept()
{
	shared_ptr<tcp::socket> socket = make_shared<tcp::socket>(io_context);

	acceptor.async_accept(*socket,
		[this, socket](const boost::system::error_code& ec)
		{
			if (!ec)
			{
				cout << "Client Connected" << endl;
				DoRead(socket);
			}
			else
			{
				cerr << "Accept error: " << ec.message() << endl;
			}

			// 다음 클라이언트 대기
			DoAccept();
		});
}

void ASIO::DoRead(shared_ptr<tcp::socket> socket)
{
	// tatic char buffer[1024];
	auto buffer = make_shared<vector<char>>(1024); // 각 호출마다 별도 버퍼

	socket->async_read_some(boost::asio::buffer(*buffer),
		// buffer 값으로 캡쳐 이유 : buffer가 스마트 포인터라서 값으로 캡쳐하면 참조 횟수(ref count) 증가.
		// 보통 shared ptr은 값으로 캡쳐로 알면 됨
		[this, buffer, socket](const boost::system::error_code& ec, size_t length)
		{
			if (!ec)
			{
				cout << "recv: " << string(buffer->data(), length) << "\n";

				// 에코
				boost::asio::async_write(*socket, boost::asio::buffer(*buffer, length),
					[buffer](const boost::system::error_code& ec, size_t)
					{
						if (ec) cerr << "send err: " << ec.message() << "\n";
					});

				DoRead(socket); // 재귀 호출
			}
			else
			{
				cerr << "recv err: " << ec.message() << "\n";
			}
		});
}
