#include <SDKDDKVer.h>
// ASIO.h
using namespace std;
using boost::asio::ip::tcp;

class ASIO
{
public:
	ASIO(boost::asio::io_context& io_context, int port)
		: io_context(io_context), port(port),
		acceptor(io_context, tcp::endpoint(tcp::v4(), port))
	{

	}

public:
	void Start();
	int GetPort() { return port; }

private:
	void DoAccept();
	void DoRead(shared_ptr<tcp::socket> socket);

private:
	const int port;
	boost::asio::io_context& io_context;
	tcp::acceptor acceptor;
};

