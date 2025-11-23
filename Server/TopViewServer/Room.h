#pragma once

class Player;
class Projectils;
class Dust;
class Obstacle;

using namespace std;

class Room
{
public:
	void Init()
	{

	}

	void Broadcast(const string& msg);

public:

private:
	void MakeObstacle();

private:
	unordered_map<uint32_t, shared_ptr<Player>> _players;
	unordered_map<uint32_t, shared_ptr<Projectils>> _projectile;
	unordered_map<uint32_t, shared_ptr<Dust>> _items;
	unordered_map<uint32_t, Obstacle> _obstacles;
};

extern shared_ptr<Room> GRoom; //tjsdjsaks