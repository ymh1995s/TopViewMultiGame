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

#include <google/protobuf/message.h>
using google::protobuf::Message;

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
		lThreads.emplace_back(&Room::Flush, this);

#ifdef _WIN32
		// Set a readable thread description for debugger (RoomConsumer-<n>)
		std::string name = "RoomConsumer-" + std::to_string(c);
		std::wstring wname(name.begin(), name.end());
		// native_handle() on MSVC yields HANDLE
		HRESULT hr = SetThreadDescription(lThreads.back().native_handle(), wname.c_str());
		(void)hr; // ignore result
#endif

		lThreads.back().detach();
	}
}

void Room::EnterObject(const shared_ptr<Object>& object)
{
	cout << "Room : {ID : " << object->_objectId <<", Type "<<object->_type << " Entered\n";
	lock_guard<mutex> guard(_lock);
	_insertObjectTable[object->_type](object);
}

void Room::ExitObject(const shared_ptr<Object>& object)
{
	cout << "Room : {ID : " << object->_objectId << ", Type " << object->_type << " Exit\n";
	lock_guard<mutex> guard(_lock);
	_eraseObjectTable[object->_type](object);
}

void Room::BroadcastSerialized(shared_ptr<vector<uint8_t>> buffer)
{
	lock_guard<mutex> guard(_lock);
	_pendingMSG.push_back(std::move(buffer));
}

void Room::PushMoveJob(Job job)
{
}

void Room::PushETCJob(Job job)
{
	{
		// lock_guard<std::mutex> guard(qLock); // 경우에 따라 락가드도 사용할 수 있는 모양
		unique_lock<mutex> lock(_lock);
		ETCQueue.push(std::move(job));
	}

	cv.notify_one();
}

void Room::Flush()
{
	while (true)
	{
		queue<Job> localQueue;
		{
			unique_lock<mutex> lock(_lock);

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

		int qSize = localQueue.size();

		while (!localQueue.empty())
		{
			localQueue.front().Execute();
			localQueue.pop();
		}

		// 1. 락을 사용한 스냅샷으로 플레이어 목록과 메시지 큐 복사
		// TODO 락을 걸필요까진 없고 리드온리 하는 법 있었던 것 같은데
		vector<shared_ptr<vector<uint8_t>>> lpendingMSG;
		vector<pair<uint32_t, shared_ptr<Player>>> playersSnapshot;
		{
			lock_guard<mutex> guard(_lock);
			lpendingMSG.swap(_pendingMSG);

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

		if (qSize > 1) cout << "{localQueue : _pendingMSG} : " << qSize << " " << lpendingMSG.size() << '\n';
	}
}

void Room::CreateObstacle()
{

}

void Room::TCOUNTPACKET()
{
	while (true)
	{
		cout << "{Player:countPackets} : " << _players.size() << " " << countPackets.load() << '\n';
		countPackets.store(0);

		this_thread::sleep_for(std::chrono::seconds(1));
	}
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