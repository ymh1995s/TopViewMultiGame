#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Projectile.h"
#include "Dust.h"
#include "Obstacle.h"

void Room::Broadcast(const string& msg)
{
	for (const auto& [id, player] : _players) // C++ 17 structured binding
	{
		//player->Send(msg);
	}
}

void Room::MakeObstacle()
{
}

shared_ptr<Room> GRoom = make_shared<Room>(); // 헤더에서 선언한 것을 정의