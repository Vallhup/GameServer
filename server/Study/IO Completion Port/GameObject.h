#pragma once

class Service;
class IocpObject;

struct APos;

enum ObjectType : char {
	Player,
	Npc
};

class GameObject
{
	friend class Service;

public:
	GameObject();
	GameObject(int id, short x, short y);

	virtual ~GameObject() {}

public:
	virtual bool IsVisible() const { return true; }

public:
	// Getter
	const std::string& GetName() const { return _name; }
	const ObjectType GetType() const { return _type; }
	int GetId()      const { return _id; }
	int GetExp()     const { return _exp; }
	short GetX()     const { return _x; }
	short GetY()     const { return _y; }
	short GetHp()    const { return _hp; }
	short GetMaxHp() const { return _maxHp; }
	short GetLevel() const { return _level; }
	bool IsAlive()   const { return _isAlive.load(); }

	// Setter
	void SetId(int id) { _id = id; }
	void SetExp(int exp) { _exp += exp; /* 레벨업 처리 */ }

protected:
	ObjectType _type;

	int _id;
	short _x, _y;
	std::string _name;

	// 레벨, HP 등 Game Contents에 추가적으로 필요한 변수들
	short _hp;
	short _level;
	int _exp;
	short _damage;

	short _maxHp;
	short _defaultX, _defaultY;

	std::atomic<bool> _isAlive{ true };

public:
	int _lastMoveTime;
	int _lastAttackTime;
};

class NPC : 
	public IocpObject, 
	public GameObject,
	public std::enable_shared_from_this<NPC> 
{
	friend class GameSesison;

public:
	NPC(int id, short x, short y, std::string_view name);

public:
	void WakeUp(bool force = false);
	void OnMove();
	void OnHeal();
	void RegisterTimer();

	// test 용
	void RegisterSuicideTimer();

public:
	std::unordered_set<int> RandomMove(std::shared_ptr<Service> service);
	std::unordered_set<int> AStarMove(std::shared_ptr<Service> service, APos npcPos, APos targetPos);

public:
	// Service 관련
	void SetService(std::shared_ptr<Service> service) { _service = service; }
	const std::shared_ptr<Service>& GetService() const { return _service.lock(); }

public:
	// 인터페이스 구현
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class ExpOver* expOver, int nuOfBytes = 0) override;

private:
	std::weak_ptr<Service> _service;

public:
	std::atomic<bool> _isActive{ false };
	std::atomic<bool> _movePending{ false };
	std::atomic<bool> _healPending{ false };
};