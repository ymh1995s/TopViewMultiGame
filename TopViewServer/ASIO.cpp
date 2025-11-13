#include "pch.h"
#include "ASIO.h"
#include "Session.h"

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
				auto session = sessionManager->AddSession();
				if (session)
				{
					// Session이 자체적으로 비동기 IO를 관리
					session->Start(socket);
					cout << "Client Connected, session id=" << session->GetSessionId() << endl;
				}
				else
				{
					cout << "Accept 과정 중 오류 발생 \n" << endl;
				}
			}
			else
			{
				cerr << "Accept error: " << ec.message() << endl;
			}

			// 다음 클라이언트 대기
			DoAccept();
		});
}