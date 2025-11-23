#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Projectile.h"
#include "Dust.h"
#include "Obstacle.h"

void Room::Init()
{
	cout << "Room Init\n";
	CreateObstacle();
	CreateDust();
	InitObjectTable();
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
	lock_guard<mutex> guard(lock);
	for (const auto& [id, player] : _players) // C++ 17 structured binding
	{
		if (auto session = player->GetSession().lock()) // weak_ptr → shared_ptr 안전 접근
		{
			session->Send(msg.c_str(), static_cast<int>(msg.size()));
		}
	}
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