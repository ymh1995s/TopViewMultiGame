#pragma once
#include "Protocol.pb.h"
using namespace std;


// session : 해당 세션, data : 패킷 데이터, length : 패킷 길이
// C#으로 치면 Action<Session, byte[], int>
// ex) 인수 없음, 반환 없음 => std::function<void()> f;
// ex) int, float 받고 반환 없음 => std::function<void(int, float)> f;
// ex) int 두 개 받고 string을 반환하는 함수 => std::function<string(int, int)> f;

// extern 함으로써 선안만
using Handler = std::function<void(std::shared_ptr<class Session>, BYTE*, int)>;
extern Handler GPacketHandler[UINT16_MAX];

using uint16 = unsigned __int16;

enum : int // TODO : 삭제
{
	C_CHAT = 1,
	S_CHAT = 2,
};

bool Handle_C_CLASS_CHOICE(shared_ptr<Session> session, Protocol::C_Chat& pkt);

class PacketHandler
{
public:
	static void Init()
	{
		GPacketHandler[C_CHAT] = [](shared_ptr<Session> session, BYTE* buffer, int len) 
			{
				return HandlePacket<Protocol::C_Chat>(Handle_C_CLASS_CHOICE, session, buffer, len); 
			};
		/*
			GPacketHandler[C_PLAYER_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PlayerMove>(Handle_C_PLAYER_MOVE, session, buffer, len); };
			GPacketHandler[C_PLAYER_DIE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PlayerDie>(Handle_C_PLAYER_DIE, session, buffer, len); };
			GPacketHandler[C_PLAYER_SKILL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PlayerSkill>(Handle_C_PLAYER_SKILL, session, buffer, len); };
			GPacketHandler[C_HIT_MONSTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_HitMonster>(Handle_C_HIT_MONSTER, session, buffer, len); };
			GPacketHandler[C_PLAYER_DAMAGED] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PlayerDamaged>(Handle_C_PLAYER_DAMAGED, session, buffer, len); };
			GPacketHandler[C_CHANGE_MAP] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ChangeMap>(Handle_C_CHANGE_MAP, session, buffer, len); };
			GPacketHandler[C_BOSS_REGISTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_BossRegister>(Handle_C_BOSS_REGISTER, session, buffer, len); };
			GPacketHandler[C_BOSS_CANCLE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_BossCancle>(Handle_C_BOSS_CANCLE, session, buffer, len); };
			GPacketHandler[C_LOOT_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LootItem>(Handle_C_LOOT_ITEM, session, buffer, len); };
		*/
	}

	static  shared_ptr<BYTE[]> MakeSendBuffer(Protocol::S_Chat& pkt) { return MakeSendBuffer(pkt, S_CHAT); }
	/*
	static SendBufferRef MakeSendBuffer(Protocol::S_EnterGame& pkt) { return MakeSendBuffer(pkt, S_ENTER_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PlayerSpawn& pkt) { return MakeSendBuffer(pkt, S_PLAYER_SPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MonsterSpawn& pkt) { return MakeSendBuffer(pkt, S_MONSTER_SPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PlayerMove& pkt) { return MakeSendBuffer(pkt, S_PLAYER_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MonsterMove& pkt) { return MakeSendBuffer(pkt, S_MONSTER_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::S_LeaveGame& pkt) { return MakeSendBuffer(pkt, S_LEAVE_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PlayerDespawn& pkt) { return MakeSendBuffer(pkt, S_PLAYER_DESPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MonsterDespawn& pkt) { return MakeSendBuffer(pkt, S_MONSTER_DESPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ItemSpawn& pkt) { return MakeSendBuffer(pkt, S_ITEM_SPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PlayerSkill& pkt) { return MakeSendBuffer(pkt, S_PLAYER_SKILL); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MonsterSkill& pkt) { return MakeSendBuffer(pkt, S_MONSTER_SKILL); }
	static SendBufferRef MakeSendBuffer(Protocol::S_HitMonster& pkt) { return MakeSendBuffer(pkt, S_HIT_MONSTER); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PlayerDamaged& pkt) { return MakeSendBuffer(pkt, S_PLAYER_DAMAGED); }
	static SendBufferRef MakeSendBuffer(Protocol::S_BossRegisterDeny& pkt) { return MakeSendBuffer(pkt, S_BOSS_REGISTER_DENY); }
	static SendBufferRef MakeSendBuffer(Protocol::S_BossWaiting& pkt) { return MakeSendBuffer(pkt, S_BOSS_WAITING); }
	static SendBufferRef MakeSendBuffer(Protocol::S_GameClear& pkt) { return MakeSendBuffer(pkt, S_GAME_CLEAR); }
	static SendBufferRef MakeSendBuffer(Protocol::S_GetExp& pkt) { return MakeSendBuffer(pkt, S_GET_EXP); }
	static SendBufferRef MakeSendBuffer(Protocol::S_LootItem& pkt) { return MakeSendBuffer(pkt, S_LOOT_ITEM); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ItemDespawn& pkt) { return MakeSendBuffer(pkt, S_ITEM_DESPAWN); }
	*/



private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, shared_ptr<Session> session, BYTE* buffer, int len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	// TODO? : SendBuffer 최적화
	template<typename T>
	static shared_ptr<BYTE[]> MakeSendBuffer(T& pkt, unsigned __int16 pktId)
	{
		const uint16 dataSize = static_cast<unsigned __int16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		std::shared_ptr<BYTE[]> buffer(
			new BYTE[packetSize],            // 동적 배열 생성
			std::default_delete<BYTE[]>()    // 배열[]용 delete 옵션, 없으면 일반 포인터 delete가 되어 UB
		);

		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer.get());
		header->size = packetSize;
		header->id = pktId;

		pkt.SerializeToArray(buffer.get() + sizeof(PacketHeader), dataSize);

		return buffer;
	}
};

