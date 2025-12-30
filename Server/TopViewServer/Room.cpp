#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Projectile.h"
#include "Dust.h"
#include "Obstacle.h"
#include "Job.h"

#ifdef _WIN32
#include <windows.h>
#endif

void Room::Init(boost::asio::io_context& io)
{
	_io = &io;
	cout << "Room Init\n";
	CreateObstacle();
	CreateDust();
	InitObjectTable();

	//t1 = std::thread(&Room::COUTPACKETCOUNT, this);
	//t1.detach(); // 안전하게 백그라운드 실행

	for (int c = 0; c < CONSUMERS_PER_QUEUE; ++c)
	{
		t2s.emplace_back(&Room::CONSUMER, this);

#ifdef _WIN32
		// Set a readable thread description for debugger (RoomConsumer-<n>)
		std::string name = "RoomConsumer-" + std::to_string(c);
		std::wstring wname(name.begin(), name.end());
		// native_handle() on MSVC yields HANDLE
		HRESULT hr = SetThreadDescription(t2s.back().native_handle(), wname.c_str());
		(void)hr; // ignore result
#endif

		t2s.back().detach();
	}
}

void Room::EnterObject(const shared_ptr<Object>& object)
{
	cout << "Room : {ID : " << object->_objectId <<", Type "<<object->_type << " Entered\n";

	// TODO : 랜덤 삭제. 임시로 입장은 아무 스레드에게나 랜덤 할당
	//int idx = object->_objectId % tWorkerThread;
	lock_guard<mutex> guard(eLock);

	_insertObjectTable[object->_type](object);
}

void Room::ExitObject(const shared_ptr<Object>& object)
{
	cout << "Room : {ID : " << object->_objectId << ", Type " << object->_type << " Exit\n";

	// TODO : 랜덤 삭제. 임시로 입장은 아무 스레드에게나 랜덤 할당
	//int idx = rand() % tWorkerThread;
	lock_guard<mutex> guard(eLock);

	_eraseObjectTable[object->_type](object);
}

//void Room::Broadcast(const string& msg)
void Room::Broadcast(const Message& packet)
{
	/*
	// 1. 락을 사용한 스냅샷으로 플레이어 목록 캡쳐
	// TODO 락을 걸필요까진 없고 리드온리 하는 법 있었던 것 같은데
	std::vector<std::pair<uint32_t, std::shared_ptr<Player>>> playersSnapshot;
	{
		std::lock_guard<std::mutex> guard(bLock); // Enter/Exit와 동일한 락 사용
		playersSnapshot.reserve(_players.size());
		for (const auto& kv : _players)
			playersSnapshot.emplace_back(kv.first, kv.second);
	}

	// 2. 긴 작업은 락없이 스냅샷으로
	for (const auto& [id, player] : playersSnapshot) // C++ 17 structured binding
	{
		if (auto session = player->GetSession())
		{
			session->SendQueuePush(msg.c_str(), static_cast<int>(msg.size()));
			//session->Send(msg.c_str(), static_cast<int>(msg.size()));
		}
	}
	*/
}

void Room::BroadcastSerialized(shared_ptr<vector<uint8_t>> buffer)
{
	lock_guard<mutex> guard(eLock);
	_pendingMSG.push_back(std::move(buffer));
}

void Room::Flush()
{
	// TODO : 게임상 처리 로직
	while (true)
	{



	}

}

void Room::PushMoveJob(Job job)
{
}

void Room::PushETCJob(Job job)
{
	{
		//lock_guard<std::mutex> guard(qLock);
		unique_lock<mutex> lock(qLock);
		ETCQueue.push(std::move(job));
	}

	cv.notify_one();

	//bool expected = false;
	//// ETCflushing가 false라면, true로 바꾼 후에 if 내부 내용 실행
	//if (ETCflushing.compare_exchange_strong(expected, true))
	//{
	//	FlushETCQueue();
	//}
}

