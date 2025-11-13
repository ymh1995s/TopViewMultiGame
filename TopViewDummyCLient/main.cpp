#include <SDKDDKVer.h>
#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using namespace std;

using boost::asio::ip::tcp;

const char SERVER_IP[] = "127.0.0.1";
const int SERVER_PORT = 7777;

/* 이전 ASIO Chat Server에서 가져옴 */
//struct PacketHeader
//{
//	int size;
//	int id;
//};
//
//struct ChatMessage
//{
//	PacketHeader header;
//	string  body;
//};
//
//class Packet
//{
//public:
//	static vector<char> MakeChatPacket(const string& msg)
//	{
//		PacketHeader header;
//		header.size = sizeof(PacketHeader) + msg.size();
//		header.id = 1; // 1 = 채팅
//
//		vector<char> buffer(header.size);
//		memcpy(buffer.data(), &header, sizeof(PacketHeader));
//		memcpy(buffer.data() + sizeof(PacketHeader), msg.data(), msg.size());
//
//		return buffer;
//	}
//};
////
//
//class ChatClient
//{
//public:
//	ChatClient(boost::asio::io_context& io_context, tcp::endpoint& endpoint)
//		: socket(io_context)
//	{
//		socket.async_connect(endpoint,
//			// this => 값으로 캡쳐 
//			[this](boost::system::error_code ec)
//			{
//				if (!ec)
//				{
//					std::cout << "서버 연결 성공\n";
//					do_read();
//				}
//				else
//				{
//					std::cout << "연결 실패: " << ec.message() << "\n";
//				}
//			});
//	}
//
//	//void send_message(std::shared_ptr<std::vector<char>> packet)
//	//{
//	//	boost::asio::async_write(socket, boost::asio::buffer(*packet),
//	//		[packet](boost::system::error_code ec, std::size_t /*length*/)
//	//		{
//	//			if (ec)
//	//				std::cout << "송신 오류: " << ec.message() << "\n";
//	//		});
//	//}
//
//	// 패킷을 의도적으로 나눠보내 서버에서 패킷이 바로 가는지 확인하기
//	void send_message(std::shared_ptr<vector<char>> packet)
//	{
//		int half_size = packet->size() / 2;
//
//		// 첫 절반 전송
//		boost::asio::async_write(socket, boost::asio::buffer(packet->data(), half_size),
//			[packet](boost::system::error_code ec, size_t /*length*/)
//			{
//				if (ec)
//					cout << "첫 번째 전송 오류: " << ec.message() << "\n";
//			});
//
//		this_thread::sleep_for(chrono::seconds(2));
//
//		// 두 번째 절반 전송
//		boost::asio::async_write(socket, boost::asio::buffer(packet->data() + half_size, packet->size() - half_size),
//			[packet](boost::system::error_code ec, size_t /*length*/)
//			{
//				if (ec)
//					cout << "두 번째 전송 오류: " << ec.message() << "\n";
//			});
//	}
//
//private:
//	void do_read()
//	{
//		socket.async_read_some(boost::asio::buffer(recvBuffer, 1024),
//			[this](boost::system::error_code ec, size_t length)
//			{
//				if (!ec)
//				{
//					cout << "[서버] " << string(recvBuffer, length) << "\n";
//					do_read(); // 계속 읽기 등록
//				}
//				else
//				{
//					cout << "수신 오류: " << ec.message() << "\n";
//				}
//			});
//	}
//
//	tcp::socket socket;
//	char recvBuffer[1024];
//};

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
	while (true) {
		auto msg_ptr = make_shared<string>();
		getline(cin, *msg_ptr);

		// 6. async_write(소켓, 데이터, 콜백함수)
		// 3번째 콜백 시그니처 void handler(const boost::system::error_code& ec, std::size_t bytes_transferred);
		boost::asio::async_write(socket, boost::asio::buffer(*msg_ptr),
			[msg_ptr](const boost::system::error_code& ec, size_t)
			{
				if (ec) cerr << "send err: " << ec.message() << "\n";
			});

		//string msg;
		//getline(cin, msg);

		//boost::asio::async_write(socket, boost::asio::buffer(msg),
		//	[msg](const boost::system::error_code& ec, size_t)
		//	{
		//		if (ec) cerr << "send err: " << ec.message() << "\n";
		//	});
	}

	ioThread.join();
	return 0;
}