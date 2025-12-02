#include <SDKDDKVer.h>
#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include "Protocol.pb.h"
#include <memory>
#include <vector>
#include <sstream>
#include <atomic>

#ifdef _DEBUG
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

using boost::asio::ip::tcp;

const char SERVER_IP[] = "127.0.0.1";
const int SERVER_PORT = 7777;

struct PacketHeader
{
    unsigned __int16 size;
    unsigned __int16 id;
};

// Changes: simplify client to only send; no read; single-threaded synchronous loop similar to Program.cs

int main(int argc, char* argv[])
{
    int clientCount = 100; // 1127 메모 : 최적화 전 90명까지 수용
    if (argc >= 2)
    {
        try { clientCount = std::stoi(argv[1]); } catch(...) { clientCount = 10; }
    }

    const int sendIntervalMs = 250; // 1000 = 1초

    // short delay to allow server to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    boost::asio::io_context io_context;
    tcp::endpoint endpoint(boost::asio::ip::make_address(SERVER_IP), SERVER_PORT);

    std::vector<std::shared_ptr<tcp::socket>> sockets;
    sockets.reserve(clientCount);

    // create and connect sockets synchronously
    for (int i = 0; i < clientCount; ++i)
    {
        auto sock = std::make_shared<tcp::socket>(io_context);
        boost::system::error_code ec;
        sock->connect(endpoint, ec);
        if (ec)
        {
            std::cerr << "Client " << i << " connect failed: " << ec.message() << "\n";
        }
        else
        {
            std::cout << "Client " << i << " connected\n";
        }
        sockets.push_back(sock);
    }

    std::cout << "Started " << clientCount << " dummy clients (no read), sending C_CHAT at 100Hz. Press Enter to stop." << std::endl;

    // simple loop: for each tick, send a C_Chat from each client
    std::vector<uint64_t> seqs(clientCount, 0);

    // run until Enter is pressed on stdin; but need non-blocking check. We'll spawn a thread to wait for Enter.
    std::atomic<bool> stop{false};
    std::thread waiter([&stop]() {
        std::string s;
        std::getline(std::cin, s);
        stop = true;
    });

    while (!stop)
    {
        for (int i = 0; i < clientCount; ++i)
        {
            auto& sock = sockets[i];
            if (!sock || !sock->is_open()) continue;

            Protocol::C_Chat chat;
            std::stringstream ss;
            ss << "hello from client " << i << " seq " << seqs[i]++;
            chat.set_message(ss.str());

            uint32_t bodySize = static_cast<uint32_t>(chat.ByteSizeLong());
            const uint32_t totalSize = static_cast<uint32_t>(sizeof(PacketHeader)) + bodySize;

            std::vector<char> sendBuffer(totalSize);
            PacketHeader header;
            header.size = static_cast<unsigned __int16>(totalSize);
            header.id = 1; // C_Chat id
            memcpy(sendBuffer.data(), &header, sizeof(header));

            if (!chat.SerializeToArray(sendBuffer.data() + sizeof(PacketHeader), static_cast<int>(bodySize)))
            {
                std::cerr << "SerializeToArray failed for client " << i << "\n";
                continue;
            }

            boost::system::error_code ec;
            boost::asio::write(*sock, boost::asio::buffer(sendBuffer), ec);
            if (ec)
            {
                std::cerr << "send error client " << i << ": " << ec.message() << "\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sendIntervalMs));
    }

    // cleanup
    for (auto& s : sockets)
    {
        if (s && s->is_open())
        {
            boost::system::error_code ec;
            s->close(ec);
        }
    }

    if (waiter.joinable()) waiter.join();
    return 0;
}