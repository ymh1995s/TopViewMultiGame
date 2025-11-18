//#include <SDKDDKVer.h>
#include "SessionManager.h"

// ASIO.h
using namespace std;
using boost::asio::ip::tcp;

class Listner
{
public:
	Listner(boost::asio::io_context& io_context, int port, shared_ptr<SessionManager> sessionManager )
		: io_context(io_context), port(port), sessionManager(sessionManager),
		acceptor(io_context, tcp::endpoint(tcp::v4(), port))
	{

	}

public:
	void Start();
	int GetPort() { return port; }

private:
	void DoAccept();
	// void DoRead(shared_ptr<tcp::socket> socket); // Session이 생기면서 ASIO는 Acce만 담당

private:
	const int port;
	boost::asio::io_context& io_context;
	tcp::acceptor acceptor;
	shared_ptr<SessionManager> sessionManager;
};

