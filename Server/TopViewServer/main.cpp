#include "pch.h"
#include "Listener.h"
#include "Room.h"

// main.cpp

using namespace std;

int main()
{
	// 세션매니저
	// 실시간 서버 관련 클래스를 모아서 ServerService 클래스로 했던 것도 같고 
	shared_ptr<SessionManager> sessionManager = make_shared<SessionManager>();

	// 패킷매니저
	PacketHandler::Init();

	// ASIO
	boost::asio::io_context io_context;
    Listener listener(io_context, 7777, sessionManager);
	listener.Start();

    // 단일 Room이기 때문에 RoomManager까지 가지 않는다.
    GRoom->Init(io_context);

    cout << "Here is Server.. \n";

    const int workerCount = std::thread::hardware_concurrency();
    vector<thread> workers;
    workers.reserve(workerCount);
    for (int i = 0; i < workerCount; ++i)
    {
        workers.emplace_back([&io_context]() {
            io_context.run();
            });
    }

    // 1은 메인 스레드
    cout << workerCount + 1 << " workers started.\n";
    io_context.run();

    for (auto& t : workers)
        t.join();

    return 0;
}