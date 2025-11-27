#pragma once

class Player;
class Projectile;
class Dust;
class Obstacle;
class Object;

using namespace std;

class Room
{
public:
	void Init();
	void EnterObject(const shared_ptr<Object>& Object);
	void ExitObject(const shared_ptr<Object>& Object);
	void Broadcast(const string& msg);

private: // 아래 애들은 나중에 생각하자
	void CreateDust();
	void CreateObstacle();
private:
	void InitObjectTable();

private:
	unordered_map<uint32_t, shared_ptr<Player>> _players;
	unordered_map<uint32_t, shared_ptr<Projectile>> _projectiles;
	unordered_map<uint32_t, shared_ptr<Dust>> _dusts;
	unordered_map<uint32_t, Obstacle> _obstacles;

private: // function 연습
	using InsertFunc = function<void(const shared_ptr<Object>&)>;
	InsertFunc _insertObjectTable[3];

	using EraseFunc = function<void(const shared_ptr<Object>&)>;
	EraseFunc _eraseObjectTable[3];

private:
	mutex lock;

public:
	atomic<int> countPackets = 0; // 테스트용
	thread t;
	void COUTPACKETCOUNT()
	{
		while (true)
		{
			cout << "{Player:countPackets} : " <<_players.size()<< " "<< countPackets.load() << '\n';
			countPackets.store(0);

			this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
};

extern shared_ptr<Room> GRoom; // 선언만