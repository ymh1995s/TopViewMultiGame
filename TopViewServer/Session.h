#pragma once
#include "RecvBuffer.h"
#include "SendBuffer.h"
using namespace std;
using boost::asio::ip::tcp;

class Player;

class Session : public enable_shared_from_this<Session>
{
public:
	Session(int sessionId) : sessionId(sessionId)
		//recvBuffer(std::make_unique<RecvBuffer>(40960)),
		//sendBuffer(std::make_unique<SendBuffer>(40960))
	{

	}
	int GetSessionId() { return sessionId; }

	shared_ptr<Player> GetPlayer() const { return player; } // 외부에서 수정하지 못하게 하기 위해 const 추가 

	void Start(shared_ptr<tcp::socket> socket);

	void Send();
	void Close();
	void Recv();

private:
	const int sessionId; // 세션 아이디, 세션 매니저에서 부여
	shared_ptr<Player> player;
	char tempSendBuffer[4096];
	char tempRecvBuffer[4096];

	shared_ptr<tcp::socket> socket; // 네트워크 IO를 위해 여기서 필요

	//// 생명주기가 Session과 같으면 굳이 쉐어드 포인터를 쓸 필요가 없다.
	//std::unique_ptr<RecvBuffer> recvBuffer;
	//std::unique_ptr<SendBuffer> sendBuffer;
};

