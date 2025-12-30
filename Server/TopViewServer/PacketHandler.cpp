#include "pch.h"
#include "Session.h"
#include "PacketHandler.h"
#include "Player.h"
#include "Job.h"
#include "Room.h"

#include <google/protobuf/message.h>
using google::protobuf::Message;

// 헤더에선 extern으로 선언, 여기에선 정의
Handler GPacketHandler[UINT16_MAX];

bool Handle_C_CHAT(shared_ptr<Session> session, Protocol::C_Chat& pkt)
{
	if (auto room = session->GetPlayer()->GetRoom())
	{
        auto sendBuffer = std::make_shared<std::vector<uint8_t>>();
        {
            // 직렬화 : 여기서 미리 W 스레드가 부담 
            // TODO : 네트워크 스레드 부하가 크면 다른 곳으로 이동
            Protocol::S_Chat chat;
            chat.set_message(pkt.message());

            const size_t size = chat.ByteSizeLong();
            sendBuffer->resize(size);
            chat.SerializeToArray(sendBuffer->data(), static_cast<int>(size));
        }

        Job job([room, sendBuffer]()
            {
                room->BroadcastSerialized(sendBuffer);
            });

        room->PushETCJob(std::move(job));
    }
    return true;
}
