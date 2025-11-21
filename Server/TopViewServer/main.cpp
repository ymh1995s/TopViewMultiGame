#include "pch.h"
#include "Listener.h"

// main.cpp

using namespace std;

int main()
{
	// 세션매니저
	// 실시간 서버 관련 클래스를 모아서 ServerService 클래스로 했던 것도 같고 
	shared_ptr<SessionManager> sessionManager = make_shared<SessionManager>();

	// ASIO
	boost::asio::io_context io_context;
    Listener listener(io_context, 7777, sessionManager);
	listener.Start();

	cout << "Here is Server.. \n";

	io_context.run();

	return 0;
}