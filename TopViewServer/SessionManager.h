#pragma once

using namespace std;

class Session;

class SessionManager
{
public:
	void AddSession(shared_ptr<Session> session);
	void RemoveSession(int id);

	shared_ptr<Session> FindSession(int id);

	void Broadcast();

private:
	mutex lock;

private:
	atomic<int> ClientNo = 0;
	// vector는 찾는데 N이니까 umap으로 하자
	unordered_map<int, shared_ptr<Session>> sessions; // 서버와 연결된 클라이언트 세션들
};

