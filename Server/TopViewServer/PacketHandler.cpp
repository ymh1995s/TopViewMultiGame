#include "pch.h"
#include "Session.h"
#include "PacketHandler.h"

// 헤더에선 extern으로 선언, 여기에선 정의
Handler GPacketHandler[UINT16_MAX];

bool Handle_C_CLASS_CHOICE(shared_ptr<Session> session, Protocol::C_Chat& pkt)
{
	cout << "approch here \n";
	return true;
}
