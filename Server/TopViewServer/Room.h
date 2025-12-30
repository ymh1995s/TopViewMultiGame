#pragma once
#include "Job.h"
#include "PQJob.h"

class Player;
class Projectile;
class Dust;
class Obstacle;
class Object;

#include <google/protobuf/message.h>
using google::protobuf::Message;

using namespace std;

class Room
{
public:
	void Init(boost::asio::io_context& io);
	void InitObjectTable();
	void EnterObject(const shared_ptr<Object>& Object);
	void ExitObject(const shared_ptr<Object>& Object);
	void Broadcast(const string& msg);
	void Broadcast(const Message& packet);
	void BroadcastSerialized(shared_ptr<vector<uint8_t>> buffer);

public:
	void PushMoveJob(class Job job);
	void PushETCJob(class Job job);
	void PushPQJob(class PQJob job);
	void FlushETCQueue();

private: // 아래 애들은 나중에 생각하자
	void CreateDust();
	void CreateObstacle();
	//======================

private:
	unordered_map<uint32_t, shared_ptr<Player>> _players;
	unordered_map<uint32_t, shared_ptr<Projectile>> _projectiles;
	unordered_map<uint32_t, shared_ptr<Dust>> _dusts;
	// store obstacles by shared_ptr to avoid needing complete type here
	unordered_map<uint32_t, shared_ptr<Obstacle>> _obstacles;

private: // function 연습
	using InsertFunc = function<void(const shared_ptr<Object>&)>;
	InsertFunc _insertObjectTable[3];
	using EraseFunc = function<void(const shared_ptr<Object>&)>;
	EraseFunc _eraseObjectTable[3];

private:
	queue<class Job> MoveQueues[4];
	queue<class Job> ETCQueue;
	atomic<bool> ETCflushing = false ;
	//priority_queue<PQJob, std::vector<PQJob>, PQJobCompare> PQQueue;

private:
	mutex bLock, qLock, eLock; // broadcast, jobqueue, enter/exit
	boost::asio::io_context* _io;

// condition_variable 연습
private:
	static constexpr int CONSUMERS_PER_QUEUE = 1;
	std::condition_variable cv;
	std::vector<std::thread> t2s;
	void CONSUMER();

// 재설계중..
private:
	void Flush();
	void tQueue();
	vector<shared_ptr<vector<uint8_t>>> _pendingMSG;

// 테스트용
public:
	atomic<int> countPackets = 0; 
	thread t1;
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