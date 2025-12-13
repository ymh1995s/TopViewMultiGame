#pragma once
#include "RecvBuffer.h"
using namespace std;
using boost::asio::ip::tcp;

class Player;
class SessionManager;

struct PacketHeader
{
	unsigned __int16 size;
	unsigned __int16 id; // 프로토콜ID (ex. 1=로그인, 2=이동요청)
};

class Session : public enable_shared_from_this<Session>
{
public:
	Session(int sessionId, shared_ptr<SessionManager> sessionManager) : 
		sessionId(sessionId), sessionManager(sessionManager), recvBuffer()
	{

	}
	int GetSessionId() { return sessionId; }

	shared_ptr<Player> GetPlayer() const { return player; } // 외부에서 수정하지 못하게 하기 위해 const 추가 

	void Start(shared_ptr<tcp::socket> socket);
	void Send(const char* msg, int size);
	void Close();
	void RegisterRecv();
	void ProcessRecv(size_t length );

	void EnterRoom();

	// Send 모아 보내기 Test TODO : 주석 삭제///////////////////////
private:
	deque<string> sendQueue;
	atomic<bool> sendQueueProcess = false;
	mutex sendlock;
public:
	void SendQueuePush(const char* msg, int size)
	{
		{
			std::lock_guard<std::mutex> guard(sendlock);
			// TODO : emplace_back으로 수정
			std::string str(msg, size);
			sendQueue.push_back(str);
		}

		bool expected = false;
		if (sendQueueProcess.compare_exchange_strong(expected, true))
		{
			DoSend();
		}
	}

	void DoSend()
	{
		std::string buffer;
		{
			std::lock_guard<std::mutex> guard(sendlock);

			if (sendQueue.empty())
			{
				sendQueueProcess.store(false, std::memory_order_release);
				return;
			}

			for (auto& s : sendQueue) buffer += s;
			sendQueue.clear();
		}

		// TODO : buffer move 복사 
		auto bufferPtr = std::make_shared<std::string>(buffer);
		auto self = shared_from_this();

		boost::asio::async_write(*socket, boost::asio::buffer(*bufferPtr),
			[self, bufferPtr](boost::system::error_code ec, size_t)
			{
				if (!ec)
				{
					// 다음 메시지 전송
					self->DoSend();
				}
				else
				{
					std::cerr << "send err: " << ec.message() << "\n";
					self->Close();
				}			
			});
	}
	//////////////////////////////////////

private:
	// TODO : TLS에서 했던것도 같고..?
	const int sessionId; // 세션 아이디, 세션 매니저에서 부여
	shared_ptr<Player> player;
	char tempRecvBuffer[4096]; // TODO : 삭제

	shared_ptr<tcp::socket> socket; // 네트워크 IO를 위해 여기서 필요
	weak_ptr<SessionManager> sessionManager;

	// 생명주기가 Session과 같으면 굳이 쉐어드 포인터를 쓸 필요가 없다.
	RecvBuffer recvBuffer;
};

