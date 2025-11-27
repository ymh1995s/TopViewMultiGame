#include "pch.h"
#include "SessionManager.h"
#include "Session.h"

// TODO : Lock 경합 제거 
shared_ptr<Session> SessionManager::AddSession()
{
	int id = ClientNo.fetch_add(1, memory_order_relaxed);
	shared_ptr<Session> session = make_shared<Session>(id, shared_from_this());

	// make_pair : 임시 객체 생성 비용 + sharedptr 1증가
	// insert : session(쉐어드 포인터이므로) 복사 + sharedptr 1증가
	// sessions.insert({ ClientNo, session });

	/*
	{
		// 복사 없음, 이동(emplace)만 1번, sharedPtr 증가 없음
		lock_guard<mutex> guard(lock);
		auto result = sessions.emplace(id, move(session));
		return result.first->second;
	}
	*/
	// TODO 삭제하고 위에 최적화 된 코드로 교체
	lock_guard<mutex> guard(lock);
	sessions.insert({ ClientNo, session });
	return sessions[ClientNo];
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
	// TODO : 잡큐 방식을 통해 1개의 스레드만 접근 가능하도록 => 다른 함수도 마찬가지 
	// lock_guard<mutex> guard(lock);
	for (auto& pair : sessions)
	{
		auto& session = pair.second;
		session->Send(msg, size);
	}
}
