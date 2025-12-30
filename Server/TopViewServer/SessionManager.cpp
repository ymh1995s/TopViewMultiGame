#include "pch.h"
#include "SessionManager.h"
#include "Session.h"

// TODO : Lock 경합 제거 
shared_ptr<Session> SessionManager::AddSession()
{
	int id = ClientNo.fetch_add(1, memory_order_relaxed);
	shared_ptr<Session> session = make_shared<Session>(id, shared_from_this());

	{
		// 복사 없음, 이동(emplace)만 1번, sharedPtr 증가 없음
		lock_guard<mutex> guard(lock);
		auto result = sessions.emplace(id, move(session));
		return result.first->second;
	}
}

void SessionManager::RemoveSession(int id)
{
	lock_guard<mutex> guard(lock);
	sessions.erase(id);
}

shared_ptr<Session> SessionManager::FindSession(int id)
{
	lock_guard<mutex> guard(lock);
	auto it = sessions.find(id);
	if (it != sessions.end())
	{
		return it->second;
	}
	return nullptr;
}

void SessionManager::Broadcast(const char* msg, int size)
{
	// TODO : 필요 시 재구현
	for (auto& pair : sessions)
	{
		auto& session = pair.second;
		//session->Send(msg, size);
	}
}
