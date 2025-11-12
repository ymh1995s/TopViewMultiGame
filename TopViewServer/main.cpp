#include "pch.h"
#include "ASIO.h"

// main.cpp

using namespace std;

int main()
{
	boost::asio::io_context io_context;
    ASIO asio(io_context, 7777);
	asio.Start();

	io_context.run();

	return 0;
}