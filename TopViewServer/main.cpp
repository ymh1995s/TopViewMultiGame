#include "pch.h"
#include "ASIO.h"

// main.cpp

using namespace std;

int main()
{
	// 세션매니저
	shared_ptr<SessionManager> sessionManager = make_shared<SessionManager>();

	// ASIO
	boost::asio::io_context io_context;
    ASIO asio(io_context, 7777, sessionManager);
	asio.Start();

	cout << "Here is Server.. \n";

	io_context.run();

	return 0;
}