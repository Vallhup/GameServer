#pragma once

#include "ChatManager.h"
#include "Sector.h"

class IocpCore;
class Listener;
class GameObject;
class Party;

enum EventType : char {
	EV_MOVE,
	EV_HEAL,
	EV_ATTACK,
	EV_PLAYER_HEAL
};

struct Event {
	int objId;
	std::chrono::high_resolution_clock::time_point wakeupTime;
	char eventId;
	int targetId;

	bool operator<(const Event& other) const {
		return wakeupTime > other.wakeupTime;
	}
};

class Service : public std::enable_shared_from_this<Service>
{
	friend class Session;
	friend class GameSession;
	friend class GameObject;
	friend class NPC;

public:
	Service(std::shared_ptr<IocpCore> core, int maxSessionCount);

public:
	bool Start();
	void CloseService();

public:
	// 객체 탐색, 추가, 삭제
	std::shared_ptr<GameObject> FindObject(int id);
	int AddObject(const std::shared_ptr<GameObject> object);
	void ReleaseObject(const std::shared_ptr<GameObject> object);
	void FinalizeRelease(const std::shared_ptr<GameSession>& session);

	int AllocateObjectId(const std::shared_ptr<GameObject>& object);

public:
	// NPC 관련
	void InitNpcs(int npcCount);
	void StartNpcTimerThread();

	// Player Login 시 인접한 NPC Wake Up
	void OnPlayerLogin(const std::shared_ptr<GameSession>& session);

	// Player가 이동 후 인접한 NPC Wake Up
	void OnPlayerMove(const std::shared_ptr<GameSession>& session);

public:
	// Sector 관련
	void enterSector(const std::shared_ptr<GameObject>& object);
	void leaveSector(const std::shared_ptr<GameObject>& object);

	void enterSector(const std::shared_ptr<GameObject>& object, int sx, int sy);
	void leaveSector(const std::shared_ptr<GameObject>& object, int sx, int sy);

	std::unordered_set<int> collectVisibleObjects(const std::shared_ptr<GameObject>& object) const;

public:
	// Chating 관련
	void OnChatRequest(int senderId, const char* msg);
	void Broadcast(const std::vector<char>& buf);

public:
	// Packet Handler
	bool OnPacket(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);

	bool OnLogin(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);
	bool OnLogout(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);
	bool OnMove(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);
	bool OnAttack(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);
	bool OnChat(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);

	bool OnPartyRequest(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);
	bool OnPartyResponse(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);
	bool OnPartyLeave(const std::shared_ptr<GameSession>& session);
	void OnPartyDisband(int partyId);

	bool OnUseItem(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet);

public:
	// Getter
	int getMaxSessionCount() const { return _maxSessionCount; }
	std::shared_ptr<IocpCore>& getIocpCore() { return _iocpCore; }

	int getRandomInterval(int minMs, int maxMs)
	{
		thread_local std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution<int> dist{ minMs, maxMs };

		return dist(rng);
	}

public:
	// Factory
	static std::shared_ptr<Service> Create(std::shared_ptr<IocpCore> core, int maxSessionCount = 10);

private:
	std::shared_ptr<IocpCore> _iocpCore;
	std::shared_ptr<Listener> _listener{ nullptr };

private:
	ChatManager _chatManager;

public:
	// object 관리
	concurrency::concurrent_unordered_map<int, std::atomic<std::shared_ptr<GameObject>>> _objects;
	std::mutex _idMutex;

	std::atomic<int> _nextNpcId;
	concurrency::concurrent_queue<int> _freeNpcIds;

	std::atomic<int> _nextPlayerId;
	concurrency::concurrent_queue<int> _freePlayerIds;

	int _maxSessionCount{ 0 };

public:
	// Party 관리
	std::atomic<int> _nextPartyId;
	std::unordered_map<int, std::atomic<std::shared_ptr<Party>>> _parties;
	mutable std::shared_mutex _partyMutex;

private:
	std::array<std::array<Sector, SECTOR_COUNT>, SECTOR_COUNT> _sectors;

public:
	std::array<std::array<bool, 2000>, 2000> _navigationMap;

private:
	std::vector<std::thread> _workers;
	std::thread _npcTimerThread;

public:
	concurrency::concurrent_priority_queue<Event> _timerQueue;

public:
	std::atomic<bool> _running{ false };
};

