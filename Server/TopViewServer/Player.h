#pragma once
using namespace std;

#include "Object.h"
#include "Room.h"

class Player : public Object
{
public:
	weak_ptr<Room> GetRoom() { return room.lock(); }
	weak_ptr<Session> GetSession() { return session.lock(); }

private:
	weak_ptr<class Room> room;
	weak_ptr<class Session> session;
};

