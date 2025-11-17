#pragma once
#include "RecvBuffer.h"
#include "SendBuffer.h"
using namespace std;
using boost::asio::ip::tcp;

class Player;
class SessionManager;

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
	void Recv();

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

