#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Projectile.h"
#include "Dust.h"
#include "Obstacle.h"
#include "Job.h"

void Room::Init(boost::asio::io_context& io)
{
	_io = &io;
	cout << "Room Init\n";
	CreateObstacle();
	CreateDust();
	InitObjectTable();

	//t = std::thread(&Room::COUTPACKETCOUNT, this);
	//t.detach(); // 안전하게 백그라운드 실행
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

void Room::Broadcast(const string& msg)
{
	// 1. 락을 사용한 스냅샷으로 플레이어 목록 캡쳐
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
}

void Room::PushMoveJob(Job job)
{
}

void Room::PushETCJob(Job job)
{
	{
		lock_guard<std::mutex> guard(qLock);
		ETCQueue.push(std::move(job));
	}

	bool expected = false;
	// ETCflushing가 false라면, true로 바꾼 후에 if 내부 내용 실행
	if (ETCflushing.compare_exchange_strong(expected, true))
	{
		FlushETCQueue();
	}
}

// PushETCJob()의 compare_exchange_strong을 통해 들어오므로 이 함수는 항상 1개의 스레드가 실행
void Room::FlushETCQueue()
{
	if (ETCQueue.size() > 1)
		cout << "q size : " << ETCQueue.size() << '\n';

	queue<Job> localQueue;
	{
		while (ETCQueue.size()) {
			localQueue.push(move(ETCQueue.front()));
			ETCQueue.pop();
		}
	}

	auto q = std::make_shared<queue<Job>>(std::move(localQueue));
	boost::asio::post(*_io, [q, this]() {
		while (!q->empty()) {
			q->front().Execute();
			q->pop();
		}

		bool needMoreFlush = false;
		{
			lock_guard<std::mutex> guard(qLock);
			needMoreFlush = !ETCQueue.empty();
		}

		if (needMoreFlush)
		{
			// 아직 남아있으면 다시 Flush
			FlushETCQueue();
		}
		else
		{
			// 진짜 끝났을 때만 false
			ETCflushing.store(false, std::memory_order_release);
		}

		});
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