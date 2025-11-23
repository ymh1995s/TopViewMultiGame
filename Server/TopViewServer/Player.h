#pragma once
using namespace std;

#include "Object.h"
#include "Room.h"

class Player : public Object
{
private:
	weak_ptr<class Room> room;
};

