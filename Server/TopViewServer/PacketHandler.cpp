#include "pch.h"
#include "Session.h"
#include "PacketHandler.h"
#include "Player.h"

// 헤더에선 extern으로 선언, 여기에선 정의
Handler GPacketHandler[UINT16_MAX];

bool Handle_C_CHAT(shared_ptr<Session> session, Protocol::C_Chat& pkt)
{
	session->GetPlayer()->GetRoom().lock()->Broadcast(pkt.message());
	return true;
}
