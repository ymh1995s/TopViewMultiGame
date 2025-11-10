#include "pch.h"
#include "SessionManager.h"
#include "Session.h"

// TODO : Lock 경합 제거 
int SessionManager::AddSession(shared_ptr<Session> session)
{
	int id = ClientNo.fetch_add(1);
	session->SetSessionId(id);


	// make_pair : 임시 객체 생성 비용 + sharedptr 1증가
	// insert : session(쉐어드 포인터이므로) 복사 + sharedptr 1증가
	// sessions.insert({ ClientNo, session });

	{
		// 복사 없음, 이동(emplace)만 1번, sharedPtr 증가 없음
		lock_guard<mutex> guard(lock);
		sessions.emplace(id, move(session));
	}

	return id;
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

void SessionManager::Broadcast()
{
	lock_guard<mutex> guard(lock);
	for (auto& pair : sessions)
	{
		auto& session = pair.second;
		//session->Send();
	}
}
