#include "pch.h"
#include "Listener.h"

// main.cpp

using namespace std;

int main()
{
	// 세션매니저
	shared_ptr<SessionManager> sessionManager = make_shared<SessionManager>();

	// ASIO
	boost::asio::io_context io_context;
    Listener listener(io_context, 7777, sessionManager);
	listener.Start();

	cout << "Here is Server.. \n";

	io_context.run();

	return 0;
}