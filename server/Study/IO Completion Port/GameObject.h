#pragma once

class Service;

enum ObjectType : char {
	Player,
	Npc
};

class GameObject
{
	friend class Service;

public:
	GameObject() : _id(-1), _x(rand() % MAP_SIZE), _y(rand() % MAP_SIZE), _type(Player), _lastMoveTime(0) {}
	GameObject(int id, short x, short y) : _id(id), _x(x), _y(y), _type(Player), _lastMoveTime(0) {}
	virtual ~GameObject() {}

public:
	virtual bool IsVisible() const { return true; }

public:
	// Getter
	const std::string& GetName() const { return _name; }
	const ObjectType GetType() const { return _type; }
	int GetId()    const { return _id; }
	int GetX()     const { return _x; }
	int GetY()     const { return _y; }

	// Setter
	void SetId(int id) { _id = id; }

protected:
	ObjectType _type;

	int _id;
	short _x, _y;
	std::string _name;

	// 레벨, HP 등 Game Contents에 추가적으로 필요한 변수들

public:
	int _lastMoveTime;
};

class NPC : public GameObject, public std::enable_shared_from_this<NPC>
{
public:
	NPC(int id, short x, short y, std::string_view name);

public:
	void WakeUp(bool force = false);
	void Update();
	void OnTimer();
	void RegisterTimer();

public:
	void RandomMove();

public:
	// Service 관련
	void SetService(std::shared_ptr<Service> service) { _service = service; }
	const std::shared_ptr<Service>& GetService() const { return _service.lock(); }

private:
	//std::chrono::steady_clock::time_point _nextActionTime;
	std::weak_ptr<Service> _service;

public:
	std::atomic<bool> _isActive{ false };
	std::atomic<bool> _timerPending{ false };
};