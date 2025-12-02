#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Projectile.h"
#include "Dust.h"
#include "Obstacle.h"
#include "Job.h"

void Room::Init()
{
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
	if(ETCQueue.size() > 1) 
		cout << "q size : " << ETCQueue.size() << '\n';

	queue<Job> localQueue;	
	{
		lock_guard<std::mutex> guard(lock);
		// TODO Move로 최적화
		while(ETCQueue.size()) {
			localQueue.push(ETCQueue.front());
			ETCQueue.pop();
		}
	}

	while(localQueue.size()) {
		localQueue.front().Execute();
		localQueue.pop();
	}

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