#include <SDKDDKVer.h>
#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include "Protocol.pb.h"

using namespace std;

#ifdef _DEBUG
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

using boost::asio::ip::tcp;

const char SERVER_IP[] = "127.0.0.1";
const int SERVER_PORT = 7777;


struct PacketHeader
{
	unsigned __int16 size;
	unsigned __int16 id; // 프로토콜ID (ex. 1=로그인, 2=이동요청)
};

void do_read(tcp::socket& socket)
{
	char buffer[1024]; // 고정 크기 버퍼

	// 2번쨰 인자 시그니쳐 void handler(const boost::system::error_code& ec, std::size_t bytes_transferred);
	socket.async_read_some(boost::asio::buffer(buffer),
		[&](const boost::system::error_code& ec, size_t length)
		{
			if (!ec)
			{
				std::cout << "server said :  " << std::string(buffer, length) << "\n";
				do_read(socket); // 계속 읽기
			}
			else
			{
				cerr << "error : " << ec.message() << "\n";
			}
		});
}

// 시그니처를 맞춰줘야 async_connect 콜백 함수로 사용 가능
void Connected(const boost::system::error_code& ec)
{
	if (!ec) cout << "Server Connected.. Here is Client \n";
	else cout << "Connected failed " << ec.message() << "\n";
}

int main()
{
	// 0. 서버가 켜지는 시간을 위해 1초 대기
	this_thread::sleep_for(chrono::seconds(1));

	// 1. socket과 io_context생성
	// io_context는 IOCP 큐에서 완료된 I/O 이벤트를 꺼내서 처리하는 역할
	boost::asio::io_context io_context;
	tcp::socket socket(io_context);
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(SERVER_IP), 7777);

	// 2. 서버 접속
	// 2번째 인자로 콜백함수 호출한다. 단, 아무 함수를 호출하는 것이 아닌 시그니쳐가 정해져 있다
	// void handler(const boost::system::error_code& ec);
	// IOCP를 사용했다면 ConnectEx() 호출 + 워커 스레드의 GetQueueCompletionPort()로 CP에서 꺼내서 처리 등 복잡하지만 ASIO를 사용함으로써 단축됨
	socket.async_connect(endpoint, Connected);

	// 3. 수신 시작
	do_read(socket);

	// 4. 별도 스레드에서 io_context.run()실행
	// run() ~= GetQueueCompletionPort() + 처리 = CP에서 완료된 이벤트를 꺼내고, 등록된 콜백 호출
	thread ioThread([&]() {
		io_context.run();
		});

	// 5. 메인 스레드는 입력(채팅) 스레드로 사용
	int loopCount = 0; // 전역 또는 main 안에서 유지
	string userInput;
	while (true) {

		/*
		auto msg_ptr = make_shared<string>();
		getline(cin, *msg_ptr);

		// 6. async_write(소켓, 데이터, 콜백함수)
		// 3번째 콜백 시그니처 void handler(const boost::system::error_code& ec, std::size_t bytes_transferred);
		boost::asio::async_write(socket, boost::asio::buffer(*msg_ptr),
			[msg_ptr](const boost::system::error_code& ec, size_t)
			{
				if (ec) cerr << "send err: " << ec.message() << "\n";
			});

		*/

		if (loopCount == 0)
		{
			cout << "Enter message: ";
			getline(cin, userInput);
		}

		loopCount++;

		int packetCount = loopCount; // 1,2,3
		for (int i = 0; i < packetCount; ++i)
		{
			Protocol::C_Chat chat;
			chat.set_message(userInput);

			// 바디 크기 계산
			uint32_t bodySize = static_cast<uint32_t>(chat.ByteSizeLong());

			// 전체 패킷 크기(헤더 + 바디)
			const uint32_t totalSize = static_cast<uint32_t>(sizeof(PacketHeader)) + bodySize;
			if (totalSize > std::numeric_limits<unsigned __int16>::max())
			{
				cerr << "packet too large, skip\n";
				continue;
			}

			// 전송 버퍼(헤더(PacketHeader) + 바디)
			auto sendBuffer = make_shared<vector<char>>(totalSize);

			// PacketHeader 작성
			PacketHeader header;
			header.size = static_cast<unsigned __int16>(totalSize); 
			header.id = 1; 

			// 헤더를 그대로 복사
			memcpy(sendBuffer->data(), &header, sizeof(header));

			// protobuf를 직접 버퍼에 직렬화 (중간 std::string 불필요)
			if (!chat.SerializeToArray(sendBuffer->data() + sizeof(PacketHeader), static_cast<int>(bodySize)))
			{
				cerr << "SerializeToArray failed\n";
				continue;
			}

			// async_write로 송신 (sendBuffer를 캡처해 생명 보장)
			boost::asio::async_write(socket, boost::asio::buffer(*sendBuffer),
				[sendBuffer](const boost::system::error_code& ec, size_t)
				{
					if (ec) cerr << "send err: " << ec.message() << "\n";
				});
		}

		if (loopCount >= 3) loopCount = 0; // 다시 0으로 초기화
	}

	ioThread.join();
	return 0;
}