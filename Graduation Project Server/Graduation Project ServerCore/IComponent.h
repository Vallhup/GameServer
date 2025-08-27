#pragma once

class GameObject;
class IGameContext;
class Instance;

class IComponent {
public:
	IComponent() = delete;
	IComponent(GameObject& owner, Instance& instance) 
		: _owner(owner), _instance(instance) { _version = 0; _lastSentVersion = 0;_enable = false; }
	virtual ~IComponent() = default;

public:
	void Register() { OnRegister(); }
	void Deregister() { OnDeregister(); }

protected:
	virtual void OnRegister() = 0;
	virtual void OnDeregister() = 0;
	virtual void OnActivate() = 0;
	virtual void OnDeactivate() = 0;

public:
	uint64_t Version() const { return _version.load(); }
	bool VersionCheckAndChange();
	bool Enable() const { return _enable.load(); }

	void SetEnable(bool e);

protected:
	GameObject& _owner;
	Instance& _instance;

	std::atomic<bool> _enable;
	std::atomic<uint64_t> _version;
	uint64_t _lastSentVersion;
};

class ITickable {
public:
	virtual ~ITickable() = default;
	
public:
	virtual void Tick(float deltaTime) = 0;
	virtual bool TickEnable() = 0;
};

class IInputable {
public:
	IInputable() = delete;
	IInputable(int id) : _id(id) {};
	virtual ~IInputable() = default;

public:
	virtual void HandleInput(const Protocol::CS_INPUT_PACKET& packet) = 0;
	virtual void InputEnable() = 0;

public:
	int GetId() const { return _id; }

protected:
	int _id;
};