void Room::CONSUMER()
{
	while (true)
	{
		queue<Job> localQueue;
		{
			unique_lock<mutex> lock(qLock);

			// 람다 : 깨어나는 조건
			cv.wait(lock, [this]() {return ETCQueue.empty() == false; });
			// 1) Lock을 잡고 / unique_lock<mutex> lock(m);
			// 2) 조건 확인 / [](){return q.empty() == false;
			// 만족 => 빠져나와서 이어서 코드 진행
			// 불만족 => Lock을 풀어주고 대기 상태로 변환
			// => empty일 때 까지 while로 돌기 때문에 사실상 while(q.size())처럼 동작 

			// notify_one 했으면 항상 조건식[](){return q.empty() == false;을 만족하는거 아닐까?
			// Spurious Wakeup 가짜 기상 (데이터가 있는줄 알고 락 잡았더니 사실 없음)
			// notify_one 할 때 Consumer()가 lock을 잡은 상태가 아니기 때문에 Spurious Wakeup 발생
			// 그렇기 때문에 추가적인 조건[](){return q.empty() == false;으로 크로스체킹.

			localQueue.swap(ETCQueue);
		}

		int temp = localQueue.size();

		while (!localQueue.empty())
		{
			localQueue.front().Execute();
			localQueue.pop();
		}

		vector<shared_ptr<vector<uint8_t>>> lpendingMSG;
		{
			lock_guard<mutex> guard(eLock);
			lpendingMSG.swap(_pendingMSG);
		}

		if (temp > 1)
			cout << "{localQueue : _pendingMSG} : " << temp << " " << lpendingMSG.size() << '\n';

		// 1. 락을 사용한 스냅샷으로 플레이어 목록 캡쳐
		// TODO 락을 걸필요까진 없고 리드온리 하는 법 있었던 것 같은데
		vector<pair<uint32_t, shared_ptr<Player>>> playersSnapshot;
		{
			std::lock_guard<std::mutex> guard(bLock); // Enter/Exit와 동일한 락 사용
			playersSnapshot.reserve(_players.size());
			for (const auto& kv : _players)
				playersSnapshot.emplace_back(kv.first, kv.second);
		}

		// 2. 긴 작업은 락없이 스냅샷으로
		for (const auto& [id, player] : playersSnapshot) // C++ 17 structured binding
		{
			if (auto session = player->GetSession())
			{
				session->Send(lpendingMSG);
			}
		}
	}
}

// PushETCJob()의 compare_exchange_strong을 통해 들어오므로 이 함수는 항상 1개의 스레드가 실행
void Room::FlushETCQueue()
{
	//if (ETCQueue.size() > 1)
	//	cout << "q size : " << ETCQueue.size() << '\n';

	//queue<Job> localQueue;
	//{
	//	while (ETCQueue.size()) {
	//		localQueue.push(move(ETCQueue.front()));
	//		ETCQueue.pop();
	//	}
	//}

	//auto q = std::make_shared<queue<Job>>(std::move(localQueue));
	//boost::asio::post(*_io, [q, this]() {
	//	while (!q->empty()) {
	//		q->front().Execute();
	//		q->pop();
	//	}
	//	ETCflushing.store(false, std::memory_order_release);
	//	});
}


void Room::CreateObstacle()
{

}

void Room::InitObjectTable()
{
	// Insert
	_insertObjectTable[OBJECT_TYPE_PLAYER] = [&](auto& o) {
		_players[o->_objectId] = static_pointer_cast<Player>(o);
		};
	_insertObjectTable[OBJECT_TYPE_PROJECTILE] = [&](auto& o) {
		_projectiles[o->_objectId] = static_pointer_cast<Projectile>(o);
		};
	_insertObjectTable[OBJECT_TYPE_DUST] = [&](auto& o) {
		_dusts[o->_objectId] = static_pointer_cast<Dust>(o);
		};

	// Erase
	_eraseObjectTable[OBJECT_TYPE_PLAYER] = [&](auto& o) {
		_players.erase(o->_objectId);
		};
	_eraseObjectTable[OBJECT_TYPE_PROJECTILE] = [&](auto& o) {
		_projectiles.erase(o->_objectId);
		};
	_eraseObjectTable[OBJECT_TYPE_DUST] = [&](auto& o) {
		_dusts.erase(o->_objectId);
		};
}

void Room::CreateDust()
{
}

shared_ptr<Room> GRoom = make_shared<Room>(); // 헤더에서 선언한 것을 정의