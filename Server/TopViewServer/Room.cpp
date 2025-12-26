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

	t = std::thread(&Room::COUTPACKETCOUNT, this);
	t.detach(); // 안전하게 백그라운드 실행
}

void Room::EnterObject(const shared_ptr<Object>& object)
{
	cout << "Room : {ID : " << object->_objectId <<", Type "<<object->_type << " Entered\n";

	// TODO : 랜덤 삭제. 임시로 입장은 아무 스레드에게나 랜덤 할당
	int idx = rand() % tWorkerThread;
	lock_guard<mutex> guard(lock[idx]);

	_insertObjectTable[object->_type](object);
}

void Room::ExitObject(const shared_ptr<Object>& object)
{
	cout << "Room : {ID : " << object->_objectId << ", Type " << object->_type << " Exit\n";

	// TODO : 랜덤 삭제. 임시로 입장은 아무 스레드에게나 랜덤 할당
	int idx = rand() % tWorkerThread;
	lock_guard<mutex> guard(lock[idx]);

	_eraseObjectTable[object->_type](object);
}

void Room::Broadcast(const string& msg)
{
	// 1. 락을 사용한 스냅샷으로 플레이어 목록 캡쳐
	std::vector<std::pair<uint32_t, std::shared_ptr<Player>>> playersSnapshot;
	{
		std::lock_guard<std::mutex> guard(lock[0]); // Enter/Exit와 동일한 락 사용
		playersSnapshot.reserve(_players.size());
		for (const auto& kv : _players)
			playersSnapshot.emplace_back(kv.first, kv.second);
	}

	// 2. 긴 작업은 락없이 스냅샷으로
	for (const auto& [id, player] : playersSnapshot) // C++ 17 structured binding
	{
		if (auto session = player->GetSession())
		{
			int targetQueue = id % tWorkerThread; // N개의 큐 중 하나 선택
			session->SendQueuePush(msg.c_str(), static_cast<int>(msg.size()), targetQueue);
			//session->Send(msg.c_str(), static_cast<int>(msg.size()));
		}
	}
}

void Room::PushMoveJob(Job job)
{
}

void Room::PushETCJob(Job job)
{
	constexpr int QUEUE_COUNT = tWorkerThread;
	int targetQueue = job._targetQueue % QUEUE_COUNT;

	{
		lock_guard<std::mutex> guard(lock[job._targetQueue]);
		ETCQueue[targetQueue].push(std::move(job));
	}

	bool expected = false;
	// ETCflushing가 false라면, true로 바꾼 후에 if 내부 내용 실행
	if (ETCflushing[targetQueue].compare_exchange_strong(expected, true))
	{
		FlushETCQueue(targetQueue);
	}
}

void Room::FlushETCQueue(int targetQueue)
{
	//if (ETCQueue.size() > 1)
	//	cout << "q size : " << ETCQueue.size() << '\n';

	queue<Job> localQueue;
	{
		lock_guard<std::mutex> guard(lock[targetQueue]);
		while (ETCQueue[targetQueue].size()) {
			localQueue.push(move(ETCQueue[targetQueue].front()));
			ETCQueue[targetQueue].pop();
		}
	}

	// 문제 : 1~200번 패킷이 있다고 치면 1~100번 패킷이 첫번째 실행.. 
			// 첫번 째 워커스레드에서 아직 50번 패킷이 실행될 때 
			// 두번 째 워커스레드에서 101~200번 패킷이 실행된다고 치면
			// 101번 패킷이 실행된다고 치자 그럼 51~100은 101번보다 늦게 실행될 수 있음
	// 해결책? : 순서를 보장하고 싶으면 strand를 사용한다.

	// 시나리오 
	// 처음에 1개 패킷만 오겠지->A워커스레드가 실행
		// 그 사이에 200개가 왔다고 치자->B워커 스레드실행, A워커 스레드 작업 완료
		// 그 사이에 200개가 왔다고 치자->B워커스레드는 아직 50번째 브로드캐스팅중, A워커 스레드한테 200개 람다 실행
		// 그 사이에 200개가 왔다고 치자->B는 아직 100번쨰, A는 아직 50번째, C 워커 스레드한테 200개 람다 실행
		// ...
	// 이런식으로 총 17개의 워커스레드가 동작함, 순서보장은 X
	auto q = std::make_shared<queue<Job>>(std::move(localQueue));
	boost::asio::post(*_io, [q, this, targetQueue]() {
		while (!q->empty()) {
			q->front().Execute();
			q->pop();
		}
		// flush 완료 표시
		ETCflushing[targetQueue].store(false, std::memory_order_release);
		});

	// !실제 비동기 작업이 완료될 때, 즉 위의 람다 안에서 store되어야 한다.
	//ETCflushing[targetQueue].store(false, std::memory_order_release);
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