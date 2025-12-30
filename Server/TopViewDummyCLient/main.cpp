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

// Changes: simplify client to only send; add lightweight recv for all clients;
// single-threaded synchronous loop similar to Program.cs
int main(int argc, char* argv[])
{
    // 1127 메모 : 최적화 전 90명까지 수용
    // 1204 메모 : 브로드 캐스트 스레드 분산 후 110명까지 수용
    // 1213 메모 : Send 모아보내기 후 130명까지 수용
    // 1225 메모 : N개의 큐로 분산하여 관리. 성능 향상 없음 - cpu 부하 떄문으로 예상
    // 1225 메모 : move와 emplace_back을 사용한 최적화. CPU 부하 감소, 여전히 130명 최대

    int clientCount = 300;
    if (argc >= 2)
    {
        try { clientCount = std::stoi(argv[1]); }
        catch (...) { clientCount = 10; }
    }

    const int sendIntervalMs = 250; // 1000 = 1초

    // short delay to allow server to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    boost::asio::io_context io_context;
    tcp::endpoint endpoint(
        boost::asio::ip::make_address(SERVER_IP),
        SERVER_PORT
    );

    std::vector<std::shared_ptr<tcp::socket>> sockets;
    sockets.reserve(clientCount);

    std::vector<std::shared_ptr<std::vector<char>>> recvBuffers;
    recvBuffers.reserve(clientCount);

    // create and connect sockets synchronously
    for (int i = 0; i < clientCount; ++i)
    {
        auto sock = std::make_shared<tcp::socket>(io_context);
        boost::system::error_code ec;
        sock->connect(endpoint, ec);

        if (ec)
            std::cerr << "Client " << i << " connect failed: " << ec.message() << "\n";
        else
            std::cout << "Client " << i << " connected\n";

        sockets.push_back(sock);

        // prepare per-client recv buffer
        auto buf = std::make_shared<std::vector<char>>(1024);
        recvBuffers.push_back(buf);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1)); // 입장 완료 후 부하 테스트 시작

    // start recv for all clients with minimal handler
    for (int i = 0; i < clientCount; ++i)
    {
        auto sock = sockets[i];
        auto recvBuffer = recvBuffers[i];

        auto startRecv = std::make_shared<std::function<void()>>();
        *startRecv = [sock, recvBuffer, startRecv]()
            {
                sock->async_read_some(
                    boost::asio::buffer(*recvBuffer),
                    [sock, recvBuffer, startRecv](
                        const boost::system::error_code& ec,
                        std::size_t /*bytes_transferred*/)
                    {
                        if (!ec)
                        {
                            (*startRecv)(); // 다음 recv 예약
                        }
                    }
                );
            };

        (*startRecv)(); // 최초 호출
    }

    // 이벤트 루프 실행
    // 기존
    // std::thread ioThread([&io_context]()
    // {
    //     io_context.run();
    // });

    // -> 멀티스레드 io_context 실행
    std::vector<std::thread> ioThreads;
    for (int t = 0; t < 16; ++t)
    {
        ioThreads.emplace_back([&io_context]() {
            io_context.run();
            });
    }

    std::cout
        << "Started " << clientCount
        << " dummy clients (light recv), sending C_CHAT at 100Hz. Press Enter to stop."
        << std::endl;

    std::vector<uint64_t> seqs(clientCount, 0);

    std::atomic<bool> stop{ false };
    std::thread waiter([&stop]()
        {
            std::string s;
            std::getline(std::cin, s);
            stop = true;
        });

    // Reusable packet and buffers to avoid per-iteration allocations
    Protocol::C_Chat chat;                  // 재사용할 프로토버프 패킷
    PacketHeader header;                     // 재사용할 헤더
    header.id = 1; // C_Chat id

    // 메시지 문자열 재사용을 위해 외부에 선언
    std::string message;
    message.reserve(128); // 예상 최대 크기 넉넉히 예약

    // 최대 바디 크기를 보수적으로 잡아 한번만 할당 후 재사용
    // 헤더(4바이트) + 메시지(예상 최대 256바이트)
    const size_t kMaxBodySize = 256;
    std::vector<char> sendBuffer(sizeof(PacketHeader) + kMaxBodySize);

    // 재사용할 에러 코드 객체
    boost::system::error_code ec;

    while (!stop)
    {
        for (int i = 0; i < clientCount; ++i)
        {
            auto& sock = sockets[i];
            if (!sock || !sock->is_open())
                continue;

            // 메시지 구성 (ostringstream 제거, 문자열 append로 최적화)
            message.clear();
            message.append("hello from client ");
            message.append(std::to_string(i));
            message.append(" seq ");
            message.append(std::to_string(seqs[i]++));
            chat.set_message(message);

            // 현재 바디 크기 계산
            const uint32_t bodySize = static_cast<uint32_t>(chat.ByteSizeLong());
            const uint32_t totalSize = static_cast<uint32_t>(sizeof(PacketHeader)) + bodySize;

            // sendBuffer가 충분히 크지 않으면 확장 (필요 시에만)
            if (sendBuffer.size() < totalSize)
                sendBuffer.resize(totalSize);

            // 헤더 설정 후 버퍼에 복사
            header.size = static_cast<unsigned __int16>(totalSize);
            std::memcpy(sendBuffer.data(), &header, sizeof(header));

            // 바디 직렬화 (고정된 버퍼 내)
            if (!chat.SerializeToArray(
                sendBuffer.data() + sizeof(PacketHeader),
                static_cast<int>(bodySize)))
            {
                std::cerr << "SerializeToArray failed for client " << i << "\n";
                continue;
            }

            // 동일 버퍼를 재사용하여 송신
            ec.clear();
            boost::asio::write(*sock, boost::asio::buffer(sendBuffer.data(), totalSize), ec);

            if (ec)
                std::cerr << "send error client " << i << ": " << ec.message() << "\n";
        }

        // 타이머 대기 (고정된 인터벌)
        std::this_thread::sleep_for(std::chrono::milliseconds(sendIntervalMs));
    }

    // cleanup
    for (auto& s : sockets)
    {
        if (s && s->is_open())
        {
            boost::system::error_code closeEc;
            s->close(closeEc);
        }
    }

    if (waiter.joinable()) waiter.join();
    //if (ioThread.joinable()) ioThread.join();
    for (auto& th : ioThreads)
    {
        if (th.joinable()) th.join();
    }


    return 0;
}