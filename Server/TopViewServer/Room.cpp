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
	lock_guard<mutex> guard(lock);
	_insertObjectTable[object->_type](object);
}

void Room::ExitObject(const shared_ptr<Object>& object)
{
	cout << "Room : {ID : " << object->_objectId << ", Type " << object->_type << " Exit\n";
	lock_guard<mutex> guard(lock);
	_eraseObjectTable[object->_type](object);
}

void Room::Broadcast(const string& msg)
{
	for (const auto& [id, player] : _players) // C++ 17 structured binding
	{
		if (auto session = player->GetSession().lock()) // weak_ptr → shared_ptr 안전 접근
		{
			session->Send(msg.c_str(), static_cast<int>(msg.size()));
		}
	}
}

void Room::PushMoveJob(Job job)
{
}

void Room::PushETCJob(Job job)
{
	{
		lock_guard<std::mutex> guard(lock);
		// TODO Move로 최적화
		ETCQueue.push(job);
		//ETCQueue.push(std::move(job));
	}

	bool expected = false;
	// ETCflushing가 false라면, true로 바꾼 후에 if 내부 내용 실행
	if (ETCflushing.compare_exchange_strong(expected, true))
	{
		FlushETCQueue();
	}
}

void Room::FlushETCQueue()
{
	if (ETCQueue.size() > 1)
		cout << "q size : " << ETCQueue.size() << '\n';

	queue<Job> localQueue;
	{
		lock_guard<std::mutex> guard(lock);
		// TODO Move로 최적화
		while (ETCQueue.size()) {
			localQueue.push(ETCQueue.front());
			ETCQueue.pop();
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
	boost::asio::post(*_io, [q]() {
		while (!q->empty()) {
			q->front().Execute();
			q->pop();
		}
		});

	// flush 완료 표시
	ETCflushing.store(false, std::memory_order_release);
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