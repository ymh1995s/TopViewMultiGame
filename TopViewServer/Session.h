#pragma once
#include "RecvBuffer.h"
#include "SendBuffer.h"
using namespace std;

class Player;

class Session
{
public:
	Session(int sessionId) : sessionId(sessionId),
		recvBuffer(std::make_unique<RecvBuffer>(40960)),
		sendBuffer(std::make_unique<SendBuffer>(40960))
	{

	}
	int GetSessionId() { return sessionId; }
	void SetSessionId(int id) { sessionId = id; }

	// 외부에서 수정하지 못하게 하기 위해 const 추가 
	shared_ptr<Player> GetPlayer() const { return player; }

	void Send();
	void Close();
	void OnRecv();


private:
	int sessionId; // 세션 아이디, 세션 매니저에서 부여
	shared_ptr<Player> player;
	// 생명주기가 Session과 같으면 굳이 쉐어드 포인터를 쓸 필요가 없다.
	std::unique_ptr<RecvBuffer> recvBuffer;
	std::unique_ptr<SendBuffer> sendBuffer;
};

