#pragma once
#include "Job.h"
#include "PQJob.h"

class Player;
class Projectile;
class Dust;
class Obstacle;
class Object;

using namespace std;

class Room
{
public:
	void Init(boost::asio::io_context& io);
	void InitObjectTable();
	void EnterObject(const shared_ptr<Object>& Object);
	void ExitObject(const shared_ptr<Object>& Object);
	void BroadcastSerialized(shared_ptr<vector<uint8_t>> buffer);

	void PushMoveJob(class Job job);
	void PushETCJob(class Job job);
	void Flush();

// 컨텐츠 함수 : TODO
	void CreateDust();
	void CreateObstacle();

// 컨텐츠 자료구조
private:
	unordered_map<uint32_t, shared_ptr<Player>> _players;
	unordered_map<uint32_t, shared_ptr<Projectile>> _projectiles;
	unordered_map<uint32_t, shared_ptr<Dust>> _dusts;
	unordered_map<uint32_t, shared_ptr<Obstacle>> _obstacles;

// function 연습
private:
	using InsertFunc = function<void(const shared_ptr<Object>&)>;
	InsertFunc _insertObjectTable[3];
	using EraseFunc = function<void(const shared_ptr<Object>&)>;
	EraseFunc _eraseObjectTable[3];

// 네트워크 관련
private:
	mutex _lock;
	boost::asio::io_context* _io;

	// condition_variable 연습
	static constexpr int CONSUMERS_PER_QUEUE = 1;
	condition_variable cv;
	vector<thread> lThreads; // 로직 스레드

	queue<class Job> MoveQueues[4];
	queue<class Job> ETCQueue;
	vector<shared_ptr<vector<uint8_t>>> _pendingMSG;

// 테스트용
public:
	atomic<int> countPackets = 0; 
	thread t1;
	void TCOUNTPACKET();
};

extern shared_ptr<Room> GRoom; // 선언만