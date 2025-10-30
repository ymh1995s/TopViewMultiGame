#include <SDKDDKVer.h>
#include "pch.h"

using boost::asio::ip::tcp;
using namespace std;

void do_read(tcp::socket& socket)
{
    // tatic char buffer[1024];
    auto buffer = make_shared<vector<char>>(1024); // 각 호출마다 별도 버퍼

    socket.async_read_some(boost::asio::buffer(*buffer),
        // buffer 값으로 캡쳐 이유 : buffer가 스마트 포인터라서 값으로 캡쳐하면 참조 횟수(ref count) 증가.
        // 보통 shared ptr은 값으로 캡쳐로 알면 됨
        [buffer, &socket](const boost::system::error_code& ec, size_t length)
        {
            if (!ec)
            {
                cout << "recv: " << string(buffer->data(), length) << "\n";

                // 에코
                boost::asio::async_write(socket, boost::asio::buffer(*buffer, length),
                    [buffer](const boost::system::error_code& ec, size_t)
                    {
                        if (ec) cerr << "send err: " << ec.message() << "\n";
                    });

                do_read(socket); // 재귀 호출
            }
            else
            {
                cerr << "recv err: " << ec.message() << "\n";
            }
        });
}

int main()
{
	boost::asio::io_context io_context;

    tcp::endpoint endpoint(tcp::v4(), 7777);
    tcp::acceptor acceptor(io_context, endpoint);
    tcp::socket socket(io_context);

    // async_accept 2번째 인자 시그니쳐
    // 신형 void handler( const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket);
    // 여기서 사용된 void handler( const boost::system::error_code& ec);
    // 테스트 이후에는 신형 함수 사용이 권장됨
    acceptor.async_accept(socket,
        [&](const boost::system::error_code& ec)
        {
            if (ec)
            {
                cerr << "accept err " << ec.message() << "\n";
                return;
            }

            cout << "Client Connected \n";
            do_read(socket);
        });

    io_context.run();

	return 0;
}