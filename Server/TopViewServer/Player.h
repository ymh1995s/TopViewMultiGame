#pragma once
using namespace std;

#include "Object.h"
#include "Room.h"

class Player : public Object
{
public:
	shared_ptr<Room> GetRoom()
	{
		// shared_lock : 읽기 전용 락, 여러 스레드 읽기 가능
		std::shared_lock lock(m);
		return room.lock();
	}
	shared_ptr<Session> GetSession()
	{
		std::shared_lock lock(m);
		return session.lock();
	}

	void SetRoom(shared_ptr<Room> r)
	{
		// unique_lock : 쓰기 전용 락, 1개의 스레드만 접근 가능
		std::unique_lock lock(m);
		room = r;
	}
	void SetSession(shared_ptr<Session> s)
	{
		std::unique_lock lock(m);
		session = s;
	}

private:
	weak_ptr<class Room> room;
	weak_ptr<class Session> session;
	mutable std::shared_mutex m;
};

